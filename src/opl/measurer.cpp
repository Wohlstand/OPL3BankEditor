/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef IS_QT_4
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#endif
#include <QQueue>
#include <QProgressDialog>

#include <vector>
#include <chrono>
#include <cmath>
#include <memory>
#include <fstream>
#include <cstring>
#include <cstdio>
#include <limits>

#include "measurer.h"

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif

//Measurer is always needs for emulator
#include "chips/opl_chip_base.h"
#include "chips/nuked_opl3.h"
#include "chips/nuked_opl3_v174.h"
#include "chips/dosbox_opl3.h"
#include "chips/opal_opl3.h"
#include "chips/java_opl3.h"

//typedef NukedOPL3 DefaultOPL3;
typedef DosBoxOPL3 DefaultOPL3;

typedef Measurer::DurationInfo DurationInfo;

template <class T>
class AudioHistory
{
    std::unique_ptr<T[]> m_data;
    size_t m_index = 0;  // points to the next write slot
    size_t m_length = 0;
    size_t m_capacity = 0;

public:
    size_t size() const { return m_length; }
    size_t capacity() const { return m_capacity; }
    const T *data() const { return &m_data[m_index + m_capacity - m_length]; }

    void reset(size_t capacity)
    {
        m_data.reset(new T[2 * capacity]());
        m_index = 0;
        m_length = 0;
        m_capacity = capacity;
    }

    void clear()
    {
        m_length = 0;
    }

    void add(const T &item)
    {
        T *data = m_data.get();
        const size_t capacity = m_capacity;
        size_t index = m_index;
        data[index] = item;
        data[index + capacity] = item;
        m_index = (index + 1 != capacity) ? (index + 1) : 0;
        size_t length = m_length + 1;
        m_length = (length < capacity) ? length : capacity;
    }
};

static void HannWindow(double *w, unsigned n)
{
    for (unsigned i = 0; i < n; ++i)
        w[i] = 0.5 * (1.0 - std::cos(2 * M_PI * i / (n - 1)));
}

static double MeasureRMS(const double *signal, const double *window, unsigned length)
{
    double mean = 0;
#pragma omp simd reduction(+: mean)
    for(unsigned i = 0; i < length; ++i)
        mean += window[i] * signal[i];
    mean /= length;

    double rms = 0;
#pragma omp simd reduction(+: rms)
    for(unsigned i = 0; i < length; ++i)
    {
        double diff = window[i] * signal[i] - mean;
        rms += diff * diff;
    }
    rms = std::sqrt(rms / (length - 1));

    return rms;
}

#ifdef DEBUG_WRITE_AMPLITUDE_PLOT
static bool WriteAmplitudePlot(
    const std::string &fileprefix,
    const std::vector<double> &amps_on,
    const std::vector<double> &amps_off,
    double timestep)
{
    std::string datafile = fileprefix + ".dat";
    std::string gpfile_on_off[2] =
        { fileprefix + "-on.gp",
          fileprefix + "-off.gp" };
    const char *plot_title[2] =
        { "Key-On Amplitude", "Key-Off Amplitude" };

#if !defined(_WIN32)
    size_t datafile_base = datafile.rfind("/");
#else
    size_t datafile_base = datafile.find_last_of("/\\");
#endif
    datafile_base = (datafile_base == datafile.npos) ? 0 : (datafile_base + 1);

    size_t n_on = amps_on.size();
    size_t n_off = amps_off.size();
    size_t n = (n_on > n_off) ? n_on : n_off;

    std::ofstream outs;

    outs.open(datafile);
    if(outs.bad())
        return false;
    for(size_t i = 0; i < n; ++i)
    {
        const double nan = std::numeric_limits<double>::quiet_NaN();
        double values[2] =
            { (i < n_on) ? amps_on[i] : nan,
              (i < n_off) ? amps_off[i] : nan };
        outs << i * timestep;
        for(unsigned j = 0; j < 2; ++j)
        {
            if(!std::isnan(values[j]))
                outs << ' ' << values[j];
            else
                outs << " m";
        }
        outs << '\n';
    }
    outs.flush();
    if(outs.bad())
        return false;
    outs.close();

    for(unsigned i = 0; i < 2; ++i)
    {
        outs.open(gpfile_on_off[i]);
        if(outs.bad())
            return false;
        outs << "set datafile missing \"m\"\n";
        outs << "plot \"" << datafile.substr(datafile_base) <<  "\""
            " u 1:" << 2 + i << " w linespoints pt 4"
            " t \"" << plot_title[i] << "\"\n";
        outs.flush();
        if(outs.bad())
            return false;
        outs.close();
    }

    return true;
}
#endif

