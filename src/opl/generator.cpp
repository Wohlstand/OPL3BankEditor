/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2025 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "generator.h"
#include <qendian.h>
#include <cmath>
#include <assert.h>
#include <QtDebug>

#include "models/opl_models.h"

#include "chips/nuked_opl3.h"
#include "chips/dosbox_opl3.h"
#include "chips/opal_opl3.h"
#include "chips/java_opl3.h"
#include "chips/esfmu_opl3.h"
#include "chips/mame_opl2.h"
#include "chips/ymf262_lle.h"
#include "chips/ym3812_lle.h"
#include "chips/vpc_opl3_emu.h"

#ifdef ENABLE_YMFM_EMULATOR
#include "chips/ymfm_opl2.h"
#include "chips/ymfm_opl3.h"
#endif

#ifdef ENABLE_HW_OPL_PROXY
#include "chips/win9x_opl_proxy.h"
#endif

#ifdef ENABLE_HW_OPL_SERIAL_PORT
#include "chips/opl_serial_port.h"
#endif

#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
      (byte & 0x80 ? '1' : '0'), \
      (byte & 0x40 ? '1' : '0'), \
      (byte & 0x20 ? '1' : '0'), \
      (byte & 0x10 ? '1' : '0'), \
      (byte & 0x08 ? '1' : '0'), \
      (byte & 0x04 ? '1' : '0'), \
      (byte & 0x02 ? '1' : '0'), \
      (byte & 0x01 ? '1' : '0')

#define BEND_COEFFICIENT 172.4387

#define OPL3_CHANNELS_MELODIC_BASE      0
#define OPL3_CHANNELS_RHYTHM_BASE       18

static const uint16_t g_Operators[NUM_OF_CHANNELS * 2] =
{
    // Channels 0-2
    0x000, 0x003, 0x001, 0x004, 0x002, 0x005, // operators  0, 3,  1, 4,  2, 5
    // Channels 3-5
    0x008, 0x00B, 0x009, 0x00C, 0x00A, 0x00D, // operators  6, 9,  7,10,  8,11
    // Channels 6-8
    0x010, 0x013, 0x011, 0x014, 0x012, 0x015, // operators 12,15, 13,16, 14,17
    // Same for second card
    0x100, 0x103, 0x101, 0x104, 0x102, 0x105, // operators 18,21, 19,22, 20,23
    0x108, 0x10B, 0x109, 0x10C, 0x10A, 0x10D, // operators 24,27, 25,28, 26,29
    0x110, 0x113, 0x111, 0x114, 0x112, 0x115, // operators 30,33, 31,34, 32,35
    // Channel 18
    0x010, 0x013,  // operators 12,15
    // Channel 19
    0xFFF, 0x014,  // operator 16
    // Channel 19
    0x012, 0xFFF,  // operator 14
    // Channel 19
    0xFFF, 0x015,  // operator 17
    // Channel 19
    0x011, 0xFFF
}; // operator 13

static const uint16_t g_Channels[NUM_OF_CHANNELS] =
{
    0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, // 0..8
    0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108, // 9..17 (secondary set)
    0x006, 0x007, 0x008, 0x008, 0x008
};
// <- hw percussions, <s>0xFFF = no support for pitch/pan</s>,
//      From AdLib MIDI manual: Hi-Hat and Cymbal are taking pitch value from
//          last pitch value set to the TomTom

static const uint16_t g_Channels_pan[NUM_OF_CHANNELS] =
{
    0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, // 0..8
    0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108, // 9..17 (secondary set)
    0x006, 0x007, 0x008, 0xFFF, 0xFFF
};

static const uint16_t g_Channels_fbconn[NUM_OF_CHANNELS] =
{
    0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, // 0..8
    0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108, // 9..17 (secondary set)
    0x006, 0xFFF, 0xFFF, 0xFFF, 0xFFF
};

/*
    In OPL3 mode:
         0    1    2    6    7    8     9   10   11    16   17   18
       op0  op1  op2 op12 op13 op14  op18 op19 op20  op30 op31 op32
       op3  op4  op5 op15 op16 op17  op21 op22 op23  op33 op34 op35
         3    4    5                   13   14   15
       op6  op7  op8                 op24 op25 op26
       op9 op10 op11                 op27 op28 op29
    Ports:
        +0   +1   +2  +10  +11  +12  +100 +101 +102  +110 +111 +112
        +3   +4   +5  +13  +14  +15  +103 +104 +105  +113 +114 +115
        +8   +9   +A                 +108 +109 +10A
        +B   +C   +D                 +10B +10C +10D

    Percussion:
      bassdrum = op(0): 0xBD bit 0x10, operators 12 (0x10) and 15 (0x13) / channels 6, 6b
      snare    = op(3): 0xBD bit 0x08, operators 16 (0x14)               / channels 7b
      tomtom   = op(4): 0xBD bit 0x04, operators 14 (0x12)               / channels 8
      cym      = op(5): 0xBD bit 0x02, operators 17 (0x17)               / channels 8b
      hihat    = op(2): 0xBD bit 0x01, operators 13 (0x11)               / channels 7


    In OPTi mode ("extended FM" in 82C924, 82C925, 82C931 chips):
         0   1   2    3    4    5    6    7     8    9   10   11   12   13   14   15   16   17
       op0 op4 op6 op10 op12 op16 op18 op22  op24 op28 op30 op34 op36 op38 op40 op42 op44 op46
       op1 op5 op7 op11 op13 op17 op19 op23  op25 op29 op31 op35 op37 op39 op41 op43 op45 op47
       op2     op8      op14      op20       op26      op32
       op3     op9      op15      op21       op27      op33    for a total of 6 quad + 12 dual
    Ports: ???
*/

