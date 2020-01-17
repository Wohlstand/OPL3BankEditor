/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

//! Typedef to signed character pointer
typedef char         *char_p;

FmBank::FmBank()
{
    reset();
}

FmBank::FmBank(const FmBank &fb)
{
    reset();
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    Ins_Melodic         = Ins_Melodic_box.data();
    Ins_Percussion      = Ins_Percussion_box.data();
    Banks_Melodic       = fb.Banks_Melodic;
    Banks_Percussion    = fb.Banks_Percussion;
    deep_vibrato        = fb.deep_vibrato;
    deep_tremolo        = fb.deep_tremolo;
}

FmBank &FmBank::operator=(const FmBank &fb)
{
    if(this == &fb)
        return *this;

    reset();
    Ins_Melodic_box     = fb.Ins_Melodic_box;
    Ins_Percussion_box  = fb.Ins_Percussion_box;
    Ins_Melodic         = Ins_Melodic_box.data();
    Ins_Percussion      = Ins_Percussion_box.data();
    Banks_Melodic       = fb.Banks_Melodic;
    Banks_Percussion    = fb.Banks_Percussion;
    deep_vibrato        = fb.deep_vibrato;
    deep_tremolo        = fb.deep_tremolo;
    return *this;
}

bool FmBank::operator==(const FmBank &fb)
{
    bool res = true;
    res &= (deep_vibrato == fb.deep_vibrato);
    res &= (deep_tremolo == fb.deep_tremolo);
    res &= (Ins_Melodic_box.size() == fb.Ins_Melodic_box.size());
    res &= (Ins_Percussion_box.size() == fb.Ins_Percussion_box.size());
    res &= (Banks_Melodic.size() == fb.Banks_Melodic.size());
    res &= (Banks_Percussion.size() == fb.Banks_Percussion.size());
    if(res)
    {
        int size = Ins_Melodic_box.size() * static_cast<int>(sizeof(Instrument));
        res &= (memcmp(Ins_Melodic,      fb.Ins_Melodic,    static_cast<size_t>(size)) == 0);
        size = Ins_Percussion_box.size() * static_cast<int>(sizeof(Instrument));
        res &= (memcmp(Ins_Percussion,   fb.Ins_Percussion, static_cast<size_t>(size)) == 0);
        size = Banks_Melodic.size() * static_cast<int>(sizeof(MidiBank));
        res &= (memcmp(Banks_Melodic.data(),   fb.Banks_Melodic.data(), static_cast<size_t>(size)) == 0);
        size = Banks_Percussion.size() * static_cast<int>(sizeof(MidiBank));
        res &= (memcmp(Banks_Percussion.data(),   fb.Banks_Percussion.data(), static_cast<size_t>(size)) == 0);
    }
    return res;
}

bool FmBank::operator!=(const FmBank &fb)
{
    return !this->operator==(fb);
}

void FmBank::reset()
{
    size_t insnum = 128;
    size_t banksnum = insnum / 128;
    size_t size = sizeof(Instrument) * insnum;
    Ins_Melodic_box.resize(static_cast<int>(insnum));
    Ins_Percussion_box.resize(static_cast<int>(insnum));
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    Banks_Melodic.resize(static_cast<int>(banksnum));
    Banks_Percussion.resize(static_cast<int>(banksnum));
    memset(Ins_Melodic,    0, size);
    memset(Ins_Percussion, 0, size);
    size = sizeof(MidiBank) * banksnum;
    memset(Banks_Melodic.data(), 0, size);
    memset(Banks_Percussion.data(), 0, size);
    deep_vibrato = false;
    deep_tremolo = false;
}

void FmBank::reset(uint16_t melodic_banks, uint16_t percussion_banks)
{
    size_t insnum = 128;
    size_t size = sizeof(Instrument) * insnum;
    Ins_Melodic_box.resize(static_cast<int>(insnum * melodic_banks));
    Ins_Percussion_box.resize(static_cast<int>(insnum * percussion_banks));
    Ins_Melodic     = Ins_Melodic_box.data();
    Ins_Percussion  = Ins_Percussion_box.data();
    Banks_Melodic.resize(static_cast<int>(melodic_banks));
    Banks_Percussion.resize(static_cast<int>(percussion_banks));
    memset(Ins_Melodic,    0, size * melodic_banks);
    memset(Ins_Percussion, 0, size * percussion_banks);
    size = sizeof(MidiBank) * melodic_banks;
    memset(Banks_Melodic.data(), 0, size);
    size = sizeof(MidiBank) * percussion_banks;
    memset(Banks_Percussion.data(), 0, size);
    deep_vibrato = false;
    deep_tremolo = false;
}

