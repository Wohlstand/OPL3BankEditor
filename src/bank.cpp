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

#include "bank.h"
#include <memory.h>

//! Typedef to unsigned char
typedef unsigned char uchar;

//! Typedef to signed character pointer
typedef char         *char_p;

//! Typedef to unsigned integer
typedef unsigned int uint;

FmBank::FmBank()
{
    reset();
}

FmBank::FmBank(const FmBank &fb)
{
    reset();
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    //int size = sizeof(Instrument)*128;
    //memcpy(Ins_Melodic,    fb.Ins_Melodic,    size);
    //memcpy(Ins_Percussion, fb.Ins_Percussion, size);
}

FmBank &FmBank::operator=(const FmBank &fb)
{
    if(this == &fb)
        return *this;

    reset();
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    return *this;
}

bool FmBank::operator==(const FmBank &fb)
{
    bool res = true;
    res &= (Ins_Melodic_box.size() == fb.Ins_Melodic_box.size());
    res &= (Ins_Percussion_box.size() == fb.Ins_Percussion_box.size());

    if(res)
    {
        int size = Ins_Melodic_box.size() * sizeof(Instrument);
        res &= (memcmp(Ins_Melodic,      fb.Ins_Melodic,    size) == 0);
        size = Ins_Percussion_box.size() * sizeof(Instrument);
        res &= (memcmp(Ins_Percussion,   fb.Ins_Percussion, size) == 0);
    }

    return res;
}

bool FmBank::operator!=(const FmBank &fb)
{
    return !this->operator==(fb);
}

void FmBank::reset()
{
    int insnum = 128;
    int size = sizeof(Instrument) * insnum;
    Ins_Melodic_box.resize(insnum);
    Ins_Percussion_box.resize(insnum);
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    memset(Ins_Melodic,    0, size);
    memset(Ins_Percussion, 0, size);
}

FmBank::Instrument FmBank::emptyInst()
{
    FmBank::Instrument inst;
    memset(&inst, 0, sizeof(FmBank::Instrument));
    return inst;
}

unsigned char FmBank::Instrument::getAVEKM(int OpID)
{
    uchar out = 0;
    out |= 0x80 & (uchar(OP[OpID].am) << 7);
    out |= 0x40 & (uchar(OP[OpID].vib) << 6);
    out |= 0x20 & (uchar(OP[OpID].eg) << 5);
    out |= 0x10 & (uchar(OP[OpID].ksr) << 4);
    out |= 0x0F &  uchar(OP[OpID].fmult);
    return out;
}

void FmBank::Instrument::setAVEKM(int OpID, unsigned char in)
{
    OP[OpID].am     = (in >> 7) & 0x01;
    OP[OpID].vib    = (in >> 6) & 0x01;
    OP[OpID].eg     = (in >> 5) & 0x01;
    OP[OpID].ksr    = (in >> 4) & 0x01;
    OP[OpID].fmult  = (in) & 0x0F;
}



unsigned char FmBank::Instrument::getKSLL(int OpID)
{
    uchar out = 0;
    out |= 0xC0 & (uchar(OP[OpID].ksl) << 6);
    out |= 0x3F & uchar(0x3F - OP[OpID].level);
    return out;
}

void FmBank::Instrument::setKSLL(int OpID, unsigned char in)
{
    OP[OpID].ksl   = (in >> 6) & 0x03;
    OP[OpID].level = 0x3F - (in & 0x3F);
}

unsigned char FmBank::Instrument::getKSL(int OpID)
{
    uchar out = 0;
    out |= 0xC0 & (uchar(OP[OpID].ksl) << 6);
    return out;
}

void FmBank::Instrument::setKSL(int OpID, unsigned char in)
{
    OP[OpID].ksl   = (in >> 6) & 0x03;
}

unsigned char FmBank::Instrument::getLevel(int OpID)
{
    uchar out = 0;
    out |= 0x3F & uchar(0x3F - OP[OpID].level);
    return out;
}

void FmBank::Instrument::setLevel(int OpID, unsigned char in)
{
    OP[OpID].level = 0x3F - (in & 0x3F);
}



unsigned char FmBank::Instrument::getAtDec(int OpID)
{
    uchar out = 0;
    out |= 0xF0 & uchar(OP[OpID].attack << 4);
    out |= 0x0F & OP[OpID].decay;
    return out;
}

void FmBank::Instrument::setAtDec(int OpID, unsigned char in)
{
    OP[OpID].attack = (in >> 4) & 0x0F;
    OP[OpID].decay  = (in)    & 0x0F;
}



unsigned char FmBank::Instrument::getSusRel(int OpID)
{
    uchar out = 0;
    out |= 0xF0 & (uchar(0x0F - OP[OpID].sustain) << 4);
    out |= 0x0F & OP[OpID].release;
    return out;
}

void FmBank::Instrument::setSusRel(int OpID, unsigned char in)
{
    OP[OpID].sustain = 0x0F - ((in >> 4) & 0x0F);
    OP[OpID].release = (in) & 0x0F;
}


unsigned char FmBank::Instrument::getWaveForm(int OpID)
{
    uchar out = 0;
    out |= 0x07 & OP[OpID].waveform;
    return out;
}

void FmBank::Instrument::setWaveForm(int OpID, unsigned char in)
{
    OP[OpID].waveform  = in & 0x07;
}



unsigned char FmBank::Instrument::getFBConn1()
{
    uchar out = 0;
    out |= uchar(connection1);
    out |= 0x0E & uchar(feedback1 << 1);
    return out;
}

void FmBank::Instrument::setFBConn1(unsigned char in)
{
    connection1 =  in & 0x01;
    feedback1   = (in >> 1) & 0x07;
}

unsigned char FmBank::Instrument::getFBConn2()
{
    uchar out = 0;
    out |= uchar(connection2);
    out |= 0x0E & uchar(feedback2 << 1);
    return out;
}

void FmBank::Instrument::setFBConn2(unsigned char in)
{
    connection2 =  in & 0x01;
    feedback2   = (in >> 1) & 0x07;
}


unsigned int FmBank::Instrument::getDataE862(int OpID)
{
    return (uint(OP[OpID].waveform) << 24)
           | (uint((0xF0 & (uchar(0x0F - OP[OpID].sustain) << 4))
                   | (0x0F & OP[OpID].release)) << 16)
           | (uint((0xF0 & uchar(OP[OpID].attack << 4))
                   | (0x0F & OP[OpID].decay)) << 8)
           | uint((0x80 & (uchar(OP[OpID].am) << 7))
                  | (0x40 & (uchar(OP[OpID].vib) << 6))
                  | (0x20 & (uchar(OP[OpID].eg) << 5))
                  | (0x10 & (uchar(OP[OpID].ksr) << 4))
                  | (0x0F &  uchar(OP[OpID].fmult)));
}