#define USED_CHANNELS_2OP       18
#define USED_CHANNELS_2OP_PS4   9
#define USED_CHANNELS_4OP       6

// Channel ranges for OPL2
#define USED_CHANNELS_2OP_OPL2       9
#define USED_CHANNELS_2OP_PS4_OPL2   4


//! Regular 2-operator channels map
static const uint16_t g_channels2Map_2op[USED_CHANNELS_2OP] =
{
    0,  1,  2,  3,  4,  5,  6,  7,  8,
    9,  10, 11, 12, 13, 14, 15, 16, 17
};

//! Pseudo 4-operators 2-operator channels map 1
static const uint16_t g_channelsMap1_p4op[USED_CHANNELS_2OP_PS4] = {0, 2, 4, 6, 8, 10, 12, 14, 16};
//! Pseudo 4-operators 2-operator channels map 1
static const uint16_t g_channelsMap2_p4op[USED_CHANNELS_2OP_PS4] = {1, 3, 5, 7, 9, 11, 13, 15, 17};

//! 4-operator channels map 1
static const uint16_t g_channelsMap1_4op[USED_CHANNELS_4OP] = {0,  1,  2,  9,  10, 11};
//! 4-operator channels map 1
static const uint16_t g_channelsMap2_4op[USED_CHANNELS_4OP] = {3,  4,  5,  12, 13, 14};



QString GeneratorDebugInfo::toStr()
{
    return QObject::tr(
        "Channels:\n"
        "2-op: %1, Ps-4op: %2\n"
        "4-op: %3")
        .arg(this->chan2op)
        .arg(this->chanPs4op)
        .arg(this->chan4op);
}

Generator::Generator(uint32_t sampleRate, OPL_Chips initialChip)
{
    m_rate = sampleRate;
    note = 60;
    m_patch =
    {
        //    ,---------+-------- Wave select settings
        //    | ,-------+-+------ Sustain/release rates
        //    | | ,-----+-+-+---- Attack/decay rates
        //    | | | ,---+-+-+-+-- AM/VIB/EG/KSR/Multiple bits
        //    | | | |   | | | |
        //    | | | |   | | | |      ,----+-- KSL/attenuation settings
        //    | | | |   | | | |      |    |                  ,----- Feedback/connection bits
        //    | | | |   | | | |      |    |                  |    +- Fine tuning
        {
            { 0x104C060, 0x10455B1, 0x60, 0xB1, 0x51, 0x80, 0x4, +12 },
            { 0x10490A0, 0x1045531, 0xA0, 0x31, 0x52, 0x80, 0x6, +12 },
        },
        0,
        OPL_PatchSetup::Flag_Pseudo4op,
        -0.125000 // Fine tuning
    };
    m_regBD = 0;
    memset(m_regC0, 0x30, NUM_OF_CHANNELS);
    memset(m_ins, 0, sizeof(uint16_t) * NUM_OF_CHANNELS);
    memset(m_keyBlockFNumCache, 0, sizeof(uint8_t) * NUM_OF_CHANNELS);
    memset(m_four_op_category, 0, NUM_OF_CHANNELS * 2);

    m_getFreq = &oplModel_genericFreq;
    m_getVolume = &oplModel_genericVolume;

    m_4op_last_state = true;
    deepTremoloMode   = 0;
    deepVibratoMode   = 0;
    rythmModePercussionMode = 0;
    testDrum = 0;//Note ON/OFF of one of legacy percussion channels

    switchChip(initialChip);

    //Send the null patch to initialize the OPL stuff
    changePatch(FmBank::emptyInst(), false);
    m_isInstrumentLoaded = false;//Reset the flag to false as no instruments loaded
}

Generator::~Generator()
{}

void Generator::OPLChipDelete::operator()(OPLChipBase *x)
{
#ifdef ENABLE_HW_OPL_PROXY
    if(x == &Generator::oplProxy())
        return;
#endif
#ifdef ENABLE_HW_OPL_SERIAL_PORT
    if(x == &Generator::serialPortOpl())
        return;
#endif
    delete x;
}

void Generator::initChip()
{
    static const uint16_t data[] =
    {
        0x004, 96, 0x004, 128,          // Pulse timer
        0x105,  0, 0x105, 1,  0x105, 0, // Pulse OPL3 enable
        0x001, 32, 0x105, 1             // Enable wave, OPL3 extensions
    };

    static const uint16_t data_opl2[] =
    {
        0x004, 96, 0x004, 128,          // Pulse timer
        0x001, 32                       // Enable wave
    };
    uint32_t maxChans = 18;

    m_chipType = chip->chipType();

    if(m_chipType == OPLChipBase::CHIPTYPE_OPL2)
        maxChans = 9;

    chip->setChipId(0);
    chip->setRate(m_rate);

    initFourOpCats();

    for(uint32_t a = 0; a < maxChans; ++a)
        WriteReg(0xB0 + g_Channels[a], 0x00);

    if(m_chipType == OPLChipBase::CHIPTYPE_OPL3)
    {
        for(size_t a = 0; a < 14; a += 2)
            WriteReg(data[a], static_cast<uint8_t>(data[a + 1]));
    }
    else
    {
        for(size_t a = 0; a < 6; a += 2)
            WriteReg(data_opl2[a], static_cast<uint8_t>(data_opl2[a + 1]));
    }

    updateRegBD();
    switch4op(m_4op_last_state, false);
    Silence();
    updateChannelManager();
}

#ifdef ENABLE_HW_OPL_PROXY
Win9x_OPL_Proxy &Generator::oplProxy()
{
    static Win9x_OPL_Proxy proxy;
    return proxy;
}
#endif

