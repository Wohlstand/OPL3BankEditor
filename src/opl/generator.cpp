/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2018 Vitaly Novichkov <admin@wohlnet.ru>
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
#include <QtDebug>

#include "chips/nuked_opl3.h"
#include "chips/dosbox_opl3.h"

#ifdef ENABLE_OPL_PROXY
#include <QSysInfo>
#include <QMessageBox>
extern "C"
{
    typedef void (_stdcall *opl_poke)(uint16_t index, uint16_t value);
    typedef void (_stdcall *opl_init)(void);
    typedef void (_stdcall *opl_unInit)(void);
}
static opl_poke     chip_oplPoke = NULL;
static opl_init     chip_oplInit = NULL;
static opl_unInit   chip_oplUninit = NULL;
static HINSTANCE    chip_lib = NULL;

template<class FunkPtr>
void initOplFunction(FunkPtr &ptr, const char *procName)
{
    ptr = (FunkPtr)GetProcAddress(chip_lib, procName);
    if(!ptr)
    {
        QMessageBox::warning(NULL,
                             "liboplproxy.dll error",
                             QString("Oops... I have failed to load %1 function:\n"
                                     "Error %2\n"
                                     "Continuing without FM sound.")
                                    .arg(procName)
                                    .arg(GetLastError()));
    }
}

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

static const uint16_t Operators[NUM_OF_CHANNELS * 2] =
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
    0x014, 0xFFF,  // operator 16
    // Channel 19
    0x012, 0xFFF,  // operator 14
    // Channel 19
    0x015, 0xFFF,  // operator 17
    // Channel 19
    0x011, 0xFFF
}; // operator 13

static const uint16_t Channels[NUM_OF_CHANNELS] =
{
    0x000, 0x001, 0x002, 0x003, 0x004, 0x005, 0x006, 0x007, 0x008, // 0..8
    0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107, 0x108, // 9..17 (secondary set)
    0x006, 0x007, 0x008, 0x008, 0x008
};
// <- hw percussions, <s>0xFFF = no support for pitch/pan</s>,
//      From AdLib MIDI manual: Hi-Hat and Cymbal are taking pitch value from
//          last pitch value set to the TomTom

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

//! Regular 2-operator channels map
static const uint16_t channels[USED_CHANNELS_2OP] =
{
    0,  1,  2,  3,  4,  5,  6,  7,  8,
    9,  10, 11, 12, 13, 14, 15, 16, 17
};

//! Pseudo 4-operators 2-operator channels map 1
static const uint16_t channels1[USED_CHANNELS_2OP_PS4] = {0, 2, 4, 6, 8, 10, 12, 14, 16};
//! Pseudo 4-operators 2-operator channels map 1
static const uint16_t channels2[USED_CHANNELS_2OP_PS4] = {1, 3, 5, 7, 9, 11, 13, 15, 17};

//! 4-operator channels map 1
static const uint16_t channels1_4op[USED_CHANNELS_4OP] = {0,  1,  2,  9,  10, 11};
//! 4-operator channels map 1
static const uint16_t channels2_4op[USED_CHANNELS_4OP] = {3,  4,  5,  12, 13, 14};

Generator::Generator(uint32_t sampleRate, OPL_Chips initialChip,
                     QObject *parent)
    :   QIODevice(parent)
{
    #ifdef ENABLE_OPL_EMULATOR
    m_rate = sampleRate;
    #else
    Q_UNUSED(sampleRate);
    #endif
    note = 60;
    m_patch =
    {
        //    ,---------+-------- Wave select settings
        //    | ,-------+-+------ Sustain/release rates
        //    | | ,-----+-+-+---- Attack/decay rates
        //    | | | ,---+-+-+-+-- AM/VIB/EG/KSR/Multiple bits
        //    | | | |   | | | |
        //    | | | |   | | | |    ,----+-- KSL/attenuation settings
        //    | | | |   | | | |    |    |    ,----- Feedback/connection bits
        //    | | | |   | | | |    |    |    |    +- Fine tuning
        {
            { 0x104C060, 0x10455B1, 0x51, 0x80, 0x4, +12 },
            { 0x10490A0, 0x1045531, 0x52, 0x80, 0x6, +12 },
        },
        0,
        OPL_PatchSetup::Flag_Pseudo4op,
        -0.125000 // Fine tuning
    };
    m_regBD = 0;
    memset(m_ins, 0, sizeof(uint16_t)*NUM_OF_CHANNELS);
    memset(m_pit, 0, sizeof(uint8_t)*NUM_OF_CHANNELS);
    memset(m_four_op_category, 0, NUM_OF_CHANNELS * 2);

    uint32_t p = 0;
    for(uint32_t b = 0; b < 18; ++b)
        m_four_op_category[p++] = 0;
    for(uint32_t b = 0; b < 5; ++b)
        m_four_op_category[p++] = 8;

    m_4op_last_state = true;
    deepTremoloMode   = 0;
    deepVibratoMode   = 0;
    rythmModePercussionMode = 0;
    testDrum = 0;//Note ON/OFF of one of legacy percussion channels

    switchChip(initialChip);
}