static const unsigned g_outputRate = 49716;

struct TinySynth
{
    OPLChipBase *m_chip;
    unsigned m_notesNum;
    int m_notenum;
    int8_t m_fineTune;
    int16_t m_noteOffsets[2];
    unsigned m_x[2];

    void resetChip()
    {
        static const short initdata[(2 + 3 + 2 + 2) * 2] =
        {
            0x004, 96, 0x004, 128,      // Pulse timer
            0x105, 0, 0x105, 1, 0x105, 0, // Pulse OPL3 enable, leave disabled
            0x001, 32, 0x0BD, 0         // Enable wave & melodic
        };

        m_chip->setRate(g_outputRate);

        for(unsigned a = 0; a < 18; a += 2)
            m_chip->writeReg((uint16_t)initdata[a], (uint8_t)initdata[a + 1]);
    }

    void setInstrument(const FmBank::Instrument *in_p)
    {
        const FmBank::Instrument &in = *in_p;
        uint8_t rawData[2][11];

        std::memset(m_x, 0, sizeof(m_x));
        m_notenum = in.percNoteNum >= 128 ? (in.percNoteNum - 128) : in.percNoteNum;
        if(m_notenum == 0)
            m_notenum = 25;
        m_notesNum = (in.en_4op || in.en_pseudo4op) ? 2 : 1;
        m_fineTune = 0;
        m_noteOffsets[0] = in.note_offset1;
        m_noteOffsets[1] = in.note_offset2;
        if(in.en_pseudo4op)
            m_fineTune = in.fine_tune;
        if((m_notesNum == 2) && !in.en_pseudo4op)
        {
            m_chip->writeReg(0x105, 1);
            m_chip->writeReg(0x104, 0xFF);
        }

        rawData[0][0] = in.getAVEKM(MODULATOR1) & 0x3F; //For clearer measurement, disable tremolo and vibrato
        rawData[0][1] = in.getAVEKM(CARRIER1) & 0x3F;
        rawData[0][2] = in.getAtDec(MODULATOR1);
        rawData[0][3] = in.getAtDec(CARRIER1);
        rawData[0][4] = in.getSusRel(MODULATOR1);
        rawData[0][5] = in.getSusRel(CARRIER1);
        rawData[0][6] = in.getWaveForm(MODULATOR1);
        rawData[0][7] = in.getWaveForm(CARRIER1);
        rawData[0][8] = in.getKSLL(MODULATOR1);
        rawData[0][9] = in.getKSLL(CARRIER1);
        rawData[0][10] = in.getFBConn1();

        rawData[1][0] = in.getAVEKM(MODULATOR2) & 0x3F;
        rawData[1][1] = in.getAVEKM(CARRIER2) & 0x3F;
        rawData[1][2] = in.getAtDec(MODULATOR2);
        rawData[1][3] = in.getAtDec(CARRIER2);
        rawData[1][4] = in.getSusRel(MODULATOR2);
        rawData[1][5] = in.getSusRel(CARRIER2);
        rawData[1][6] = in.getWaveForm(MODULATOR2);
        rawData[1][7] = in.getWaveForm(CARRIER2);
        rawData[1][8] = in.getKSLL(MODULATOR2);
        rawData[1][9] = in.getKSLL(CARRIER2);
        rawData[1][10] = in.getFBConn2();

        for(unsigned n = 0; n < m_notesNum; ++n)
        {
            static const unsigned char patchdata[11] =
            {0x20, 0x23, 0x60, 0x63, 0x80, 0x83, 0xE0, 0xE3, 0x40, 0x43, 0xC0};
            for(unsigned a = 0; a < 10; ++a)
                m_chip->writeReg(patchdata[a] + n * 8, rawData[n][a]);
            m_chip->writeReg(patchdata[10] + n * 8, rawData[n][10] | 0x30);
        }
    }