#ifdef ENABLE_HW_OPL_SERIAL_PORT
OPL_SerialPort &Generator::serialPortOpl()
{
    static OPL_SerialPort serial;
    return serial;
}
#endif

void Generator::switchChip(Generator::OPL_Chips chipId)
{
    switch(chipId)
    {
#ifdef ENABLE_HW_OPL_PROXY
    case CHIP_Win9xProxy:
        oplProxy().startChip();
        chip.reset(&oplProxy());
        break;
#endif
#ifdef ENABLE_YMFM_EMULATOR
    case CHIP_YmFmOPL2:
        chip.reset(new YmFmOPL2());
        break;
    case CHIP_YmFm:
        chip.reset(new YmFmOPL3());
        break;
#endif
#ifdef ENABLE_HW_OPL_SERIAL_PORT
    case CHIP_SerialPort:
        chip.reset(&serialPortOpl());
        break;
#endif
    case CHIP_YMF262LLE:
        chip.reset(new Ymf262LLEOPL3());
        break;
    case CHIP_YM3812LLE:
        chip.reset(new Ym3812LLEOPL2());
        break;
    case CHIP_DosBox:
        chip.reset(new DosBoxOPL3());
        break;
    default:
    case CHIP_Nuked:
        chip.reset(new NukedOPL3());
        break;
    case CHIP_Opal:
        chip.reset(new OpalOPL3());
        break;
    case CHIP_Java:
        chip.reset(new JavaOPL3());
        break;
    case CHIP_EsFMu:
        chip.reset(new ESFMuOPL3());
        break;
    case CHIP_MameOPL2:
        chip.reset(new MameOPL2());
        break;
    case CHIP_VPC_OPL3:
        chip.reset(new VpcOPL3());
        break;
    }

    initChip();
}

void Generator::WriteReg(uint16_t address, uint8_t byte)
{
    chip->writeReg(address, byte);
}

void Generator::initFourOpCats()
{
    uint32_t p = 0;

    memset(&m_four_op_category, 0, sizeof(m_four_op_category));

    for(uint32_t b = 0; b < 18; ++b, ++p)
    {
        if(m_chipType == OPLChipBase::CHIPTYPE_OPL2 && b >= 9)
            m_four_op_category[p] = ChanCat_None;
        else
            m_four_op_category[p] = ChanCat_Regular;

        if(rythmModePercussionMode && b >= 6 && b < 9)
            m_four_op_category[p] = ChanCat_Rhythm_Secondary;
    }

    if(rythmModePercussionMode == 0)
    {
        for(uint32_t b = 0; b < 5; ++b)
            m_four_op_category[p++] = ChanCat_Rhythm_Secondary;
    }
    else
    {
        for(uint32_t b = 0; b < 5; ++b)
            m_four_op_category[p++] = (ChanCat_Rhythm_Bass + b);
    }
}

void Generator::NoteOff(uint32_t c)
{
    uint8_t cc = static_cast<uint8_t>(c % 23);

    if(cc >= 18)
    {
        m_regBD &= ~(0x10 >> (cc - 18));
        WriteReg(0xBD, m_regBD);
        return;
    }
    else if(m_chipType == OPLChipBase::CHIPTYPE_OPL2 && cc >= USED_CHANNELS_2OP_OPL2)
        return;

    WriteReg(0xB0 + g_Channels[cc], m_keyBlockFNumCache[c] & 0xDF);
}

void Generator::NoteOn(uint32_t c1, uint32_t c2, double tone, bool voice2ps4op) // Hertz range: 0..131071
{
    uint32_t cc1 = c1 % 23, cc2 = c2 % 23, mul_offset = 0;
    uint16_t ftone, chn = g_Channels[cc1];

    ftone = m_getFreq(tone, &mul_offset);

    if(cc1 < OPL3_CHANNELS_RHYTHM_BASE)
    {
        ftone += 0x2000u; /* Key-ON [KON] */

        const bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
        const size_t opsCount = natural_4op ? 4 : 2;
        const uint16_t op_addr[4] =
        {
            g_Operators[cc1 * 2 + 0], g_Operators[cc1 * 2 + 1],
            g_Operators[cc2 * 2 + 0], g_Operators[cc2 * 2 + 1]
        };
        const uint8_t ops[4] =
        {
            m_patch.OPS[voice2ps4op ? 1 : 0].modulator_20,
            m_patch.OPS[voice2ps4op ? 1 : 0].carrier_20,
            m_patch.OPS[1].modulator_20,
            m_patch.OPS[1].carrier_20
        };

        for(size_t op = 0; op < opsCount; op++)
        {
            if(op_addr[op] == 0xFFF)
                continue;
            if(mul_offset > 0)
            {
                uint32_t dt  = ops[op] & 0xF0;
                uint32_t mul = ops[op] & 0x0F;
                if((mul + mul_offset) > 0x0F)
                {
                    mul_offset = 0;
                    mul = 0x0F;
                }
                WriteReg(0x20 + op_addr[op],  uint8_t(dt | (mul + mul_offset)) & 0xFF);
            }
            else
            {
                WriteReg(0x20 + op_addr[op],  ops[op] & 0xFF);
            }
        }
    }

    if(chn != 0xFFF)
    {
        WriteReg(0xA0 + chn, (ftone & 0xFF));
        WriteReg(0xB0 + chn, (ftone >> 8));
        m_keyBlockFNumCache[c1] = static_cast<uint8_t>(ftone >> 8);
    }

    if(cc1 >= OPL3_CHANNELS_RHYTHM_BASE)
    {
        m_regBD |= (0x10 >> (cc1 - OPL3_CHANNELS_RHYTHM_BASE));
        WriteReg(0x0BD, m_regBD);
        //x |= 0x800; // for test
    }
}

