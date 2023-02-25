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

#ifndef METAPARAMETER_H
#define METAPARAMETER_H

#include "bank.h"

struct MetaParameter
{
    typedef int (Get)(const FmBank::Instrument &);

    MetaParameter() {}
    MetaParameter(const char *name, Get *get, int min, int max, unsigned flags)
        : name(name), get(get), min(min), max(max), flags(flags) {}

    const char *const name = nullptr;
    Get *const get = nullptr;
    int min = 0;
    int max = 0;
    const unsigned flags = 0;
};

enum MetaParameterFlag
{
    MP_None          = 0,
    MP_4OpOnly       = 8,
    MP_Pseudo4OpOnly = 16 | MP_4OpOnly,
    MP_Measure       = 32,
    MP_Operator1     = 4,  /* all operators set the 3rd flag bit */
    MP_Operator2     = (MP_Operator1 + 1),
    MP_Operator3     = (MP_Operator1 + 2),
    MP_Operator4     = (MP_Operator1 + 3),

    MP_OperatorMask = MP_Operator1|MP_Operator2|MP_Operator3|MP_Operator4
};

static const MetaParameter MP_instrument[] =
{
#define G(x) (+[](const FmBank::Instrument &ins) -> int { return (x); })

    {"4op", G(ins.en_4op), 0, 1, MP_None},
    {"ps4op", G(ins.en_pseudo4op), 0, 1, MP_4OpOnly},
    {"rhy", G(ins.rhythm_drum_type ? (ins.rhythm_drum_type - 5) : 0), 0, 5, MP_None},
    {"con1", G(ins.connection1), 0, 1, MP_None},
    {"fb1", G(ins.feedback1), 0, 7, MP_None},
    {"con2", G(ins.connection2), 0, 1, MP_4OpOnly},
    {"fb2", G(ins.feedback2), 0, 7, MP_4OpOnly},
#define OP(n, flags)                                                           \
    {"ar", G(ins.OP[n].attack), 0, 15, (flags)}, \
    {"dr", G(ins.OP[n].decay), 0, 15, (flags)}, \
    {"sl", G(ins.OP[n].sustain), 0, 15, (flags)}, \
    {"rr", G(ins.OP[n].release), 0, 15, (flags)}, \
    {"wf", G(ins.OP[n].waveform), 0, 7, (flags)}, \
    {"tl", G(ins.OP[n].level), 0, 63, (flags)}, \
    {"ksl", G(ins.OP[n].ksl), 0, 3, (flags)}, \
    {"mul", G(ins.OP[n].fmult), 0, 15, (flags)}, \
    {"am", G(ins.OP[n].am), 0, 1, (flags)}, \
    {"vib", G(ins.OP[n].vib), 0, 1, (flags)}, \
    {"eg", G(ins.OP[n].eg), 0, 1, (flags)}, \
    {"ksr", G(ins.OP[n].ksr), 0, 1, (flags)}
    OP(MODULATOR1, MP_Operator1),
    OP(CARRIER1, MP_Operator2),
    OP(MODULATOR2, MP_Operator3|MP_4OpOnly),
    OP(CARRIER2, MP_Operator4|MP_4OpOnly),
#undef OP
    {"note1", G(ins.note_offset1), -128, 127, MP_None},
    {"note2", G(ins.note_offset2), -128, 127, MP_Pseudo4OpOnly},
    {"vel", G(ins.velocity_offset), -128, 127, MP_None},
    {"dt2", G(ins.fine_tune), -128, 127, MP_Pseudo4OpOnly},
    {"pk", G(ins.percNoteNum), 0, 127, MP_None},
    {"kon", G(ins.ms_sound_kon), 0, 65535, MP_Measure},
    {"koff", G(ins.ms_sound_koff), 0, 65535, MP_Measure},

#undef G
};

#endif // METAPARAMETER_H