void FmBank::autocreateMissingBanks()
{
    int melodic_banks = ((countMelodic() - 1) / 128 + 1);
    int percussion_banks = ((countDrums() - 1) / 128 + 1);
    size_t size = 0;
    if(Banks_Melodic.size() < melodic_banks)
    {
        size = (size_t)Banks_Melodic.size();
        Banks_Melodic.resize(melodic_banks);
        memset(Banks_Melodic.data() + size, 0, sizeof(MidiBank) * ((size_t)melodic_banks - size));
        for(int i = (int)size; i < Banks_Melodic.size(); i++)
        {
            int lsb = i % 256;
            int msb = (i >> 8) & 255;
            Banks_Melodic[i].lsb = (uint8_t)lsb;
            Banks_Melodic[i].msb = (uint8_t)msb;
        }
    }
    if(Banks_Percussion.size() < percussion_banks)
    {
        size = (size_t)Banks_Percussion.size();
        Banks_Percussion.resize(percussion_banks);
        memset(Banks_Percussion.data() + size, 0, sizeof(MidiBank) * ((size_t)percussion_banks - size));
        for(int i = (int)size; i < Banks_Percussion.size(); i++)
        {
            int lsb = i % 256;
            int msb = (i >> 8) & 255;
            Banks_Percussion[i].lsb = (uint8_t)lsb;
            Banks_Percussion[i].msb = (uint8_t)msb;
        }
    }
}

FmBank::Instrument FmBank::emptyInst()
{
    FmBank::Instrument inst;
    memset(&inst, 0, sizeof(FmBank::Instrument));
    return inst;
}

FmBank::Instrument FmBank::blankInst()
{
    FmBank::Instrument inst = emptyInst();
    inst.is_blank = true;
    return inst;
}

FmBank::MidiBank FmBank::emptyBank(uint16_t index)
{
    FmBank::MidiBank bank;
    memset(&bank, 0, sizeof(FmBank::MidiBank));
    bank.lsb = ((index >> 0) & 0xFF);
    bank.msb = ((index >> 8) & 0xFF);
    bank.name[0] = '\0';
    return bank;
}

bool FmBank::getBank(uint8_t msb, uint8_t lsb, bool percussive,
                     MidiBank **pBank, Instrument **pIns)
{
    Instrument *Ins = percussive ? Ins_Percussion : Ins_Melodic;
    // QVector<Instrument> &Ins_Box = percussive ? Ins_Percussion_box : Ins_Melodic_box;
    QVector<MidiBank> &Banks = percussive ? Banks_Percussion : Banks_Melodic;

    for(size_t index = 0, count = Banks.size(); index < count; ++index)
    {
        MidiBank &midiBank = Banks[index];
        if(midiBank.msb == msb && midiBank.lsb == lsb)
        {
            if(pBank)
                *pBank = &midiBank;
            if(pIns)
                *pIns = &Ins[128 * index];
            return true;
        }
    }

    if(pBank)
        *pBank = nullptr;
    if(pIns)
        *pIns = nullptr;

    return false;
}

bool FmBank::createBank(uint8_t msb, uint8_t lsb, bool percussive,
                        MidiBank **pBank, Instrument **pIns)
{
    if(getBank(msb, lsb, percussive, pBank, pIns))
        return false;

    Instrument *&Ins = percussive ? Ins_Percussion : Ins_Melodic;
    QVector<Instrument> &Ins_Box = percussive ? Ins_Percussion_box : Ins_Melodic_box;
    QVector<MidiBank> &Banks = percussive ? Banks_Percussion : Banks_Melodic;

    size_t index = Banks.size();
    Banks.push_back(MidiBank());
    MidiBank &midiBank = Banks.back();
    memset(midiBank.name, 0, sizeof(midiBank.name));
    midiBank.msb = msb;
    midiBank.lsb = lsb;

    Ins_Box.insert(Ins_Box.end(), 128, blankInst());
    Ins = Ins_Box.data();

    if(pBank)
        *pBank = &midiBank;
    if(pIns)
        *pIns = &Ins[128 * index];

    return true;
}

uint8_t FmBank::Instrument::getAVEKM(int OpID) const
{
    uint8_t out = 0;
    out |= 0x80 & (uint8_t(OP[OpID].am) << 7);
    out |= 0x40 & (uint8_t(OP[OpID].vib) << 6);
    out |= 0x20 & (uint8_t(OP[OpID].eg) << 5);
    out |= 0x10 & (uint8_t(OP[OpID].ksr) << 4);
    out |= 0x0F &  uint8_t(OP[OpID].fmult);
    return out;
}

void FmBank::Instrument::setAVEKM(int OpID, uint8_t in)
{
    OP[OpID].am     = (in >> 7) & 0x01;
    OP[OpID].vib    = (in >> 6) & 0x01;
    OP[OpID].eg     = (in >> 5) & 0x01;
    OP[OpID].ksr    = (in >> 4) & 0x01;
    OP[OpID].fmult  = (in) & 0x0F;
}



uint8_t FmBank::Instrument::getKSLL(int OpID) const
{
    uint8_t out = 0;
    out |= 0xC0 & (uint8_t(OP[OpID].ksl) << 6);
    out |= 0x3F & uint8_t(0x3F - OP[OpID].level);
    return out;
}

void FmBank::Instrument::setKSLL(int OpID, uint8_t in)
{
    OP[OpID].ksl   = (in >> 6) & 0x03;
    OP[OpID].level = 0x3F - (in & 0x3F);
}