    void noteOn()
    {
        std::memset(m_x, 0, sizeof(m_x));
        for(unsigned n = 0; n < m_notesNum; ++n)
        {
            double hertz = 172.00093 * std::exp(0.057762265 * (m_notenum + m_noteOffsets[n]));
            if(hertz > 131071)
            {
                std::fprintf(stderr, "MEASURER WARNING: Why does note %d + note-offset %d produce hertz %g?          \n",
                             m_notenum, m_noteOffsets[n], hertz);
                hertz = 131071;
            }
            m_x[n] = 0x2000;
            while(hertz >= 1023.5)
            {
                hertz /= 2.0;    // Calculate octave
                m_x[n] += 0x400;
            }
            m_x[n] += (unsigned int)(hertz + 0.5);

            // Keyon the note
            m_chip->writeReg(0xA0 + n * 3, m_x[n] & 0xFF);
            m_chip->writeReg(0xB0 + n * 3, m_x[n] >> 8);
        }
    }

    void noteOff()
    {
        // Keyoff the note
        for(unsigned n = 0; n < m_notesNum; ++n)
            m_chip->writeReg(0xB0 + n * 3, (m_x[n] >> 8) & 0xDF);
    }

    void generate(int16_t *output, size_t frames)
    {
        m_chip->generate(output, frames);
    }
};

static void BenchmarkChip(FmBank::Instrument *in_p, OPLChipBase *chip)
{
    TinySynth synth;
    synth.m_chip = chip;
    synth.resetChip();
    synth.setInstrument(in_p);

    const unsigned interval             = 150;
    const unsigned samples_per_interval = g_outputRate / interval;
    const unsigned max_on  = 10;
    const unsigned max_off = 20;

    unsigned max_period_on = max_on * interval;
    unsigned max_period_off = max_off * interval;

    const size_t audioBufferLength = 256;
    const size_t audioBufferSize = 2 * audioBufferLength;
    int16_t audioBuffer[audioBufferSize];

    synth.noteOn();
    for(unsigned period = 0; period < max_period_on; ++period)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < audioBufferLength) ? blocksize : audioBufferLength;
            synth.generate(audioBuffer, blocksize);
            i += blocksize;
        }
    }

    synth.noteOff();
    for(unsigned period = 0; period < max_period_off; ++period)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < 256) ? blocksize : 256;
            synth.generate(audioBuffer, blocksize);
            i += blocksize;
        }
    }
}

