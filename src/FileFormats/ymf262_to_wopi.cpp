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

#include "ymf262_to_wopi.h"
#include <cstdio>

RawYmf262ToWopi::RawYmf262ToWopi()
{
    m_insdata.reset(new InstrumentData);
    reset();
}

void RawYmf262ToWopi::reset()
{
    InstrumentData &insdata = *m_insdata;
    insdata.cache.clear();
    insdata.caughtInstruments.clear();

    for(unsigned i = 0; i < 9; ++i)
    {
        unsigned o = 6 * (i / 3) + (i % 3);
        m_channel[i].pair[0] = &m_operator[o];
        m_channel[i].pair[1] = &m_operator[o + 3];
        o += 18;
        m_channel[i + 9].pair[0] = &m_operator[o];
        m_channel[i + 9].pair[1] = &m_operator[o + 3];
    }

    for(unsigned i = 0; i < 18; ++i)
    {
        m_channel[i].cat = ChanCat_2op;
        m_channel[i].buddy = nullptr;
        m_channel[i].regA0 = 0;
        m_channel[i].regB0 = 0;
        m_channel[i].regC0 = 0;
    }

    for(unsigned i = 0; i < 36; ++i)
    {
        m_operator[i].reg20 = 0;
        m_operator[i].reg40 = 0;
        m_operator[i].reg60 = 0;
        m_operator[i].reg80 = 0;
        m_operator[i].regE0 = 0;
    }
}

void RawYmf262ToWopi::shareInstruments(RawYmf262ToWopi &other)
{
    m_insdata = other.m_insdata;
}

static unsigned operatorOfRegister(unsigned reg)
{
    unsigned opno;

    unsigned lo = reg & 0xff;
    switch(lo)
    {
    default:
        return ~0u;
    case 0x0: case 0x1: case 0x2: case 0x3: case 0x4: case 0x5:
        opno = lo;
        break;
    case 0x8: case 0x9: case 0xa: case 0xb: case 0xc: case 0xd:
        opno = lo - 2;
        break;
    case 0x10: case 0x11: case 0x12: case 0x13: case 0x14: case 0x15:
        opno = lo - 4;
        break;
    }

    if(opno != ~0u)
        opno += ((reg & 0x100) ? 18 : 0);

    return opno;
}

void RawYmf262ToWopi::passReg(uint16_t addr, uint8_t val)
{
    unsigned cs = addr & 0x100; // primary/secondary channel set
    unsigned reg = addr & 0xff;

    if(reg == 0x104)
    {
        // 4-op mask
        updateChannelRoles(val);
        return;
    }

    for(unsigned operatorReg : {0x20, 0x40, 0x60, 0x80, 0xE0})
    {
        if(reg >= operatorReg && reg < operatorReg + 18)
        {
            unsigned opno = operatorOfRegister((reg - operatorReg) | cs);
            if(opno != ~0u)
            {
                Operator &op = m_operator[opno];
                switch(operatorReg)
                {
                case 0x20: op.reg20 = val; break;
                case 0x40: op.reg40 = val; break;
                case 0x60: op.reg60 = val; break;
                case 0x80: op.reg80 = val; break;
                case 0xE0: op.regE0 = val; break;
                }
            }
            return;
        }
    }

    for(unsigned channelReg : {0xA0, 0xB0, 0xC0})
    {
        if(reg >= channelReg && reg < channelReg + 9)
        {
            unsigned chno = (reg - channelReg) + ((cs != 0) ? 9 : 0);
            Channel &ch = m_channel[chno];
            switch(channelReg)
            {
            case 0xA0: ch.regA0 = val; break;
            case 0xB0: ch.regB0 = val; break;
            case 0xC0: ch.regC0 = val; break;
            }
            return;
        }
    }
}

void RawYmf262ToWopi::doAnalyzeState()
{
    InstrumentData &insdata = *m_insdata;

    for(unsigned chno = 0; chno < 18; chno++)
    {
        const Channel &ch = m_channel[chno];
        bool keyOn = (ch.regB0 & 32) != 0;

        if(!keyOn)
            continue; //Skip if key is not pressed

        ChannelCategory cat = m_channel->cat;
        if(cat == ChanCat_4opSlave)
            continue;

        QByteArray insRaw; //Raw instrument
        FmBank::Instrument ins = FmBank::emptyInst();

        ins.en_4op = cat == ChanCat_4opMaster;

        Operator *ops[4];
        ops[MODULATOR1] = ch.pair[0];
        ops[CARRIER1] = ch.pair[1];
        if(ins.en_4op)
        {
            ops[MODULATOR2] = ch.buddy->pair[0];
            ops[CARRIER2] = ch.buddy->pair[1];
        }

        for (unsigned pairno = 0; pairno < (ins.en_4op ? 2 : 1); ++pairno)
        {
            for(unsigned i = 0; i < 2; ++i)
            {
                unsigned opno = 2 * pairno + i;
                Operator *src = ops[opno];

                ins.setAVEKM(opno, src->reg20);
                ins.setKSLL(opno, src->reg40);
                ins.setAtDec(opno, src->reg60);
                ins.setSusRel(opno, src->reg80);
                ins.setWaveForm(opno, src->regE0);

                insRaw.push_back((char)ins.getAVEKM(opno));
                insRaw.push_back((char)ins.getKSLL(opno));
                insRaw.push_back((char)ins.getAtDec(opno));
                insRaw.push_back((char)ins.getSusRel(opno));
                insRaw.push_back((char)ins.getWaveForm(opno));
            }
        }

        ins.setFBConn1(ch.regC0 & 15);
        insRaw.push_back((char)ins.getFBConn1());
        if(ins.en_4op)
        {
            ins.setFBConn2(ch.buddy->regC0 & 15);
            insRaw.push_back((char)ins.getFBConn2());
        }

        if(!insdata.cache.contains(insRaw))
        {
            std::snprintf(ins.name, 32,
                          "Ins %d, channel %u",
                          insdata.caughtInstruments.size(),
                          chno);
            insdata.caughtInstruments.push_back(ins);
            insdata.cache.insert(insRaw);
        }
    }
}

const QList<FmBank::Instrument> &RawYmf262ToWopi::caughtInstruments()
{
    return m_insdata->caughtInstruments;
}

void RawYmf262ToWopi::updateChannelRoles(uint8_t mask)
{
    for(unsigned pair = 0; pair < 6; ++pair)
    {
        bool fourOp = (mask & (1u << pair)) != 0;

        unsigned chno1st = (pair < 3) ? pair : (pair + 6);
        unsigned chno2nd = chno1st + 3;

        Channel *ch1st = &m_channel[chno1st];
        Channel *ch2nd = &m_channel[chno2nd];

        ch1st->cat = fourOp ? ChanCat_4opMaster : ChanCat_2op;
        ch1st->buddy = fourOp ? ch2nd : nullptr;
        ch2nd->cat = fourOp ? ChanCat_4opSlave : ChanCat_2op;
        ch2nd->buddy = fourOp ? ch1st : nullptr;
    }
}
