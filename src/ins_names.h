/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2022 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef INSTRUMENTNAMES_H
#define INSTRUMENTNAMES_H

#include <QString>
#include <cstdint>

#pragma pack(push, 1)
struct MidiProgram
{
    //! Kind of instrument. 'M' melodic 'P' percussive
    char kind;
    //! Bank identifier MSB
    unsigned char bankMsb;
    //! Bank identifier LSB. (if percussive, this is the program number)
    unsigned char bankLsb;
    //! Program number (if percussive, this is the key number)
    unsigned char program;
    //! Bank name
    const char *bankName;
    //! Patch name
    const char *patchName;
};
#pragma pack(pop)

enum MidiSpec
{
    //! General MIDI Level 1
    kMidiSpecGM1 = 1,
    //! General MIDI Level 2
    kMidiSpecGM2 = 2,
    //! Roland Sound Canvas
    kMidiSpecSC  = 4,
    //! Roland GS
    kMidiSpecGS  = 8,
    //! Yamaha XG Level 1, 2, 3
    kMidiSpecXG  = 16,
    //! No MIDI specification
    kMidiSpecNone = 0,
    //! Any MIDI specification
    kMidiSpecAny = 255,
};

struct MidiProgramId
{
    explicit MidiProgramId(uint32_t i = 0) : identifier(i)
    {
    }

    MidiProgramId(bool d, unsigned m, unsigned l, unsigned p) : identifier()
    {
        percussive = d;
        bankMsb = m;
        bankLsb = l;
        program = p;
    }

    union
    {
        uint32_t identifier;
        struct
        {
            uint32_t percussive : 1;
            uint32_t bankMsb : 7;
            uint32_t bankLsb : 7;
            uint32_t program : 7;
            uint32_t reserved : 1;
        };
    };
};

const MidiProgram *getMidiProgram(MidiProgramId id, unsigned spec, unsigned *specObtained = nullptr);
const MidiProgram *getFallbackProgram(MidiProgramId id, unsigned spec, unsigned *specObtained = nullptr);
const MidiProgram *getMidiBank(MidiProgramId id, unsigned spec, unsigned *specObtained = nullptr);

Q_DECL_DEPRECATED QString getMidiInsNameM(unsigned index);
Q_DECL_DEPRECATED QString getMidiInsNameP(unsigned index);

#endif // INSTRUMENTNAMES_H