static void ComputeDurations(const FmBank::Instrument *in_p, DurationInfo *result_p, OPLChipBase *chip)
{
    const FmBank::Instrument &in = *in_p;
    DurationInfo &result = *result_p;

    AudioHistory<double> audioHistory;

    const unsigned interval             = 150;
    const unsigned samples_per_interval = g_outputRate / interval;

    const double historyLength = 0.1;  // maximum duration to memorize (seconds)
    audioHistory.reset(std::ceil(historyLength * g_outputRate));

#if defined(ENABLE_PLOTS) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
    const double timestep = (double)samples_per_interval / g_outputRate;  // interval between analysis steps (seconds)
#endif
#if defined(ENABLE_PLOTS)
    result.amps_timestep = timestep;
#endif

    std::unique_ptr<double[]> window;
    window.reset(new double[audioHistory.capacity()]);
    unsigned winsize = 0;

    TinySynth synth;
    synth.m_chip = chip;
    synth.resetChip();
    synth.setInstrument(&in);
    synth.noteOn();

    /* For capturing */
    const unsigned max_silent = 6;
    const unsigned max_on  = 40;
    const unsigned max_off = 60;

    unsigned max_period_on = max_on * interval;
    unsigned max_period_off = max_off * interval;

    const double min_coefficient_on = 0.008;
    const double min_coefficient_off = 0.003;

    unsigned windows_passed_on = 0;
    unsigned windows_passed_off = 0;

    /* For Analyze the results */
    double begin_amplitude        = 0;
    double peak_amplitude_value   = 0;
    size_t peak_amplitude_time    = 0;
    size_t quarter_amplitude_time = max_period_on;
    bool   quarter_amplitude_time_found = false;
    size_t keyoff_out_time        = 0;
    bool   keyoff_out_time_found  = false;

    const size_t audioBufferLength = 256;
    const size_t audioBufferSize = 2 * audioBufferLength;
    int16_t audioBuffer[audioBufferSize];

    // For up to 40 seconds, measure mean amplitude.
    double highest_sofar = 0;
    short sound_min = 0, sound_max = 0;

#if defined(ENABLE_PLOTS)
    std::vector<double> &amplitudecurve_on = result.amps_on;
    amplitudecurve_on.clear();
    amplitudecurve_on.reserve(max_period_on);
#elif defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
    std::vector<double> amplitudecurve_on;
    amplitudecurve_on.reserve(max_period_on);
#endif
    for(unsigned period = 0; period < max_period_on; ++period, ++windows_passed_on)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < audioBufferLength) ? blocksize : audioBufferLength;
            synth.generate(audioBuffer, blocksize);
            for (unsigned j = 0; j < blocksize; ++j)
            {
                int16_t s = audioBuffer[2 * j];
                audioHistory.add(s);
                if(sound_min > s) sound_min = s;
                if(sound_max < s) sound_max = s;
            }
            i += blocksize;
        }

        if(winsize != audioHistory.size())
        {
            winsize = audioHistory.size();
            HannWindow(window.get(), winsize);
        }

        double rms = MeasureRMS(audioHistory.data(), window.get(), winsize);
        /* ======== Peak time detection ======== */
        if(period == 0)
        {
            begin_amplitude = rms;
            peak_amplitude_value = rms;
            peak_amplitude_time = 0;
        }
        else if(rms > peak_amplitude_value)
        {
            peak_amplitude_value = rms;
            peak_amplitude_time  = period;
            // In next step, update the quater amplitude time
            quarter_amplitude_time_found = false;
        }
        else if(!quarter_amplitude_time_found && (rms <= peak_amplitude_value * min_coefficient_on))
        {
            quarter_amplitude_time = period;
            quarter_amplitude_time_found = true;
        }
        /* ======== Peak time detection =END==== */
#if defined(ENABLE_PLOTS) || defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
        amplitudecurve_on.push_back(rms);
#endif
        if(rms > highest_sofar)
            highest_sofar = rms;

        if((period > max_silent * interval) &&
           ( (rms < highest_sofar * min_coefficient_on) || (sound_min >= -1 && sound_max <= 1) )
        )
            break;
    }

    if(!quarter_amplitude_time_found)
        quarter_amplitude_time = windows_passed_on;

