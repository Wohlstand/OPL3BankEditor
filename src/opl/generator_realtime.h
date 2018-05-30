/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef GENERATOR_REALTIME_H
#define GENERATOR_REALTIME_H

#include "realtime/ring_buffer.h"
#include "../bank.h"
#include <QObject>
#include <QTimer>
#include <thread>
#include <memory>
#include <mutex>
#include <QMutex>
#include <system_error>
#include <stdint.h>
#if defined(_WIN32)
#include <windows.h>
#endif

class Generator;
struct GeneratorDebugInfo;

/**
   A control interface which drives a generator from a user interface.
 */
class IRealtimeControl : public QObject
{
    Q_OBJECT
public:
    IRealtimeControl(QObject *parent = nullptr);
    virtual ~IRealtimeControl() {}
    virtual void ctl_switchChip(int chipId) = 0;

public slots:
    void changeNote(int note) { m_note = note; }

    virtual void ctl_silence() = 0;
    virtual void ctl_noteOffAllChans() = 0;
    virtual void ctl_playNote() = 0;
    virtual void ctl_stopNote() = 0;
    virtual void ctl_pitchBend(int bend) = 0;
    virtual void ctl_hold(bool held) = 0;

    virtual void ctl_playChord(int chord) = 0;
    void ctl_playMajorChord();
    void ctl_playMinorChord();
    void ctl_playAugmentedChord();
    void ctl_playDiminishedChord();
    void ctl_playMajor7Chord();
    void ctl_playMinor7Chord();

    virtual void ctl_changePatch(FmBank::Instrument &instrument, bool isDrum = false) = 0;
    virtual void ctl_changeDeepVibrato(bool enabled) = 0;
    virtual void ctl_changeDeepTremolo(bool enabled) = 0;
    virtual void ctl_changeVolumeModel(int model) = 0;
    virtual void ctl_changeVolume(unsigned vol) = 0;

signals:
    void debugInfo(QString);

private slots:
    void debugInfoUpdate();

protected:
    virtual const GeneratorDebugInfo &generatorDebugInfo() const = 0;

protected:
    unsigned m_note = 0;
    QTimer *m_debugInfoTimer = nullptr;
};

/**
   A control interface which drives a generator from a MIDI interface.
 */
class IRealtimeMIDI
{
public:
    virtual ~IRealtimeMIDI() {}
    virtual void midi_event(const uint8_t *msg, unsigned msglen) = 0;
};

/**
   A realtime interface for the audio processing thread.
 */
class IRealtimeProcess
{
public:
    virtual ~IRealtimeProcess() {}
    virtual void rt_generate(int16_t *frames, unsigned nframes) = 0;
};

class RealtimeGenerator :
    public IRealtimeControl, public IRealtimeMIDI, public IRealtimeProcess
{
public:
    explicit RealtimeGenerator(const std::shared_ptr<Generator> &gen, QObject *parent = nullptr);
    ~RealtimeGenerator();

private:
    RealtimeGenerator(const RealtimeGenerator &);
    RealtimeGenerator &operator=(const RealtimeGenerator &);

public:
    /* Control */
    void ctl_switchChip(int chipId) override;
    void ctl_silence() override;
    void ctl_noteOffAllChans() override;
    void ctl_playNote() override;
    void ctl_stopNote() override;
    void ctl_pitchBend(int bend) override;
    void ctl_hold(bool held) override;
    void ctl_playChord(int chord) override;
    void ctl_changePatch(FmBank::Instrument &instrument, bool isDrum = false) override;
    void ctl_changeDeepVibrato(bool enabled) override;
    void ctl_changeDeepTremolo(bool enabled) override;
    void ctl_changeVolumeModel(int model) override;
    void ctl_changeVolume(unsigned vol) override;
    /* MIDI */
    void midi_event(const uint8_t *msg, unsigned msglen) override;
    /* Realtime */
    void rt_generate(int16_t *frames, unsigned nframes) override;

private:
    void rt_message_process(int tag, const uint8_t *data, unsigned len);
    void rt_midi_process(const uint8_t *data, unsigned len);

protected:
    const GeneratorDebugInfo &generatorDebugInfo() const override;

private:
    std::shared_ptr<Generator> m_gen;
    std::unique_ptr<Ring_Buffer> m_rb_ctl;
    std::unique_ptr<Ring_Buffer> m_rb_midi;
    std::unique_ptr<uint8_t[]> m_body;

    struct MidiChannelInfo
    {
        unsigned lastmrpn = 0;
        unsigned lastlrpn = 0;
        bool nrpn = false;
        unsigned bendsensemsb = 2;
        unsigned bendsenselsb = 0;
        unsigned volume = 100;
        unsigned expression = 127;
    };
    MidiChannelInfo m_midichan[16];

#if defined(ENABLE_WIN9X_OPL_PROXY)
    class QStdMutex
    {
    public:
        QStdMutex()
        {}
        ~QStdMutex() {}
        void lock() { m.lock(); }
        void unlock() { m.unlock(); }
        bool try_lock() { return m.tryLock(); }
    private:
        QMutex m;
    };
    QStdMutex m_generator_mutex;
    typedef QStdMutex mutex_type;
#elif !defined(_WIN32)
    std::mutex m_generator_mutex;
    typedef std::mutex mutex_type;
#else
    class WindowsMutex
    {
    public:
        WindowsMutex()
        {
            if (!(hMutex = CreateMutex(nullptr, false, nullptr)))
                throw std::system_error(GetLastError(), std::system_category());
        }
        ~WindowsMutex() { CloseHandle(hMutex); }
        void lock() { WaitForSingleObject(hMutex, INFINITE); }
        void unlock() { ReleaseMutex(hMutex); }
        bool try_lock() { return WaitForSingleObject(hMutex, 0) == WAIT_OBJECT_0; }
    private:
        HANDLE hMutex;
    };
    WindowsMutex m_generator_mutex;
    typedef WindowsMutex mutex_type;
#endif
};


#endif // GENERATOR_REALTIME_H