Generator::~Generator()
{}

void Generator::initChip()
{
    static const uint16_t data[] =
    {
        0x004, 96, 0x004, 128,          // Pulse timer
        0x105,  0, 0x105, 1,  0x105, 0, // Pulse OPL3 enable
        0x001, 32, 0x105, 1             // Enable wave, OPL3 extensions
    };

    #ifdef ENABLE_OPL_PROXY
    QSysInfo::WinVersion wver = QSysInfo::windowsVersion();
    bool m_enableProxy =    (wver == QSysInfo::WV_98) ||
                            (wver == QSysInfo::WV_Me);
    if(m_enableProxy && !chip_lib)
    {
        chip_lib = LoadLibraryA("liboplproxy.dll");
        if(!chip_lib)
            QMessageBox::warning(NULL, "liboplproxy.dll error", "Can't load liboplproxy.dll library");
        else
        {
            initOplFunction(chip_oplInit,   "_chipInit@0");
            initOplFunction(chip_oplPoke,   "_chipPoke@8");
            initOplFunction(chip_oplUninit, "_chipUnInit@0");
            if(chip_oplInit)
                chip_oplInit();
        }
    }
    #endif

    #ifdef ENABLE_OPL_EMULATOR
    chip->setRate(m_rate);
    #endif

    for(uint32_t a = 0; a < 18; ++a)
        WriteReg(0xB0 + Channels[a], 0x00);

    for(size_t a = 0; a < sizeof(data) / sizeof(*data); a += 2)
        WriteReg(data[a], static_cast<uint8_t>(data[a + 1]));

    updateRegBD();
    switch4op(m_4op_last_state, false);
    Silence();
}

void Generator::switchChip(Generator::OPL_Chips chipId)
{
    #ifdef ENABLE_OPL_EMULATOR
    std::lock_guard<std::mutex> g(chip_mutex);
    Q_UNUSED(g);
    switch(chipId)
    {
    case CHIP_DosBox:
        chip.reset(new DosBoxOPL3());
        break;
    case CHIP_Nuked:
        chip.reset(new NukedOPL3());
        break;
    }
    #else
    Q_UNUSED(chipId);
    #endif

    initChip();
}

void Generator::WriteReg(uint16_t address, uint8_t byte)
{
    #ifdef ENABLE_OPL_PROXY
    if(chip_oplPoke)
        chip_oplPoke(address, byte);
    #endif

    #ifdef ENABLE_OPL_EMULATOR
    chip->writeReg(address, byte);
    #endif

    #if !defined(ENABLE_OPL_EMULATOR) && !defined(ENABLE_OPL_PROXY)
    Q_UNUSED(address);
    Q_UNUSED(byte);
    #endif
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

    WriteReg(0xB0 + Channels[cc], m_pit[c] & 0xDF);
}

void Generator::NoteOn(uint32_t c, double hertz) // Hertz range: 0..131071
{
    uint16_t cc = c % 23;
    uint16_t x = 0x2000;

    if(hertz < 0 || hertz > 131071) // Avoid infinite loop
        return;

    while(hertz >= 1023.5)
    {
        hertz /= 2.0;    // Calculate octave
        x += 0x400;
    }

    x += static_cast<uint32_t>(hertz + 0.5);
    uint16_t chn = Channels[cc];

    if(cc >= 18)
        x &= ~0x2000u;

    WriteReg(0xA0 + chn, x & 0xFF);
    WriteReg(0xB0 + chn, m_pit[c] = static_cast<uint8_t>(x >> 8));

    if(cc >= 18)
    {
        m_regBD |= (0x10 >> (cc - 18));
        WriteReg(0x0BD, m_regBD);
        //x |= 0x800; // for test
    }
}