#ifdef DEBUG_AMPLITUDE_PEAK_VALIDATION
    char outBufOld[250];
    char outBufNew[250];
    std::memset(outBufOld, 0, 250);
    std::memset(outBufNew, 0, 250);

    std::snprintf(outBufOld, 250, "Peak: beg=%g, peakv=%g, peakp=%zu, q=%zu",
                begin_amplitude,
                peak_amplitude_value,
                peak_amplitude_time,
                quarter_amplitude_time);

    /* Detect the peak time */
    begin_amplitude        = amplitudecurve_on[0];
    peak_amplitude_value   = begin_amplitude;
    peak_amplitude_time    = 0;
    quarter_amplitude_time = amplitudecurve_on.size();
    keyoff_out_time        = 0;
    for(size_t a = 1; a < amplitudecurve_on.size(); ++a)
    {
        if(amplitudecurve_on[a] > peak_amplitude_value)
        {
            peak_amplitude_value = amplitudecurve_on[a];
            peak_amplitude_time  = a;
        }
    }
    for(size_t a = peak_amplitude_time; a < amplitudecurve_on.size(); ++a)
    {
        if(amplitudecurve_on[a] <= peak_amplitude_value * min_coefficient_on)
        {
            quarter_amplitude_time = a;
            break;
        }
    }

    std::snprintf(outBufNew, 250, "Peak: beg=%g, peakv=%g, peakp=%zu, q=%zu",
                begin_amplitude,
                peak_amplitude_value,
                peak_amplitude_time,
                quarter_amplitude_time);

    if(memcmp(outBufNew, outBufOld, 250) != 0)
    {
        qDebug() << "Pre: " << outBufOld << "\n" <<
                    "Pos: " << outBufNew;
    }
#endif

    if(windows_passed_on >= max_period_on)
    {
        // Just Keyoff the note
        synth.noteOff();
    }
    else
    {
        // Reset the emulator and re-run the "ON" simulation until reaching the peak time
        synth.resetChip();
        synth.setInstrument(&in);
        synth.noteOn();

        audioHistory.reset(std::ceil(historyLength * g_outputRate));
        for(unsigned period = 0;
            ((period < peak_amplitude_time) || (period == 0)) && (period < max_period_on);
            ++period)
        {
            for(unsigned i = 0; i < samples_per_interval;)
            {
                size_t blocksize = samples_per_interval - i;
                blocksize = (blocksize < audioBufferLength) ? blocksize : audioBufferLength;
                synth.generate(audioBuffer, blocksize);
                for (unsigned j = 0; j < blocksize; ++j)
                    audioHistory.add(audioBuffer[2 * j]);
                i += blocksize;
            }
        }
        synth.noteOff();
    }

    // Now, for up to 60 seconds, measure mean amplitude.
#if defined(ENABLE_PLOTS)
    std::vector<double> &amplitudecurve_off = result.amps_off;
    amplitudecurve_off.clear();
    amplitudecurve_off.reserve(max_period_on);
#elif defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
    std::vector<double> amplitudecurve_off;
    amplitudecurve_off.reserve(max_period_off);
#endif
    for(unsigned period = 0; period < max_period_off; ++period, ++windows_passed_off)
    {
        for(unsigned i = 0; i < samples_per_interval;)
        {
            size_t blocksize = samples_per_interval - i;
            blocksize = (blocksize < 256) ? blocksize : 256;
            synth.generate(audioBuffer, blocksize);
            for (unsigned j = 0; j < blocksize; ++j)
            {
                int16_t s = audioBuffer[2 * j];
                audioHistory.add(s);
                if(sound_min > s) sound_min = s;
                if(sound_max < s) sound_max = s;
            }
            i += blocksize;
        }

        if(winsize != audioHistory.size())
        {
            winsize = audioHistory.size();
            HannWindow(window.get(), winsize);
        }

        double rms = MeasureRMS(audioHistory.data(), window.get(), winsize);
        /* ======== Find Key Off time ======== */
        if(!keyoff_out_time_found && (rms <= peak_amplitude_value * min_coefficient_off))
        {
            keyoff_out_time = period;
            keyoff_out_time_found = true;
        }
        /* ======== Find Key Off time ==END=== */
#if defined(ENABLE_PLOTS) || defined(DEBUG_AMPLITUDE_PEAK_VALIDATION) || defined(DEBUG_WRITE_AMPLITUDE_PLOT)
        amplitudecurve_off.push_back(rms);
#endif
        if(rms < highest_sofar * min_coefficient_off)
            break;

        if((period > max_silent * interval) && (sound_min >= -1 && sound_max <= 1))
            break;
    }