void Generator::touchNote(uint32_t c,
                          uint32_t velocity,
                          uint8_t ccvolume,
                          uint8_t ccexpr,
                          uint32_t brightness, bool isDrum)
{
    uint16_t /*card = c/23,*/ cc = c % 23;
    uint16_t i = m_ins[c],
            o1 = g_Operators[cc * 2 + 0],
            o2 = g_Operators[cc * 2 + 1];
    uint16_t srcMod = m_patch.OPS[i].modulator_40,
             srcCar = m_patch.OPS[i].carrier_40;
    uint_fast32_t kslMod = srcMod & 0xC0;
    uint_fast32_t kslCar = srcCar & 0xC0;
    OPLVolume_t vol =
    {
        (uint_fast8_t)(velocity & 0x7F),
        (uint_fast8_t)(ccvolume & 0x7F),
        (uint_fast8_t)(ccexpr & 0x7F),
        0x7F,
        OPLVoice_MODE_2op_AM, // 2-op AM
        (uint_fast8_t)m_patch.OPS[i].feedconn,
        (uint_fast8_t)(srcMod & 0x3F),
        (uint_fast8_t)(srcCar & 0x3F),
        0, // Do Modulator
        0, // Do Carrier
        (unsigned int)isDrum
    };

    uint_fast8_t modulator;
    uint_fast8_t carrier;

    static const bool do_ops[10][2] =
    {
        { false, true },  /* 2 op FM */
        { true,  true },  /* 2 op AM */
        { false, false }, /* 4 op FM-FM ops 1&2 */
        { true,  false }, /* 4 op AM-FM ops 1&2 */
        { false, true  }, /* 4 op FM-AM ops 1&2 */
        { true,  false }, /* 4 op AM-AM ops 1&2 */
        { false, true  }, /* 4 op FM-FM ops 3&4 */
        { false, true  }, /* 4 op AM-FM ops 3&4 */
        { false, true  }, /* 4 op FM-AM ops 3&4 */
        { true,  true  }  /* 4 op AM-AM ops 3&4 */
    };

    if(m_chipType == OPLChipBase::CHIPTYPE_OPL2 && m_four_op_category[c] == ChanCat_None)
        return; // Do nothing

    if(m_four_op_category[c] == ChanCat_Regular ||
       m_four_op_category[c] == ChanCat_Rhythm_Bass)
    {
        vol.voiceMode = m_patch.OPS[i].feedconn & 1; // 2-op FM or 2-op AM
    }
    else if(m_four_op_category[c] == ChanCat_4op_First ||
            m_four_op_category[c] == ChanCat_4op_Second)
    {
        uint32_t i0, i1;
        if(m_four_op_category[c] == ChanCat_4op_First)
        {
            i0 = i;
            i1 = m_ins[c + 3];
            vol.voiceMode = OPLVoice_MODE_4op_1_2_FM_FM; // 4-op xx-xx ops 1&2
        }
        else
        {
            assert(c >= 3);
            i0 = m_ins[c - 3];
            i1 = i;
            vol.voiceMode = OPLVoice_MODE_4op_3_4_FM_FM; // 4-op xx-xx ops 3&4
        }

        vol.voiceMode += (m_patch.OPS[i0].feedconn & 1) + (m_patch.OPS[i1].feedconn & 1) * 2;
    }

    vol.doMod = do_ops[vol.voiceMode][0];
    vol.doCar = do_ops[vol.voiceMode][1];


    // ------ Mix volumes and compute average ------
    m_getVolume(&vol);

    if(brightness != 127)
    {
        brightness = oplModels_xgBrightnessToOPL(brightness);

        if(!vol.doMod)
            vol.tlMod = 63 - brightness + (brightness * vol.tlMod) / 63;

        if(!vol.doCar)
            vol.tlCar = 63 - brightness + (brightness * vol.tlCar) / 63;
    }

    modulator = (kslMod & 0xC0) | (vol.tlMod & 63);
    carrier = (kslCar & 0xC0) | (vol.tlCar & 63);

    if(o1 != 0xFFF)
        WriteReg(0x40 + o1, static_cast<uint8_t>(modulator));

    if(o2 != 0xFFF)
        WriteReg(0x40 + o2, static_cast<uint8_t>(carrier));
}

void Generator::Patch(uint32_t c, uint32_t i)
{
    uint32_t cc = c % 23;
    static const uint16_t data[4] = {0x20, 0x60, 0x80, 0xE0};
    m_ins[c] = static_cast<uint16_t>(i);
    uint16_t o1 = g_Operators[cc * 2 + 0],
             o2 = g_Operators[cc * 2 + 1];
    uint32_t x = m_patch.OPS[i].modulator_E862, y = m_patch.OPS[i].carrier_E862;
    uint8_t fbconn = 0;
    uint16_t fbconn_reg = 0x00;

    if(m_four_op_category[c] == ChanCat_Regular && (m_volumeScale == VOLUME_DMX || m_volumeScale == VOLUME_DMX_FIXED))
    {
        // Also write dummy volume value
        if(o1 != 0xFFF)
            WriteReg(0x40 + o1, m_patch.OPS[i].modulator_40);

        if(o2 != 0xFFF)
            WriteReg(0x40 + o2, m_patch.OPS[i].carrier_40);
    }


    for(uint32_t a = 0; a < 4; ++a, x >>= 8, y >>= 8)
    {
        if(o1 != 0xFFF)
            WriteReg(data[a] + o1, x & 0xFF);
        if(o2 != 0xFFF)
            WriteReg(data[a] + o2, y & 0xFF);
    }

    if(g_Channels_fbconn[cc] != 0xFFF)
    {
        fbconn |= m_patch.OPS[i].feedconn;
        fbconn_reg = 0xC0 + g_Channels_fbconn[cc];
    }

    if(m_chipType != OPLChipBase::CHIPTYPE_OPL2 && g_Channels_pan[cc] != 0xFFF)
    {
        fbconn |= (m_regC0[c] & 0x30);
        if(!fbconn_reg)
            fbconn_reg = 0xC0 + g_Channels_pan[cc];
    }

    if(fbconn_reg != 0x00)
        WriteReg(fbconn_reg, fbconn);
}