void Generator::Touch_Real(uint32_t c, uint32_t volume)
{
    if(volume > 63)
        volume = 63;

    uint16_t /*card = c/23,*/ cc = c % 23;
    uint16_t i = m_ins[c], o1 = Operators[cc * 2], o2 = Operators[cc * 2 + 1];
    uint16_t x = m_patch.OPS[i].modulator_40,
             y = m_patch.OPS[i].carrier_40;
    bool do_modulator;
    bool do_carrier;
    uint32_t mode = 1; // 2-op AM

    if(m_four_op_category[c] == 0 || m_four_op_category[c] == 3)
    {
        mode = m_patch.OPS[i].feedconn & 1; // 2-op FM or 2-op AM
    }
    else if(m_four_op_category[c] == 1 || m_four_op_category[c] == 2)
    {
        uint32_t i0, i1;
        if(m_four_op_category[c] == 1)
        {
            i0 = i;
            i1 = m_ins[c + 3];
            mode = 2; // 4-op xx-xx ops 1&2
        }
        else
        {
            i0 = m_ins[c - 3];
            i1 = i;
            mode = 6; // 4-op xx-xx ops 3&4
        }

        mode += (m_patch.OPS[i0].feedconn & 1) + (m_patch.OPS[i1].feedconn & 1) * 2;
    }

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
    do_modulator = do_ops[ mode ][ 0 ];
    do_carrier   = do_ops[ mode ][ 1 ];
    WriteReg(0x40 + o1, static_cast<Bit8u>(do_modulator ? (x | 63) - volume + volume * (x & 63) / 63 : x));

    if(o2 != 0xFFF)
        WriteReg(0x40 + o2, static_cast<Bit8u>(do_carrier   ? (y | 63) - volume + volume * (y & 63) / 63 : y));

    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void Generator::Touch(uint32_t c, uint32_t volume) // Volume maxes at 127*127*127
{
    // The formula below: SOLVE(V=127^3 * 2^( (A-63.49999) / 8), A)
    Touch_Real(c, static_cast<uint32_t>(volume > 8725  ? std::log(volume) * 11.541561 + (0.5 - 104.22845) : 0));
    // The incorrect formula below: SOLVE(V=127^3 * (2^(A/63)-1), A)
    //Touch_Real(c, volume>11210 ? 91.61112 * std::log(4.8819E-7*volume + 1.0)+0.5 : 0);
}

void Generator::Patch(uint32_t c, uint32_t i)
{
    uint32_t cc = c % 23;
    static const uint16_t data[4] = {0x20, 0x60, 0x80, 0xE0};
    m_ins[c] = static_cast<uint16_t>(i);
    uint16_t o1 = Operators[cc * 2 + 0],
             o2 = Operators[cc * 2 + 1];
    uint32_t x = m_patch.OPS[i].modulator_E862, y = m_patch.OPS[i].carrier_E862;

    for(uint32_t a = 0; a < 4; ++a)
    {
        WriteReg(data[a] + o1, x & 0xFF);
        x >>= 8;
        if(o2 != 0xFFF)
            WriteReg(data[a] + o2, y & 0xFF);
        y >>= 8;
    }
}

void Generator::Pan(uint32_t c, uint32_t value)
{
    uint8_t cc = c % 23;
    if(Channels[cc] != 0xFFF)
        WriteReg(0xC0 + Channels[cc], static_cast<uint8_t>(m_patch.OPS[m_ins[c]].feedconn | value));
}

void Generator::PlayNoteF(int noteID)
{
    static int chan2op   = 0;
    static int chanPs4op = 0;
    static int chan4op = 0;
    static struct DebugInfo
    {
        int chan2op;
        int chanPs4op;
        int chan4op;
        QString toStr()
        {
            return QString("Channels:\n"
                           "2-op: %1, Ps-4op: %2\n"
                           "4-op: %3")
                   .arg(this->chan2op)
                   .arg(this->chanPs4op)
                   .arg(this->chan4op);
        }
    } _debug { -1, -1, -1};
    int tone = noteID;

    if(m_patch.tone)
    {
        tone = m_patch.tone;
        if(tone > 128)
            tone -= 128;
    }

    uint16_t i[2] = { 0, 1 };
    bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
    bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
    uint16_t  adlchannel[2] = { 0, 0 };

    if(!natural_4op || pseudo_4op)
    {
        if(pseudo_4op)
        {
            adlchannel[0] = channels1[chanPs4op];
            adlchannel[1] = channels2[chanPs4op];
            /* Rotating channels to have nicer poliphony on key spam */
            _debug.chanPs4op = chanPs4op++;

            if(chanPs4op > (USED_CHANNELS_2OP_PS4 - 1))
                chanPs4op = 0;
        }
        else
        {
            adlchannel[0] = channels[chan2op];
            adlchannel[1] = channels[chan2op];
            /* Rotating channels to have nicer poliphony on key spam */
            _debug.chan2op = chan2op++;

            if(chan2op > (USED_CHANNELS_2OP - 1))
                chan2op = 0;
        }
    }
    else if(natural_4op)
    {
        adlchannel[0] = channels1_4op[chan4op];
        adlchannel[1] = channels2_4op[chan4op];
        /* Rotating channels to have nicer poliphony on key spam */
        _debug.chan4op = chan4op++;

        if(chan4op > (USED_CHANNELS_4OP - 1))
            chan4op = 0;
    }

    emit debugInfo(_debug.toStr());
    m_ins[adlchannel[0]] = i[0];
    m_ins[adlchannel[1]] = i[1];
    double bend = 0.0;
    double phase = 0.0;
    Patch(adlchannel[0], i[0]);

    if(pseudo_4op || natural_4op)
        Patch(adlchannel[1], i[1]);

    Pan(adlchannel[0], 0x30);

    if(pseudo_4op || natural_4op)
        Pan(adlchannel[1], 0x30);

    Touch_Real(adlchannel[0], 63);

    if(pseudo_4op || natural_4op)
        Touch_Real(adlchannel[1], 63);

    bend  = 0.0 + m_patch.OPS[i[0]].finetune;
    NoteOn(adlchannel[0], BEND_COEFFICIENT * std::exp(0.057762265 * (tone + bend + phase)));

    if(pseudo_4op)
    {
        bend  = 0.0 + m_patch.OPS[i[1]].finetune + m_patch.voice2_fine_tune;
        NoteOn(adlchannel[1], BEND_COEFFICIENT * std::exp(0.057762265 * (tone + bend + phase)));
    }
}

void Generator::PlayDrum(uint8_t drum, int noteID)
{
    int tone = noteID;

    if(m_patch.tone)
    {
        tone = m_patch.tone;
        if(tone > 128)
            tone -= 128;
    }

    uint32_t adlchannel = 18 + drum;
    Patch(adlchannel, 0);
    Pan(adlchannel, 0x30);
    Touch_Real(adlchannel, 63);
    double bend = 0.0;
    double phase = 0.0;
    bend  = 0.0 + m_patch.OPS[0].finetune;
    NoteOn(adlchannel, BEND_COEFFICIENT * std::exp(0.057762265 * (tone + bend + phase)));
}

void Generator::switch4op(bool enabled, bool patchCleanUp)
{
    m_4op_last_state = enabled;
    //Shut up currently playing stuff
    for(uint32_t b = 0; b < NUM_OF_CHANNELS; ++b)
    {
        NoteOff(b);
        Touch_Real(b, 0);
    }

    memset(&m_four_op_category, 0, sizeof(m_four_op_category));
    uint32_t p = 0;

    for(uint32_t b = 0; b < 18; ++b) m_four_op_category[p++] = 0;
    for(uint32_t b = 0; b < 5; ++b)  m_four_op_category[p++] = 8;

    // Mark all channels that are reserved for four-operator function
    if(rythmModePercussionMode != 0)
    {
        //for(uint32_t a = 0; a < NumCards; ++a) {}
        for(uint32_t b = 0; b < 5; ++b) m_four_op_category[18 + b] = static_cast<char>(b + 3);
        for(uint32_t b = 0; b < 3; ++b) m_four_op_category[6  + b] = 8;
    }

    if(enabled)
    {
        //Enable 4-operators mode
        WriteReg(0x104, 0xFF);
        uint32_t fours = 6;
        uint32_t nextfour = 0;

        for(uint32_t a = 0; a < fours; ++a)
        {
            m_four_op_category[nextfour    ] = 1;
            m_four_op_category[nextfour + 3] = 2;

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
        //Disable 4-operators mode
        WriteReg(0x104, 0x00);
        for(uint32_t a = 0; a < 18; ++a)
            m_four_op_category[a] = 0;
    }

    if(patchCleanUp)
    {
        //Reset patch settings
        memset(&m_patch, 0, sizeof(OPL_PatchSetup));
        m_patch.OPS[0].modulator_40   = 0x3F;
        m_patch.OPS[0].modulator_E862 = 0x00FFFF00;
        m_patch.OPS[1].carrier_40     = 0x3F;
        m_patch.OPS[1].carrier_E862   = 0x00FFFF00;
    }

    //Clear all operator registers from crap from previous patches
    for(uint32_t b = 0; b < NUM_OF_CHANNELS; ++b)
    {
        Patch(b, 0);
        Pan(b, 0x00);
        Touch_Real(b, 0);
    }
}

void Generator::Silence()
{
    //bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
    bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
    //Shutup!
    if(natural_4op)
    {
        for(uint32_t c = 0; c < USED_CHANNELS_4OP; ++c)
        {
            NoteOff(channels1_4op[c]);
            Touch_Real(channels1_4op[c], 0);
            Touch_Real(channels2_4op[c], 0);
        }
    }
    else
    {
        for(uint32_t c = 0; c < USED_CHANNELS_2OP; ++c)
        {
            NoteOff(channels[c]);
            Touch_Real(channels[c], 0);
        }
    }
}

void Generator::NoteOffAllChans()
{
    if(rythmModePercussionMode)
    {
        updateRegBD();
        return;
    }

    //bool pseudo_4op  = (m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op) != 0;
    bool natural_4op = (m_patch.flags & OPL_PatchSetup::Flag_True4op) != 0;
    if(natural_4op)
    {
        for(uint32_t c = 0; c < USED_CHANNELS_4OP; ++c)
            NoteOff(channels1_4op[c]);
    }
    else
    {
        for(uint32_t c = 0; c < NUM_OF_CHANNELS; ++c)
            NoteOff(channels[c]);
    }
}



void Generator::PlayNote()
{
    if(rythmModePercussionMode)
        PlayDrum(testDrum, note);
    else
        PlayNoteF(note);
}

void Generator::PlayMajorChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 4);
    PlayNoteF(note - 5);
}

