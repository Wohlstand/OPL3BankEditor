/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef GENERATOR_H
#define GENERATOR_H

#include <stdint.h>
#include <memory>
#include <QIODevice>
#include <QObject>

#include "chips/opl_chip_base.h"
#include "../bank.h"

#ifdef ENABLE_HW_OPL_PROXY
class Win9x_OPL_Proxy;
#endif

#define NUM_OF_CHANNELS         23
#define MAX_OPLGEN_BUFFER_SIZE  4096

struct OPL_Operator
{
    //! Operator properties
    uint32_t    modulator_E862, carrier_E862;
    //! AM/VIB/EGT/KSR/MULT settings
    uint8_t     modulator_20, carrier_20;
    //! KSL/attenuation settings
    uint8_t     modulator_40, carrier_40;
    //! Feedback/connection bits for the channel
    uint8_t     feedconn;
    //! Fine tuning
    int8_t      finetune;
};

struct OPL_PatchSetup
{
    enum
    {
        Flag_True4op    = 0x04,
        Flag_Pseudo4op  = 0x01,
        Flag_NoSound    = 0x02
    };
    //! Operators prepared for sending to OPL chip emulator
    OPL_Operator OPS[2];
    //! Single note (for percussion instruments)
    uint8_t     tone;
    //! Extra patch flags
    uint8_t     flags;
    //! Pseudo-4op second voice only
    double         voice2_fine_tune;
};

struct GeneratorDebugInfo
{
    int chan2op = -1;
    int chanPs4op = -1;
    int chan4op = -1;
    QString toStr();
};

class Generator
{
public:
    enum OPL_Chips
    {
        CHIP_Nuked = 0,
        CHIP_DosBox,
        CHIP_Opal,
        CHIP_Win9xProxy
    };
    Generator(uint32_t sampleRate, OPL_Chips initialChip);
    ~Generator();

    void initChip();
    void switchChip(OPL_Chips chipId);

    void generate(int16_t *frames, unsigned nframes);

    /**
     * @brief Set the tone frequency on the chip channel and turn note on
     * @param c1 2-op channel or 4-op master channel index
     * @param c2 4-op slave channel, unused with 2-op
     * @param voice2ps4op where it is the second voice of a pseudo-4op instrument
     * @param hertz Tone frequency
     */
    void NoteOn(uint32_t c1, uint32_t c2, double hertz, bool voice2ps4op = false);

    /**
     * @brief Turn the note off
     * @param c 2-op channel or master 4-op channel index
     */
    void NoteOff(uint32_t c);

    void Touch_Real(uint32_t c, uint32_t volume);
    void Touch(uint32_t c, uint32_t volume);
    void Patch(uint32_t c, uint32_t i);
    void Pan(uint32_t c, uint32_t value);
    void PlayNoteF(int noteID, uint32_t volume = 127, uint8_t ccvolume = 100, uint8_t ccexpr = 127);
    void PlayNoteCh(int channelID);
    void StopNoteF(int noteID);
    void StopNoteCh(int channelID);
    void PlayDrum(uint8_t drum, int noteID);
    void switch4op(bool enabled, bool patchCleanUp = true);

    enum VolumesScale
    {
        VOLUME_Generic,
        VOLUME_CMF,
        VOLUME_DMX,
        VOLUME_APOGEE,
        VOLUME_9X
    };

public:
    void Silence();
    void NoteOffAllChans();

    void PlayNote(uint32_t volume = 127, uint8_t ccvolume = 100, uint8_t ccexpr = 127);
    void PlayMajorChord();
    void PlayMinorChord();
    void PlayAugmentedChord();
    void PlayDiminishedChord();
    void PlayMajor7Chord();
    void PlayMinor7Chord();
    void StopNote();
    void PitchBend(int bend);
    void PitchBendSensitivity(int cents);
    void Hold(bool held);

    void changePatch(const FmBank::Instrument &instrument, bool isDrum = false);
    void changeNote(int newnote);
    void changeDeepTremolo(bool enabled);
    void changeDeepVibrato(bool enabled);
    void changeVolumeModel(int volmodel);
    void changeRhythmMode(bool enabled);
    void updateRegBD();