void Generator::Pan(uint32_t c, uint32_t value)
{
    uint8_t cc = c % 23;

    if(g_Channels_pan[cc] != 0xFFF)
    {
        m_regC0[cc] = value;
        if(m_chipType != OPLChipBase::CHIPTYPE_OPL2)
            WriteReg(0xC0 + g_Channels_pan[cc], static_cast<uint8_t>(m_patch.OPS[m_ins[c]].feedconn | value));
    }
}

void Generator::PlayNoteF(int noteID, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    bool replace;
    int ch = m_noteManager.noteOn(noteID, volume, ccvolume, ccexpr, &replace);

    if(replace)
    {
        //if it replaces an old note, shut up the old one first
        //this lets the sustain take over with a fresh envelope
        bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
        bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
        if(natural_4op)
        {
            NoteOff(g_channelsMap1_4op[ch]);
        }
        else
        {
            if(pseudo_4op)
            {
                NoteOff(g_channelsMap1_p4op[ch]);
                NoteOff(g_channelsMap2_p4op[ch]);
            }
            else
                NoteOff(g_channels2Map_2op[ch]);
        }
    }

    PlayNoteCh(ch);
}

void Generator::PlayNoteCh(int ch)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    int tone;
    const NotesManager::Note &channel = m_noteManager.channel(ch);

    if(m_patch.tone)
    {
        tone = m_patch.tone;
        if(tone > 128)
            tone -= 128;
    }
    else
    {
        tone = channel.note;
    }

    uint16_t i[2] = { 0, 1 };
    bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
    bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
    uint16_t  adlchannel[2] = { 0, 0 };

    if(!natural_4op || pseudo_4op)
    {
        if(pseudo_4op)
        {
            adlchannel[0] = g_channelsMap1_p4op[ch];
            adlchannel[1] = g_channelsMap2_p4op[ch];
            m_debug.chanPs4op = ch;
        }
        else
        {
            adlchannel[0] = g_channels2Map_2op[ch];
            adlchannel[1] = g_channels2Map_2op[ch];
            m_debug.chan2op = ch;
        }
    }
    else if(natural_4op)
    {
        adlchannel[0] = g_channelsMap1_4op[ch];
        adlchannel[1] = g_channelsMap2_4op[ch];
        m_debug.chan4op = ch;
    }

    m_ins[adlchannel[0]] = i[0];
    m_ins[adlchannel[1]] = i[1];
    double bend = 0.0;
    double phase = 0.0;
    Patch(adlchannel[0], i[0]);
    Pan(adlchannel[0], 0x30);

    if(natural_4op)
    {
        Patch(adlchannel[1], i[1]);
        Pan(adlchannel[1], 0x30);
    }

    touchNote(adlchannel[0], channel.volume, channel.ccvolume, channel.ccexpr);
    if(natural_4op)
        touchNote(adlchannel[1], channel.volume, channel.ccvolume, channel.ccexpr);

    bend  = m_bend + m_patch.OPS[i[0]].finetune;
    NoteOn(adlchannel[0], adlchannel[1], (tone + bend + phase));

    if(pseudo_4op)
    {
        Patch(adlchannel[1], i[1]);
        Pan(adlchannel[1], 0x30);
        touchNote(adlchannel[1], channel.volume, channel.ccvolume, channel.ccexpr);
        bend  = m_bend + m_patch.OPS[i[1]].finetune + m_patch.voice2_fine_tune;
        NoteOn(adlchannel[1], 0, (tone + bend + phase), true);
    }
}

void Generator::StopNoteF(int noteID)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    if(rythmModePercussionMode)
    {
        //TODO: Turn each working RythmMode drum individually!
        //updateRegBD();
        return;
    }

    int ch = m_noteManager.findNoteOffChannel(noteID);
    if(ch == -1)
        return;

    StopNoteCh(ch);
}

void Generator::StopNoteCh(int ch)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    if(m_hold)
    {
        m_noteManager.hold(ch, true);  // stop later after hold is over
        return;
    }

    m_noteManager.channelOff(ch);

    bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
    bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
    if(natural_4op)
    {
        NoteOff(g_channelsMap1_4op[ch]);
    }
    else
    {
        if(pseudo_4op)
        {
            NoteOff(g_channelsMap1_p4op[ch]);
            NoteOff(g_channelsMap2_p4op[ch]);
        }
        else
            NoteOff(g_channels2Map_2op[ch]);
    }
}

void Generator::PlayDrum(uint8_t drum, int noteID)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    int tone = noteID;

    if(m_patch.tone)
    {
        tone = m_patch.tone;
        if(tone > 128)
            tone -= 128;
    }

    uint32_t adlchannel = OPL3_CHANNELS_RHYTHM_BASE + drum;
    // Patch(adlchannel, 0);
    Pan(adlchannel, 0x30);
    touchNote(adlchannel, 127, 127, 127, 127, true);
    double bend = 0.0;
    double phase = 0.0;
    bend  = 0.0 + m_patch.OPS[0].finetune;
    NoteOn(adlchannel, 0, (tone + bend + phase));
}

