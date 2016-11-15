/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "dbopl.h"
#include "../bank.h"
#include <stdint.h>

#include <QIODevice>
#include <QObject>

#define NUM_OF_CHANNELS         23
#define MAX_OPLGEN_BUFFER_SIZE  4096

struct OPL_Operator
{
    //! Operator properties
    uint32_t    modulator_E862, carrier_E862;
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

class Generator : public QIODevice
{
        Q_OBJECT

    public:
        Generator(uint32_t sampleRate, QObject *parent);
        ~Generator();

        void start();
        void stop();

        qint64 readData(char *data, qint64 maxlen);
        qint64 writeData(const char *data, qint64 len);
        qint64 bytesAvailable() const;

        void NoteOn(uint32_t c, double hertz);
        void NoteOff(uint32_t c);
        void Touch_Real(uint32_t c, uint32_t volume);
        void Touch(unsigned c, unsigned volume);
        void Patch(unsigned c, unsigned i);
        void Pan(unsigned c, unsigned value);
        void PlayNoteF(int noteID);
        void PlayDrum(uint8_t drum, int noteID);
        void switch4op(bool enabled);

    public slots:
        void Silence();
        void NoteOffAllChans();

        void PlayNote();
        void PlayMajorChord();
        void PlayMinorChord();
        void PlayAugmentedChord();
        void PlayDiminishedChord();
        void PlayMajor7Chord();
        void PlayMinor7Chord();

        void changePatch(FmBank::Instrument &instrument, bool isDrum = false);
        void changeNote(int32_t newnote);
        void changeDeepTremolo(bool enabled);
        void changeDeepVibrato(bool enabled);
        void changeAdLibPercussion(bool enabled);
    signals:
        void debugInfo(QString);

    private:
        int32_t     note;
        uint8_t     DeepTremoloMode;
        uint8_t     DeepVibratoMode;
        uint8_t     AdLibPercussionMode;
        uint8_t     testDrum;
        DBOPL::Handler chip;
        OPL_PatchSetup m_patch;

        uint8_t     m_regBD;

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
        uint8_t     m_pit[NUM_OF_CHANNELS];
};

#endif // GENERATOR_H