#ifdef DEBUG_WRITE_AMPLITUDE_PLOT
    WriteAmplitudePlot(
        "/tmp/amplitude", amplitudecurve_on, amplitudecurve_off, timestep);
#endif

#ifdef DEBUG_AMPLITUDE_PEAK_VALIDATION
    size_t debug_peak_old = keyoff_out_time;

    /* Analyze the final results */
    for(size_t a = 0; a < amplitudecurve_off.size(); ++a)
    {
        if(amplitudecurve_off[a] <= peak_amplitude_value * min_coefficient_off)
        {
            keyoff_out_time = a;
            break;
        }
    }

    if(debug_peak_old != keyoff_out_time)
    {
        qDebug() << "KeyOff time is 1:" << debug_peak_old << " and 2:" << keyoff_out_time;
    }
#endif

    result.peak_amplitude_time = peak_amplitude_time;
    result.peak_amplitude_value = peak_amplitude_value;
    result.begin_amplitude = begin_amplitude;
    result.quarter_amplitude_time = (double)quarter_amplitude_time;
    result.keyoff_out_time = (double)keyoff_out_time;

    result.ms_sound_kon  = (int64_t)(quarter_amplitude_time * 1000.0 / interval);
    result.ms_sound_koff = (int64_t)(keyoff_out_time        * 1000.0 / interval);
    result.nosound = (peak_amplitude_value < 0.5) || ((sound_min >= -1) && (sound_max <= 1));
}

static void ComputeDurationsDefault(const FmBank::Instrument *in, DurationInfo *result)
{
    DefaultOPL3 chip;
    ComputeDurations(in, result, &chip);
}

static void MeasureDurations(FmBank::Instrument *in_p, OPLChipBase *chip)
{
    FmBank::Instrument &in = *in_p;
    DurationInfo result;

    if(in_p->rhythm_drum_type == 0)
    {
        ComputeDurations(&in, &result, chip);
        in.ms_sound_kon = (uint16_t)result.ms_sound_kon;
        in.ms_sound_koff = (uint16_t)result.ms_sound_koff;
        in.is_blank = result.nosound;
    }
    else // Rhyth-mode percussion
    {
        in.ms_sound_kon = 1;
        in.ms_sound_koff = 1;
        in.is_blank = false;
    }
}

static void MeasureDurationsDefault(FmBank::Instrument *in_p)
{
    DefaultOPL3 chip;
    MeasureDurations(in_p, &chip);
}

static void MeasureDurationsBenchmark(FmBank::Instrument *in_p, OPLChipBase *chip, QVector<Measurer::BenchmarkResult> *result)
{
    std::chrono::steady_clock::time_point start, stop;
    Measurer::BenchmarkResult res;
    start = std::chrono::steady_clock::now();
    BenchmarkChip(in_p, chip);
    stop  = std::chrono::steady_clock::now();
    res.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start).count();
    res.name = QString::fromUtf8(chip->emulatorName());
    result->push_back(res);
}

static void MeasureDurationsBenchmarkRunner(FmBank::Instrument *in_p, QVector<Measurer::BenchmarkResult> *result)
{
    std::vector<std::shared_ptr<OPLChipBase > > emuls =
    {
        std::shared_ptr<OPLChipBase>(new NukedOPL3v174),
        std::shared_ptr<OPLChipBase>(new NukedOPL3),
        std::shared_ptr<OPLChipBase>(new DosBoxOPL3),
        std::shared_ptr<OPLChipBase>(new OpalOPL3),
        std::shared_ptr<OPLChipBase>(new JavaOPL3)
    };
    for(std::shared_ptr<OPLChipBase> &p : emuls)
        MeasureDurationsBenchmark(in_p, p.get(), result);
}