void Generator::switch4op(bool enabled, bool patchCleanUp)
{
    m_4op_last_state = enabled;
    //Shut up currently playing stuff
    for(uint32_t b = 0; b < NUM_OF_CHANNELS; ++b)
    {
        if(m_chipType == OPLChipBase::CHIPTYPE_OPL2 && (b == 9))
            b = 18;
        NoteOff(b);
        touchNote(b, 0, 0, 0);
    }

    updateRegBD();
    initFourOpCats();

    if(enabled && (m_chipType == OPLChipBase::CHIPTYPE_OPL3))
    {
        //Enable 4-operators mode
        WriteReg(0x104, 0xFF);
        uint32_t fours = 6;
        uint32_t nextfour = 0;

        for(uint32_t a = 0; a < fours; ++a)
        {
            m_four_op_category[nextfour    ] = ChanCat_4op_First;
            m_four_op_category[nextfour + 3] = ChanCat_4op_Second;

            switch(a % 6)
            {
            case 0:
            case 1:
                nextfour += 1;
                break;
            case 2:
                nextfour += 9 - 2;
                break;
            case 3:
            case 4:
                nextfour += 1;
                break;
            case 5:
                nextfour += 23 - 9 - 2;
                break;
            }
        }
    }
    else
    {
        if(m_chipType == OPLChipBase::CHIPTYPE_OPL3)
            WriteReg(0x104, 0x00);//Disable 4-operators mode
    }

    if(patchCleanUp)
    {
        //Reset patch settings
        memset(&m_patch, 0, sizeof(OPL_PatchSetup));
        m_patch.OPS[0].modulator_20   = 0x00;
        m_patch.OPS[0].modulator_40   = 0x3F;
        m_patch.OPS[0].modulator_E862 = 0x00FFFF00;
        m_patch.OPS[1].carrier_20     = 0x00;
        m_patch.OPS[1].carrier_40     = 0x3F;
        m_patch.OPS[1].carrier_E862   = 0x00FFFF00;
    }

    //Clear all operator registers from crap left from previous patches
    for(uint32_t b = 0; b < NUM_OF_CHANNELS; ++b)
    {
        if(m_chipType == OPLChipBase::CHIPTYPE_OPL2 && (b == 9))
            b = 18;
        Patch(b, 0);
        Pan(b, (rythmModePercussionMode == 0) ? 0x00 : 0x30);
        touchNote(b, 0, 0, 0);
    }
}

void Generator::Silence()
{
    //Shutup!
    for(uint32_t c = 0; c < NUM_OF_CHANNELS; ++c)
    {
        NoteOff(c);
        touchNote(c, 0, 0, 0);
    }

    m_noteManager.clearNotes();
}

void Generator::NoteOffAllChans()
{
    if(rythmModePercussionMode)
    {
        updateRegBD();
        return;
    }

    if(m_hold)
    {
        // mark all channels held for later key-off
        int channels = m_noteManager.channelCount();
        for(int ch = 0; ch < channels; ++ch)
        {
            const NotesManager::Note &channel = m_noteManager.channel(ch);
            if(channel.note != -1)
                m_noteManager.hold(ch, true);
        }
        return;
    }

    //bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
    bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
    if(natural_4op)
    {
        for(uint32_t c = 0; c < USED_CHANNELS_4OP; ++c)
            NoteOff(g_channelsMap1_4op[c]);
    }
    else
    {
        for(uint32_t c = 0; c < USED_CHANNELS_2OP; ++c)
            NoteOff(g_channels2Map_2op[c]);
    }

    m_noteManager.clearNotes();
}



void Generator::PlayNote(uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    if(rythmModePercussionMode)
        PlayDrum(testDrum, note);
    else
        PlayNoteF(note, volume, ccvolume, ccexpr);
}

void Generator::PlayMajorChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 4, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::PlayMinorChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 3, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::PlayAugmentedChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 4, volume, ccvolume, ccexpr);
    PlayNoteF(n - 4, volume, ccvolume, ccexpr);
}

void Generator::PlayDiminishedChord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 3, volume, ccvolume, ccexpr);
    PlayNoteF(n - 6, volume, ccvolume, ccexpr);
}

void Generator::PlayMajor7Chord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n - 2, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 4, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::PlayMinor7Chord(int n, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr)
{
    PlayNoteF(n - 12, volume, ccvolume, ccexpr);
    PlayNoteF(n - 2, volume, ccvolume, ccexpr);
    PlayNoteF(n, volume, ccvolume, ccexpr);
    PlayNoteF(n + 3, volume, ccvolume, ccexpr);
    PlayNoteF(n - 5, volume, ccvolume, ccexpr);
}

void Generator::StopNote()
{
    if(rythmModePercussionMode)
        NoteOffAllChans();
    else
        StopNoteF(note);
}

void Generator::PitchBend(int bend)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    m_bend = bend * m_bendsense;

    int channels = m_noteManager.channelCount();
    for(int ch = 0; ch < channels; ++ch)
    {
        const NotesManager::Note &channel = m_noteManager.channel(ch);
        if(channel.note != -1)
            PlayNoteCh(ch);  // updates frequency
    }
}

void Generator::PitchBendSensitivity(int cents)
{
    m_bendsense = cents * (1e-2 / 8192);
}

void Generator::Hold(bool held)
{
    if(!m_isInstrumentLoaded)
        return;//Deny playing notes without instrument loaded

    if (m_hold == held)
        return;
    m_hold = held;

    if (!held)
    {
        // key-off all held notes now
        int channels = m_noteManager.channelCount();
        for(int ch = 0; ch < channels; ++ch)
        {
            const NotesManager::Note &channel = m_noteManager.channel(ch);
            if(channel.note != -1 && channel.held)
                StopNoteCh(ch);
        }
    }
}