    const GeneratorDebugInfo &debugInfo() const
        { return m_debug; }

    static uint32_t getChipVolume(
        uint32_t volume, uint8_t ccvolume, uint8_t ccexpr, int volmodel);

#ifdef ENABLE_HW_OPL_PROXY
    static Win9x_OPL_Proxy &oplProxy();
#endif

private:
    GeneratorDebugInfo m_debug;

private:
    void WriteReg(uint16_t address, uint8_t byte);

    class NotesManager
    {
    public:
        struct Note
        {
            //! Currently pressed key. -1 means channel is free
            int note    = -1;
            //! Note volume determined by velocity
            uint32_t volume = 0;
            //! Channel volume determined by controller
            uint8_t ccvolume = 0;
            //! Channel expression determined by controller
            uint8_t ccexpr = 0;
            //! Age in count of noteOn requests
            int age = 0;
            //! Whether it has a pending noteOff being delayed while held
            bool held = false;
        };
    private:
        //! Channels range, contains entries count equal to chip channels
        QVector<Note> channels;
        //! Round-Robin cycler. Looks for any free channel that is not busy. Otherwise, oldest busy note will be replaced
        uint8_t cycle = 0;
    public:
        NotesManager();
        ~NotesManager();
        void allocateChannels(int count);
        uint8_t noteOn(int note, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr, bool *replace = nullptr);
        int8_t  noteOff(int note);
        void    channelOff(int ch);
        int8_t  findNoteOffChannel(int note);
        void hold(int ch, bool h);
        void clearNotes();
        const Note &channel(int ch) const
            { return channels.at(ch); }
        int channelCount() const
            { return static_cast<int>(channels.size()); }
    } m_noteManager;

    int32_t     note;
    double      m_bend = 0.0;
    double      m_bendsense = 2.0 / 8192;
    bool        m_hold = false;
    int         m_volmodel = VOLUME_Generic;
    bool        m_isInstrumentLoaded = false;
    bool        m_4op_last_state;
    uint8_t     deepTremoloMode;
    uint8_t     deepVibratoMode;
    uint8_t     rythmModePercussionMode;
    uint8_t     testDrum;
    uint32_t    m_rate = 44100;
    struct OPLChipDelete { void operator()(OPLChipBase *); };
    std::unique_ptr<OPLChipBase, OPLChipDelete> chip;

    OPL_PatchSetup m_patch;
    uint8_t     m_regBD;

    /**
     * @brief Channel categiry enumeration
     */
    enum ChanCat
    {
        //! Regular melodic/percussion channel
        ChanCat_Regular     = 0,
        //! Four-op master
        ChanCat_4op_Master  = 1,
        //! Four-op slave
        ChanCat_4op_Slave   = 2,
        //! Rhythm-mode Bass drum
        ChanCat_Rhythm_Bass     = 3,
        //! Rhythm-mode Snare drum
        ChanCat_Rhythm_Snare    = 4,
        //! Rhythm-mode Tom-Tom
        ChanCat_Rhythm_Tom      = 5,
        //! Rhythm-mode Cymbal
        ChanCat_Rhythm_Cymbal   = 6,
        //! Rhythm-mode Hi-Hat
        ChanCat_Rhythm_HiHat    = 7,
        //! Rhythm-mode Slave channel
        ChanCat_Rhythm_Slave    = 8
    };

    int8_t      m_four_op_category[NUM_OF_CHANNELS * 2];
    // 1 = quad-master, 2 = quad-slave, 0 = regular
    // 3 = percussion BassDrum
    // 4 = percussion Snare
    // 5 = percussion Tom
    // 6 = percussion Crash cymbal
    // 7 = percussion Hihat
    // 8 = percussion slave

    //! index of operators pair, cached, needed by Touch()
    uint16_t    m_ins[NUM_OF_CHANNELS];
    //! value poked to B0, cached, needed by NoteOff)(
    uint8_t     m_keyBlockFNumCache[NUM_OF_CHANNELS];
};

#endif // GENERATOR_H