void Generator::PlayMinorChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 3);
    PlayNoteF(note - 5);
}

void Generator::PlayAugmentedChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 4);
    PlayNoteF(note - 4);
}

void Generator::PlayDiminishedChord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note);
    PlayNoteF(note + 3);
    PlayNoteF(note - 6);
}

void Generator::PlayMajor7Chord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note - 2);
    PlayNoteF(note);
    PlayNoteF(note + 4);
    PlayNoteF(note - 5);
}

void Generator::PlayMinor7Chord()
{
    PlayNoteF(note - 12);
    PlayNoteF(note - 2);
    PlayNoteF(note);
    PlayNoteF(note + 3);
    PlayNoteF(note - 5);
}



void Generator::changePatch(FmBank::Instrument &instrument, bool isDrum)
{
    //Shutup everything
    Silence();
    switch4op(instrument.en_4op && !instrument.en_pseudo4op && (instrument.adlib_drum_number == 0));
    bool isAdLibDrums = isDrum && (instrument.adlib_drum_number > 0);
    changeAdLibPercussion(isAdLibDrums);

    if(isAdLibDrums)
    {
        testDrum = instrument.adlib_drum_number - 6;
        if(testDrum == 0)
        {
            m_patch.OPS[0].modulator_E862   = instrument.getDataE862(MODULATOR1);
            m_patch.OPS[0].modulator_40     = instrument.getKSLL(MODULATOR1);
            m_patch.OPS[0].carrier_E862     = instrument.getDataE862(CARRIER1);
            m_patch.OPS[0].carrier_40       = instrument.getKSLL(CARRIER1);
        }

        if((testDrum == 1) || (testDrum == 3))
        {
            m_patch.OPS[0].carrier_E862     = instrument.getDataE862(CARRIER1);
            m_patch.OPS[0].carrier_40       = instrument.getKSLL(CARRIER1);
            m_patch.OPS[0].modulator_E862   = instrument.getDataE862(CARRIER1);
            m_patch.OPS[0].modulator_40     = instrument.getKSLL(CARRIER1);
        }
        if((testDrum == 2) || (testDrum == 4))
        {
            m_patch.OPS[0].carrier_E862     = instrument.getDataE862(MODULATOR1);
            m_patch.OPS[0].carrier_40       = instrument.getKSLL(MODULATOR1);
            m_patch.OPS[0].modulator_E862   = instrument.getDataE862(MODULATOR1);
            m_patch.OPS[0].modulator_40     = instrument.getKSLL(MODULATOR1);
        }

        m_patch.OPS[0].feedconn         = instrument.getFBConn1();
        m_patch.flags   = 0;
        m_patch.tone    = instrument.percNoteNum;
        m_patch.voice2_fine_tune = 0.0;
    }
    else
    {
        m_patch.OPS[0].modulator_E862   = instrument.getDataE862(MODULATOR1);
        m_patch.OPS[0].modulator_40     = instrument.getKSLL(MODULATOR1);
        m_patch.OPS[0].carrier_E862     = instrument.getDataE862(CARRIER1);
        m_patch.OPS[0].carrier_40       = instrument.getKSLL(CARRIER1);
        m_patch.OPS[0].feedconn         = instrument.getFBConn1();
        m_patch.OPS[1].modulator_E862   = instrument.getDataE862(MODULATOR2);
        m_patch.OPS[1].modulator_40     = instrument.getKSLL(MODULATOR2);
        m_patch.OPS[1].carrier_E862     = instrument.getDataE862(CARRIER2);
        m_patch.OPS[1].carrier_40       = instrument.getKSLL(CARRIER2);
        m_patch.OPS[1].feedconn         = instrument.getFBConn2();
        m_patch.flags   = 0;
        m_patch.tone    = 0;
        m_patch.voice2_fine_tune = 0.0;

        if(isDrum)
        {
            m_patch.tone = instrument.percNoteNum;
            //if(instrument.percNoteNum && instrument.percNoteNum < 20)
            //{
            //    uint8_t nnum = instrument.percNoteNum;

            //    while(nnum && nnum < 20)
            //    {
            //        nnum += 12;
            //        m_patch.OPS[0].finetune -= 12;
            //        m_patch.OPS[1].finetune -= 12;
            //    }
            //}
        }

        if(instrument.en_4op && instrument.en_pseudo4op)
        {
            m_patch.voice2_fine_tune = (double(instrument.fine_tune) * 15.625) / 1000.0;

            if(instrument.fine_tune == 1)
                m_patch.voice2_fine_tune =  0.000025;
            else if(instrument.fine_tune == -1)
                m_patch.voice2_fine_tune = -0.000025;

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
    }
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

void Generator::changeAdLibPercussion(bool enabled)
{
    rythmModePercussionMode = uint8_t(enabled);
    updateRegBD();
}

void Generator::updateRegBD()
{
    m_regBD = (deepTremoloMode * 0x80) + (deepVibratoMode * 0x40) + (rythmModePercussionMode * 0x20);
    WriteReg(0x0BD, m_regBD);
}



void Generator::start()
{
    open(QIODevice::ReadOnly);
}

void Generator::stop()
{
    #ifdef ENABLE_OPL_PROXY
    if(chip_oplUninit)
        chip_oplUninit();
    #endif
    close();
}

qint64 Generator::readData(char *data, qint64 len)
{
    #ifdef ENABLE_OPL_EMULATOR
    std::lock_guard<std::mutex> g(chip_mutex);
    Q_UNUSED(g);
    int16_t *_out = reinterpret_cast<int16_t *>(data);
    len -= len % 4; //must be multiple 4!
    uint32_t lenS = (static_cast<uint32_t>(len) / 4);
    chip->generate(_out, lenS);
    #else
    memset(data, 0, size_t(len));
    #endif
    return len;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return MAX_OPLGEN_BUFFER_SIZE;
}