uint8_t FmBank::Instrument::getKSL(int OpID) const
{
    uint8_t out = 0;
    out |= 0xC0 & (uint8_t(OP[OpID].ksl) << 6);
    return out;
}

void FmBank::Instrument::setKSL(int OpID, uint8_t in)
{
    OP[OpID].ksl   = (in >> 6) & 0x03;
}

uint8_t FmBank::Instrument::getLevel(int OpID) const
{
    uint8_t out = 0;
    out |= 0x3F & uint8_t(0x3F - OP[OpID].level);
    return out;
}

void FmBank::Instrument::setLevel(int OpID, uint8_t in)
{
    OP[OpID].level = 0x3F - (in & 0x3F);
}



uint8_t FmBank::Instrument::getAtDec(int OpID) const
{
    uint8_t out = 0;
    out |= 0xF0 & uint8_t(OP[OpID].attack << 4);
    out |= 0x0F & OP[OpID].decay;
    return out;
}

void FmBank::Instrument::setAtDec(int OpID, uint8_t in)
{
    OP[OpID].attack = (in >> 4) & 0x0F;
    OP[OpID].decay  = (in)    & 0x0F;
}



uint8_t FmBank::Instrument::getSusRel(int OpID) const
{
    uint8_t out = 0;
    out |= 0xF0 & (uint8_t(0x0F - OP[OpID].sustain) << 4);
    out |= 0x0F & OP[OpID].release;
    return out;
}

void FmBank::Instrument::setSusRel(int OpID, uint8_t in)
{
    OP[OpID].sustain = 0x0F - ((in >> 4) & 0x0F);
    OP[OpID].release = (in) & 0x0F;
}


uint8_t FmBank::Instrument::getWaveForm(int OpID) const
{
    uint8_t out = 0;
    out |= 0x07 & OP[OpID].waveform;
    return out;
}

void FmBank::Instrument::setWaveForm(int OpID, uint8_t in)
{
    OP[OpID].waveform  = in & 0x07;
}



uint8_t FmBank::Instrument::getFBConn1() const
{
    uint8_t out = 0;
    out |= uint8_t(connection1);
    out |= 0x0E & uint8_t(feedback1 << 1);
    return out;
}

void FmBank::Instrument::setFBConn1(uint8_t in)
{
    connection1 =  in & 0x01;
    feedback1   = (in >> 1) & 0x07;
}

uint8_t FmBank::Instrument::getFBConn2() const
{
    uint8_t out = 0;
    out |= uint8_t(connection2);
    out |= 0x0E & uint8_t(feedback2 << 1);
    return out;
}

void FmBank::Instrument::setFBConn2(uint8_t in)
{
    connection2 =  in & 0x01;
    feedback2   = (in >> 1) & 0x07;
}


uint32_t FmBank::Instrument::getDataE862(int OpID) const
{
    return (uint32_t(OP[OpID].waveform) << 24)
           | (uint32_t((0xF0 & (uint8_t(0x0F - OP[OpID].sustain) << 4))
                    | (0x0F & OP[OpID].release)) << 16)
           | (uint32_t((0xF0 & uint8_t(OP[OpID].attack << 4))
                    | (0x0F & OP[OpID].decay)) << 8)
           | uint32_t((0x80 & (uint8_t(OP[OpID].am) << 7))
                    | (0x40 & (uint8_t(OP[OpID].vib) << 6))
                    | (0x20 & (uint8_t(OP[OpID].eg) << 5))
                    | (0x10 & (uint8_t(OP[OpID].ksr) << 4))
                    | (0x0F &  uint8_t(OP[OpID].fmult)));
}


TmpBank::TmpBank(FmBank &bank, int minMelodic, int minPercusive)
{
    insMelodic = bank.Ins_Melodic;
    insPercussion = bank.Ins_Percussion;
    if(bank.Ins_Melodic_box.size() < minMelodic)
    {
        tmpMelodic = bank.Ins_Melodic_box;
        tmpMelodic.reserve(128 - tmpMelodic.size());
        while(tmpMelodic.size() < 128)
            tmpMelodic.push_back(FmBank::emptyInst());
        insMelodic = tmpMelodic.data();
    }
    if(bank.Ins_Melodic_box.size() > minMelodic)
    {
        tmpMelodic = bank.Ins_Melodic_box;
        tmpMelodic.resize(minMelodic);
        insMelodic = tmpMelodic.data();
    }

    if(bank.Ins_Percussion_box.size() < minPercusive)
    {
        tmpPercussion = bank.Ins_Percussion_box;
        tmpPercussion.reserve(128 - tmpPercussion.size());
        while(tmpPercussion.size() < 128)
            tmpPercussion.push_back(FmBank::emptyInst());
        insPercussion = tmpPercussion.data();
    }
    if(bank.Ins_Percussion_box.size() > minPercusive)
    {
        tmpPercussion = bank.Ins_Percussion_box;
        tmpPercussion.resize(minPercusive);
        insPercussion = tmpPercussion.data();
    }
}