Measurer::Measurer(QWidget *parent) :
    QObject(parent),
    m_parentWindow(parent)
{}

Measurer::~Measurer()
{}

static void insertOrBlank(FmBank::Instrument &ins, const FmBank::Instrument &blank, QQueue<FmBank::Instrument *> &tasks)
{
    ins.is_blank = false;
    if(memcmp(&ins, &blank, sizeof(FmBank::Instrument)) != 0)
        tasks.enqueue(&ins);
    else
    {
        ins.is_blank = true;
        ins.ms_sound_kon = 0;
        ins.ms_sound_koff = 0;
    }
}

bool Measurer::doMeasurement(FmBank &bank, FmBank &bankBackup, bool forceReset)
{
    QQueue<FmBank::Instrument *> tasks;
    FmBank::Instrument blank = FmBank::emptyInst();

    int i = 0;
    for(i = 0; i < bank.Ins_Melodic_box.size() && i < bankBackup.Ins_Melodic_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Melodic_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Melodic_box[i];
        if(forceReset || (ins1.ms_sound_kon == 0) || (memcmp(&ins1, &ins2, sizeof(FmBank::Instrument)) != 0))
        {
            ins1.rhythm_drum_type = 0;
            ins2.rhythm_drum_type = 0; // Just in a case, be sure this value is zero for all melodic instruments
            insertOrBlank(ins1, blank, tasks);
        }
    }
    for(; i < bank.Ins_Melodic_box.size(); i++)
        insertOrBlank(bank.Ins_Melodic_box[i], blank, tasks);

    for(i = 0; i < bank.Ins_Percussion_box.size() && i < bankBackup.Ins_Percussion_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Percussion_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Percussion_box[i];
        if(forceReset || (ins1.ms_sound_kon == 0) || (memcmp(&ins1, &ins2, sizeof(FmBank::Instrument)) != 0))
            insertOrBlank(ins1, blank, tasks);
    }
    for(; i < bank.Ins_Percussion_box.size(); i++)
        insertOrBlank(bank.Ins_Percussion_box[i], blank, tasks);

    if(tasks.isEmpty())
        return true;// Nothing to do! :)

    QProgressDialog m_progressBox(m_parentWindow);
    m_progressBox.setWindowModality(Qt::WindowModal);
    m_progressBox.setWindowTitle(tr("Sounding delay calculation"));
    m_progressBox.setLabelText(tr("Please wait..."));

#ifndef IS_QT_4
    QFutureWatcher<void> watcher;
    watcher.connect(&m_progressBox, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    watcher.connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &m_progressBox, SLOT(setRange(int,int)));
    watcher.connect(&watcher, SIGNAL(progressValueChanged(int)), &m_progressBox, SLOT(setValue(int)));
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

    watcher.setFuture(QtConcurrent::map(tasks, &MeasureDurationsDefault));

    m_progressBox.exec();
    watcher.waitForFinished();

    tasks.clear();

    // Apply all calculated values into backup store to don't re-calculate same stuff
    for(i = 0; i < bank.Ins_Melodic_box.size() && i < bankBackup.Ins_Melodic_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Melodic_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Melodic_box[i];
        ins2.ms_sound_kon  = ins1.ms_sound_kon;
        ins2.ms_sound_koff = ins1.ms_sound_koff;
        ins2.is_blank = ins1.is_blank;
    }
    for(i = 0; i < bank.Ins_Percussion_box.size() && i < bankBackup.Ins_Percussion_box.size(); i++)
    {
        FmBank::Instrument &ins1 = bank.Ins_Percussion_box[i];
        FmBank::Instrument &ins2 = bankBackup.Ins_Percussion_box[i];
        ins2.ms_sound_kon  = ins1.ms_sound_kon;
        ins2.ms_sound_koff = ins1.ms_sound_koff;
        ins2.is_blank = ins1.is_blank;
    }

    return !watcher.isCanceled();

#else
    m_progressBox.setMaximum(tasks.size());
    m_progressBox.setValue(0);
    int count = 0;
    foreach(FmBank::Instrument *ins, tasks)
    {
        MeasureDurationsDefault(ins);
        m_progressBox.setValue(++count);
        if(m_progressBox.wasCanceled())
            return false;
    }
    return true;
#endif
}