void Generator::changePatch(const FmBank::Instrument &instrument, bool isDrum)
{
    //Shutup everything
    Silence();
    m_bend = 0.0;
    m_bendsense = 2.0 / 8192;
    //m_hold = false;
    bool isRhythmMode = isDrum && (instrument.rhythm_drum_type >= 6);

    changeRhythmMode(isRhythmMode);
    switch4op(instrument.en_4op && !instrument.en_pseudo4op && (instrument.rhythm_drum_type == 0));

    m_patch.OPS[0].modulator_E862   = instrument.getDataE862(MODULATOR1);
    m_patch.OPS[0].modulator_20     = instrument.getAVEKM(MODULATOR1);
    m_patch.OPS[0].modulator_40     = instrument.getKSLL(MODULATOR1);
    m_patch.OPS[0].carrier_E862     = instrument.getDataE862(CARRIER1);
    m_patch.OPS[0].carrier_20       = instrument.getAVEKM(CARRIER1);
    m_patch.OPS[0].carrier_40       = instrument.getKSLL(CARRIER1);
    m_patch.OPS[0].feedconn         = instrument.getFBConn1();

    m_patch.OPS[1].modulator_E862   = instrument.getDataE862(MODULATOR2);
    m_patch.OPS[1].modulator_20     = instrument.getAVEKM(MODULATOR2);
    m_patch.OPS[1].modulator_40     = instrument.getKSLL(MODULATOR2);
    m_patch.OPS[1].carrier_E862     = instrument.getDataE862(CARRIER2);
    m_patch.OPS[1].carrier_20       = instrument.getAVEKM(CARRIER2);
    m_patch.OPS[1].carrier_40       = instrument.getKSLL(CARRIER2);
    m_patch.OPS[1].feedconn         = instrument.getFBConn2();

    m_patch.flags   = 0;
    m_patch.tone    = 0;
    m_patch.voice2_fine_tune = 0.0;

    if(isDrum || instrument.is_fixed_note)
        m_patch.tone = instrument.percNoteNum;

    if(isRhythmMode)// Rhythm-mode percussion instrument
    {
        testDrum = instrument.rhythm_drum_type - 6;
        Patch(OPL3_CHANNELS_RHYTHM_BASE + testDrum, 0);
    }
    else // Melodic or Generic percussion instrument
    {
        if(instrument.en_4op && instrument.en_pseudo4op)
        {
            m_patch.voice2_fine_tune = (double)((((int)instrument.fine_tune + 128) >> 1) - 64) / 32.0;
            m_patch.OPS[0].finetune = static_cast<int8_t>(instrument.note_offset1);
            m_patch.OPS[1].finetune = static_cast<int8_t>(instrument.note_offset2);
        }
        else
        {
            m_patch.OPS[0].finetune = static_cast<int8_t>(instrument.note_offset1);
            m_patch.OPS[1].finetune = static_cast<int8_t>(instrument.note_offset1);
        }

        if(instrument.en_4op)
        {
            if(instrument.en_pseudo4op)
                m_patch.flags |= OPL_PatchSetup::Flag_Pseudo4op;
            else
                m_patch.flags |= OPL_PatchSetup::Flag_True4op;
        }
        updateChannelManager();
    }

    m_isInstrumentLoaded = true;//Mark instrument as loaded
}

void Generator::changeNote(int newnote)
{
    note = int32_t(newnote);
}

void Generator::changeDeepTremolo(bool enabled)
{
    deepTremoloMode   = uint8_t(enabled);
    updateRegBD();
}

void Generator::changeDeepVibrato(bool enabled)
{
    deepVibratoMode   = uint8_t(enabled);
    updateRegBD();
}

void Generator::changeVolumeModel(int volmodel)
{
    m_volumeScale = volmodel;

    switch(m_volumeScale)
    {
    default:
    case VOLUME_Generic:
        m_getFreq = &oplModel_genericFreq;
        m_getVolume = &oplModel_genericVolume;
        break;

    case VOLUME_CMF:
        m_getFreq = &oplModel_genericFreq;
        m_getVolume = &oplModel_nativeVolume;
        break;

    case VOLUME_DMX:
        m_getFreq = &oplModel_dmxFreq;
        m_getVolume = &oplModel_dmxOrigVolume;
        break;

    case VOLUME_APOGEE:
        m_getFreq = &oplModel_apogeeFreq;
        m_getVolume = &oplModel_apogeeOrigVolume;
        break;

    case VOLUME_9X:
        m_getFreq = &oplModel_9xFreq;
        m_getVolume = &oplModel_9xSB16Volume;
        break;

    case VOLUME_DMX_FIXED:
        m_getFreq = &oplModel_dmxFreq;
        m_getVolume = &oplModel_dmxFixedVolume;
        break;

    case VOLUME_APOGEE_FIXED:
        m_getFreq = &oplModel_apogeeFreq;
        m_getVolume = &oplModel_apogeeFixedVolume;
        break;

    case VOLUME_AIL:
        m_getFreq = &oplModel_ailFreq;
        m_getVolume = &oplModel_ailVolume;
        break;

    case VOLUME_9X_GENERIC_FM:
        m_getFreq = &oplModel_9xFreq;
        m_getVolume = &oplModel_9xGenericVolume;
        break;

    case VOLUME_HMI:
        m_getFreq = &oplModel_hmiFreq;
        m_getVolume = &oplModel_sosNewVolume;
        break;

    case VOLUME_HMI_OLD:
        m_getFreq = &oplModel_hmiFreq;
        m_getVolume = &oplModel_sosOldVolume;
        break;

    case VOLUME_MS_ADLIB:
        m_getFreq = &oplModel_msAdLibFreq;
        m_getVolume = &oplModel_msAdLibVolume;
        break;

    case VOLUME_IMF_CREATOR:
        m_getFreq = &oplModel_hmiFreq;
        m_getVolume = &oplModel_dmxFixedVolume;
        break;

    case VOLUME_OCONNELL:
        m_getFreq = &oplModel_OConnellFreq;
        m_getVolume = &oplModel_OConnellVolume;
        break;
    }
}