bool Measurer::doMeasurement(FmBank::Instrument &instrument)
{
    QProgressDialog m_progressBox(m_parentWindow);
    m_progressBox.setWindowModality(Qt::WindowModal);
    m_progressBox.setWindowTitle(tr("Sounding delay calculation"));
    m_progressBox.setLabelText(tr("Please wait..."));

#ifndef IS_QT_4
    QFutureWatcher<void> watcher;
    watcher.connect(&m_progressBox, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    watcher.connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &m_progressBox, SLOT(setRange(int,int)));
    watcher.connect(&watcher, SIGNAL(progressValueChanged(int)), &m_progressBox, SLOT(setValue(int)));
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

    watcher.setFuture(QtConcurrent::run(&MeasureDurationsDefault, &instrument));
    m_progressBox.exec();
    watcher.waitForFinished();

    return !watcher.isCanceled();

#else
    m_progressBox.show();
    MeasureDurationsDefault(&instrument);
    return true;
#endif
}

bool Measurer::doComputation(const FmBank::Instrument &instrument, DurationInfo &result)
{
    QProgressDialog m_progressBox(m_parentWindow);
    m_progressBox.setWindowModality(Qt::WindowModal);
    m_progressBox.setWindowTitle(tr("Sounding delay calculation"));
    m_progressBox.setLabelText(tr("Please wait..."));

#ifndef IS_QT_4
    QFutureWatcher<void> watcher;
    watcher.connect(&m_progressBox, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    watcher.connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &m_progressBox, SLOT(setRange(int,int)));
    watcher.connect(&watcher, SIGNAL(progressValueChanged(int)), &m_progressBox, SLOT(setValue(int)));
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

    watcher.setFuture(QtConcurrent::run(&ComputeDurationsDefault, &instrument, &result));
    m_progressBox.exec();
    watcher.waitForFinished();

    return !watcher.isCanceled();

#else
    m_progressBox.show();
    ComputeDurationsDefault(&instrument, &result);
    return true;
#endif
}

bool Measurer::runBenchmark(FmBank::Instrument &instrument, QVector<Measurer::BenchmarkResult> &result)
{
    QProgressDialog m_progressBox(m_parentWindow);
    m_progressBox.setWindowModality(Qt::WindowModal);
    m_progressBox.setWindowTitle(tr("Benchmarking emulators"));
    m_progressBox.setLabelText(tr("Please wait..."));
    m_progressBox.setCancelButton(nullptr);


#ifndef IS_QT_4
    QFutureWatcher<void> watcher;
    watcher.connect(&m_progressBox, SIGNAL(canceled()), &watcher, SLOT(cancel()));
    watcher.connect(&watcher, SIGNAL(progressRangeChanged(int,int)), &m_progressBox, SLOT(setRange(int,int)));
    watcher.connect(&watcher, SIGNAL(progressValueChanged(int)), &m_progressBox, SLOT(setValue(int)));
    watcher.connect(&watcher, SIGNAL(finished()), &m_progressBox, SLOT(accept()));

    watcher.setFuture(QtConcurrent::run(MeasureDurationsBenchmarkRunner, &instrument, &result));
    m_progressBox.exec();
    watcher.waitForFinished();
#else
    m_progressBox.show();
    MeasureDurationsBenchmarkRunner(&instrument, &result);
#endif

    return true;
}