void Generator::changeRhythmMode(bool enabled)
{
    rythmModePercussionMode = uint8_t(enabled);
    updateRegBD();
}

void Generator::updateRegBD()
{
    m_regBD = (deepTremoloMode * 0x80) + (deepVibratoMode * 0x40) + (rythmModePercussionMode * 0x20);
    WriteReg(0x0BD, m_regBD);
}

void Generator::updateChannelManager()
{
    int chan2ops = USED_CHANNELS_2OP;
    int chanPs4ops = USED_CHANNELS_2OP_PS4;
    bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
    bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;

    if(m_chipType == OPLChipBase::CHIPTYPE_OPL2)
    {
        chan2ops = USED_CHANNELS_2OP_OPL2;
        chanPs4ops = USED_CHANNELS_2OP_PS4_OPL2;
    }

    if(pseudo_4op)
        m_noteManager.allocateChannels(chanPs4ops);
    else if(natural_4op)
        m_noteManager.allocateChannels(USED_CHANNELS_4OP);
    else
        m_noteManager.allocateChannels(chan2ops);
}

void Generator::setChanAllocMode(int mode)
{
    m_noteManager.setChanAllocMode(mode);
}

void Generator::generate(int16_t *frames, unsigned nframes)
{
    chip->generate(frames, nframes);
    // Gain by default
    for(size_t i = 0; i < nframes * 2; ++i)
        frames[i] *= m_gain;
}

void Generator::generate(float* frames, unsigned int nframes)
{
    int32_t out[2];

    chip->nativePreGenerate();

    for(unsigned int i = 0; i < nframes; ++i)
    {
        chip->resampledGenerate(out);
        *frames++ = (out[0] / 32767.0f) * m_gain;
        *frames++ = (out[1] / 32767.0f) * m_gain;
    }

    chip->nativePostGenerate();
}

void Generator::setGain(float gain)
{
    m_gain = gain;
}

float Generator::getGain() const
{
    return m_gain;
}

Generator::NotesManager::NotesManager()
{
    channels.reserve(USED_CHANNELS_2OP);
}

Generator::NotesManager::~NotesManager()
{}

void Generator::NotesManager::allocateChannels(int count)
{
    channels.clear();
    channels.resize(count);
    cycle = 0;
}

uint8_t Generator::NotesManager::noteOn(int note, uint32_t volume, uint8_t ccvolume, uint8_t ccexpr, bool *r)
{
    uint8_t beganAt = cycle;
    uint8_t chan = 0;

    // Increase age of all working notes;
    for(Note &ch : channels)
    {
        if(note >= 0)
            ch.age++;
    }

    bool replace = true;

    do
    {
        switch(alg)
        {
        default:
        case ALG_CYCLE:
            chan = cycle++;
            // Rotate cycles
            if(cycle == channels.size())
                cycle = 0;
            break;

        case ALG_RELEASED:
        {
            int age = 0;
            int found = -1;

            // Find first released
            for(chan = 0; chan < channels.size(); ++chan)
            {
                if(channels[chan].note == -1)
                {
                    cycle = chan;
                    found = chan;
                    break;
                }
            }

            // Find oldest
            if(found < 0)
            {
                for(chan = 0; chan < channels.size(); ++chan)
                {
                    if(channels[chan].age > age)
                    {
                        channels[chan].age = age;
                        found = chan;
                    }
                }

                if(found >= 0)
                    chan = found;
            }

            break;
        }
        }

        if(channels[chan].note == -1)
        {
            channels[chan].note = note;
            channels[chan].volume = volume;
            channels[chan].ccvolume = ccvolume;
            channels[chan].ccexpr = ccexpr;
            channels[chan].held = false;
            channels[chan].age = 0;
            replace = false;
            break;
        }

        if (cycle == beganAt) // If no free channels found
        {
            int age = -1;
            int oldest = -1;
            // Find oldest note
            for(uint8_t c = 0; c < channels.size(); c++)
            {
                if((channels[c].note >= 0) && ((age == -1) || (channels[c].age > age)))
                {
                    oldest = c;
                    age = channels[c].age;
                }
            }

            if(age >= 0)
            {
                chan = (uint8_t)oldest;
                channels[chan].note = note;
                channels[chan].volume = volume;
                channels[chan].ccvolume = ccvolume;
                channels[chan].ccexpr = ccexpr;
                channels[chan].held = false;
                channels[chan].age = 0;
            }
            break;
        }
    } while(1);

    if(r)
        *r = replace;

    return chan;
}

int8_t Generator::NotesManager::noteOff(int note)
{
    int8_t chan = findNoteOffChannel(note);
    if(chan != -1)
        channelOff(chan);
    return chan;
}

void Generator::NotesManager::channelOff(int ch)
{
    channels[ch].note = -1;
}

int8_t Generator::NotesManager::findNoteOffChannel(int note)
{
    // find the first active note not in held state (delayed noteoff)
    for(uint8_t chan = 0; chan < channels.size(); chan++)
    {
        if(channels[chan].note == note && !channels[chan].held)
            return (int8_t)chan;
    }
    return -1;
}

void Generator::NotesManager::hold(int ch, bool h)
{
    channels[ch].held = h;
}

void Generator::NotesManager::clearNotes()
{
    for(uint8_t chan = 0; chan < channels.size(); chan++)
        channels[chan].note = -1;
}
