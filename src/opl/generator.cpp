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

#include "generator.h"
#include <qendian.h>
#include <cmath>
#include <QtDebug>

#include "chips/nuked_opl3.h"
#include "chips/dosbox_opl3.h"
#include "chips/opal_opl3.h"
#include "chips/java_opl3.h"

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

//! Mapping from MIDI volume level to OPL level value.
static const uint_fast32_t g_dmx_volume_model[128] =
{
    0,  1,  3,  5,  6,  8,  10, 11,
    13, 14, 16, 17, 19, 20, 22, 23,
    25, 26, 27, 29, 30, 32, 33, 34,
    36, 37, 39, 41, 43, 45, 47, 49,
    50, 52, 54, 55, 57, 59, 60, 61,
    63, 64, 66, 67, 68, 69, 71, 72,
    73, 74, 75, 76, 77, 79, 80, 81,
    82, 83, 84, 84, 85, 86, 87, 88,
    89, 90, 91, 92, 92, 93, 94, 95,
    96, 96, 97, 98, 99, 99, 100, 101,
    101, 102, 103, 103, 104, 105, 105, 106,
    107, 107, 108, 109, 109, 110, 110, 111,
    112, 112, 113, 113, 114, 114, 115, 115,
    116, 117, 117, 118, 118, 119, 119, 120,
    120, 121, 121, 122, 122, 123, 123, 123,
    124, 124, 125, 125, 126, 126, 127, 127,
};

//! Mapping from MIDI volume level to OPL level value.
static const uint_fast32_t s_w9x_sb16_volume_model[32] =
{
    63, 63, 40, 36, 32, 28, 23, 21,
    19, 17, 15, 14, 13, 12, 11, 10,
    9,  8,  7,  6,  5,  5,  4,  4,
    3,  3,  2,  2,  1,  1,  0,  0
};

static const uint_fast32_t s_w9x_generic_fm_volume_model[32] =
{
    40, 36, 32, 28, 23, 21, 19, 17,
    15, 14, 13, 12, 11, 10, 9,  8,
    7,  6,  5,  5,  4,  4,  3,  3,
    2,  2,  1,  1,  1,  0,  0,  0
};

static const uint_fast32_t s_ail_vel_graph[16] =
{
    82,   85,  88,  91,  94,  97, 100, 103,
    106, 109, 112, 115, 118, 121, 124, 127
};

static const uint_fast32_t s_hmi_volume_table[64] =
{
    0x3F, 0x3A, 0x35, 0x30, 0x2C, 0x29, 0x25, 0x24,
    0x23, 0x22, 0x21, 0x20, 0x1F, 0x1E, 0x1D, 0x1C,
    0x1B, 0x1A, 0x19, 0x18, 0x17, 0x16, 0x15, 0x14,
    0x13, 0x12, 0x11, 0x10, 0x0F, 0x0E, 0x0E, 0x0D,
    0x0D, 0x0C, 0x0C, 0x0B, 0x0B, 0x0A, 0x0A, 0x09,
    0x09, 0x08, 0x08, 0x07, 0x07, 0x06, 0x06, 0x06,
    0x05, 0x05, 0x05, 0x04, 0x04, 0x04, 0x04, 0x03,
    0x03, 0x03, 0x02, 0x02, 0x02, 0x01, 0x01, 0x00,
};






/***************************************************************
 *               Standard frequency formula                    *
 * *************************************************************/

static inline double s_commonFreq(double tone)
{
    return BEND_COEFFICIENT * std::exp(0.057762265 * tone);
}




/***************************************************************
 *                   DMX frequency model                       *
 * *************************************************************/

// DMX volumes table
static const int_fast32_t s_dmx_freq_table[] =
{
    0x0133, 0x0133, 0x0134, 0x0134, 0x0135, 0x0136, 0x0136, 0x0137,
    0x0137, 0x0138, 0x0138, 0x0139, 0x0139, 0x013A, 0x013B, 0x013B,
    0x013C, 0x013C, 0x013D, 0x013D, 0x013E, 0x013F, 0x013F, 0x0140,
    0x0140, 0x0141, 0x0142, 0x0142, 0x0143, 0x0143, 0x0144, 0x0144,

    0x0145, 0x0146, 0x0146, 0x0147, 0x0147, 0x0148, 0x0149, 0x0149,
    0x014A, 0x014A, 0x014B, 0x014C, 0x014C, 0x014D, 0x014D, 0x014E,
    0x014F, 0x014F, 0x0150, 0x0150, 0x0151, 0x0152, 0x0152, 0x0153,
    0x0153, 0x0154, 0x0155, 0x0155, 0x0156, 0x0157, 0x0157, 0x0158,

    0x0158, 0x0159, 0x015A, 0x015A, 0x015B, 0x015B, 0x015C, 0x015D,
    0x015D, 0x015E, 0x015F, 0x015F, 0x0160, 0x0161, 0x0161, 0x0162,
    0x0162, 0x0163, 0x0164, 0x0164, 0x0165, 0x0166, 0x0166, 0x0167,
    0x0168, 0x0168, 0x0169, 0x016A, 0x016A, 0x016B, 0x016C, 0x016C,

    0x016D, 0x016E, 0x016E, 0x016F, 0x0170, 0x0170, 0x0171, 0x0172,
    0x0172, 0x0173, 0x0174, 0x0174, 0x0175, 0x0176, 0x0176, 0x0177,
    0x0178, 0x0178, 0x0179, 0x017A, 0x017A, 0x017B, 0x017C, 0x017C,
    0x017D, 0x017E, 0x017E, 0x017F, 0x0180, 0x0181, 0x0181, 0x0182,

    0x0183, 0x0183, 0x0184, 0x0185, 0x0185, 0x0186, 0x0187, 0x0188,
    0x0188, 0x0189, 0x018A, 0x018A, 0x018B, 0x018C, 0x018D, 0x018D,
    0x018E, 0x018F, 0x018F, 0x0190, 0x0191, 0x0192, 0x0192, 0x0193,
    0x0194, 0x0194, 0x0195, 0x0196, 0x0197, 0x0197, 0x0198, 0x0199,

    0x019A, 0x019A, 0x019B, 0x019C, 0x019D, 0x019D, 0x019E, 0x019F,
    0x01A0, 0x01A0, 0x01A1, 0x01A2, 0x01A3, 0x01A3, 0x01A4, 0x01A5,
    0x01A6, 0x01A6, 0x01A7, 0x01A8, 0x01A9, 0x01A9, 0x01AA, 0x01AB,
    0x01AC, 0x01AD, 0x01AD, 0x01AE, 0x01AF, 0x01B0, 0x01B0, 0x01B1,

    0x01B2, 0x01B3, 0x01B4, 0x01B4, 0x01B5, 0x01B6, 0x01B7, 0x01B8,
    0x01B8, 0x01B9, 0x01BA, 0x01BB, 0x01BC, 0x01BC, 0x01BD, 0x01BE,
    0x01BF, 0x01C0, 0x01C0, 0x01C1, 0x01C2, 0x01C3, 0x01C4, 0x01C4,
    0x01C5, 0x01C6, 0x01C7, 0x01C8, 0x01C9, 0x01C9, 0x01CA, 0x01CB,

    0x01CC, 0x01CD, 0x01CE, 0x01CE, 0x01CF, 0x01D0, 0x01D1, 0x01D2,
    0x01D3, 0x01D3, 0x01D4, 0x01D5, 0x01D6, 0x01D7, 0x01D8, 0x01D8,
    0x01D9, 0x01DA, 0x01DB, 0x01DC, 0x01DD, 0x01DE, 0x01DE, 0x01DF,
    0x01E0, 0x01E1, 0x01E2, 0x01E3, 0x01E4, 0x01E5, 0x01E5, 0x01E6,

    0x01E7, 0x01E8, 0x01E9, 0x01EA, 0x01EB, 0x01EC, 0x01ED, 0x01ED,
    0x01EE, 0x01EF, 0x01F0, 0x01F1, 0x01F2, 0x01F3, 0x01F4, 0x01F5,
    0x01F6, 0x01F6, 0x01F7, 0x01F8, 0x01F9, 0x01FA, 0x01FB, 0x01FC,
    0x01FD, 0x01FE, 0x01FF,

    0x0200, 0x0201, 0x0201, 0x0202, 0x0203, 0x0204, 0x0205, 0x0206,
    0x0207, 0x0208, 0x0209, 0x020A, 0x020B, 0x020C, 0x020D, 0x020E,
    0x020F, 0x0210, 0x0210, 0x0211, 0x0212, 0x0213, 0x0214, 0x0215,
    0x0216, 0x0217, 0x0218, 0x0219, 0x021A, 0x021B, 0x021C, 0x021D,

    0x021E, 0x021F, 0x0220, 0x0221, 0x0222, 0x0223, 0x0224, 0x0225,
    0x0226, 0x0227, 0x0228, 0x0229, 0x022A, 0x022B, 0x022C, 0x022D,
    0x022E, 0x022F, 0x0230, 0x0231, 0x0232, 0x0233, 0x0234, 0x0235,
    0x0236, 0x0237, 0x0238, 0x0239, 0x023A, 0x023B, 0x023C, 0x023D,

    0x023E, 0x023F, 0x0240, 0x0241, 0x0242, 0x0244, 0x0245, 0x0246,
    0x0247, 0x0248, 0x0249, 0x024A, 0x024B, 0x024C, 0x024D, 0x024E,
    0x024F, 0x0250, 0x0251, 0x0252, 0x0253, 0x0254, 0x0256, 0x0257,
    0x0258, 0x0259, 0x025A, 0x025B, 0x025C, 0x025D, 0x025E, 0x025F,

    0x0260, 0x0262, 0x0263, 0x0264, 0x0265, 0x0266, 0x0267, 0x0268,
    0x0269, 0x026A, 0x026C, 0x026D, 0x026E, 0x026F, 0x0270, 0x0271,
    0x0272, 0x0273, 0x0275, 0x0276, 0x0277, 0x0278, 0x0279, 0x027A,
    0x027B, 0x027D, 0x027E, 0x027F, 0x0280, 0x0281, 0x0282, 0x0284,

    0x0285, 0x0286, 0x0287, 0x0288, 0x0289, 0x028B, 0x028C, 0x028D,
    0x028E, 0x028F, 0x0290, 0x0292, 0x0293, 0x0294, 0x0295, 0x0296,
    0x0298, 0x0299, 0x029A, 0x029B, 0x029C, 0x029E, 0x029F, 0x02A0,
    0x02A1, 0x02A2, 0x02A4, 0x02A5, 0x02A6, 0x02A7, 0x02A9, 0x02AA,

    0x02AB, 0x02AC, 0x02AE, 0x02AF, 0x02B0, 0x02B1, 0x02B2, 0x02B4,
    0x02B5, 0x02B6, 0x02B7, 0x02B9, 0x02BA, 0x02BB, 0x02BD, 0x02BE,
    0x02BF, 0x02C0, 0x02C2, 0x02C3, 0x02C4, 0x02C5, 0x02C7, 0x02C8,
    0x02C9, 0x02CB, 0x02CC, 0x02CD, 0x02CE, 0x02D0, 0x02D1, 0x02D2,

    0x02D4, 0x02D5, 0x02D6, 0x02D8, 0x02D9, 0x02DA, 0x02DC, 0x02DD,
    0x02DE, 0x02E0, 0x02E1, 0x02E2, 0x02E4, 0x02E5, 0x02E6, 0x02E8,
    0x02E9, 0x02EA, 0x02EC, 0x02ED, 0x02EE, 0x02F0, 0x02F1, 0x02F2,
    0x02F4, 0x02F5, 0x02F6, 0x02F8, 0x02F9, 0x02FB, 0x02FC, 0x02FD,

    0x02FF, 0x0300, 0x0302, 0x0303, 0x0304, 0x0306, 0x0307, 0x0309,
    0x030A, 0x030B, 0x030D, 0x030E, 0x0310, 0x0311, 0x0312, 0x0314,
    0x0315, 0x0317, 0x0318, 0x031A, 0x031B, 0x031C, 0x031E, 0x031F,
    0x0321, 0x0322, 0x0324, 0x0325, 0x0327, 0x0328, 0x0329, 0x032B,

    0x032C, 0x032E, 0x032F, 0x0331, 0x0332, 0x0334, 0x0335, 0x0337,
    0x0338, 0x033A, 0x033B, 0x033D, 0x033E, 0x0340, 0x0341, 0x0343,
    0x0344, 0x0346, 0x0347, 0x0349, 0x034A, 0x034C, 0x034D, 0x034F,
    0x0350, 0x0352, 0x0353, 0x0355, 0x0357, 0x0358, 0x035A, 0x035B,

    0x035D, 0x035E, 0x0360, 0x0361, 0x0363, 0x0365, 0x0366, 0x0368,
    0x0369, 0x036B, 0x036C, 0x036E, 0x0370, 0x0371, 0x0373, 0x0374,
    0x0376, 0x0378, 0x0379, 0x037B, 0x037C, 0x037E, 0x0380, 0x0381,
    0x0383, 0x0384, 0x0386, 0x0388, 0x0389, 0x038B, 0x038D, 0x038E,

    0x0390, 0x0392, 0x0393, 0x0395, 0x0397, 0x0398, 0x039A, 0x039C,
    0x039D, 0x039F, 0x03A1, 0x03A2, 0x03A4, 0x03A6, 0x03A7, 0x03A9,
    0x03AB, 0x03AC, 0x03AE, 0x03B0, 0x03B1, 0x03B3, 0x03B5, 0x03B7,
    0x03B8, 0x03BA, 0x03BC, 0x03BD, 0x03BF, 0x03C1, 0x03C3, 0x03C4,

    0x03C6, 0x03C8, 0x03CA, 0x03CB, 0x03CD, 0x03CF, 0x03D1, 0x03D2,
    0x03D4, 0x03D6, 0x03D8, 0x03DA, 0x03DB, 0x03DD, 0x03DF, 0x03E1,
    0x03E3, 0x03E4, 0x03E6, 0x03E8, 0x03EA, 0x03EC, 0x03ED, 0x03EF,
    0x03F1, 0x03F3, 0x03F5, 0x03F6, 0x03F8, 0x03FA, 0x03FC, 0x03FE,

    0x036C
};

static inline double s_dmxFreq(double tone)
{
    uint_fast32_t noteI = (uint_fast32_t)(tone);
    int_fast32_t bendI = 0;
    int_fast32_t outHz = 0.0;
    double bendDec = tone - (int)tone;

    bendI = (int_fast32_t)((bendDec * 128.0) / 2.0) + 128;
    bendI = bendI >> 1;

    int_fast32_t oct = 0;
    int_fast32_t freqIndex = (noteI << 5) + bendI;

#define MAX_FREQ_IDX 283 // 284 - with the DMX side bug
    if(freqIndex < 0)
        freqIndex = 0;
    else if(freqIndex >= MAX_FREQ_IDX)
    {
        freqIndex -= MAX_FREQ_IDX;
        oct = freqIndex / 384;
        freqIndex = (freqIndex % 384) + MAX_FREQ_IDX;
    }
#undef MAX_FREQ_IDX

    outHz = s_dmx_freq_table[freqIndex];

    while(oct > 1)
    {
        outHz *= 2;
        oct -= 1;
    }

    return (double)outHz;
}




/***************************************************************
 *             Apogee Sound System frequency model             *
 ***************************************************************/

static const int_fast32_t s_apogee_freq_table[31 + 1][12] =
{
    { 0x157, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca, 0x1e5, 0x202, 0x220, 0x241, 0x263, 0x287 },
    { 0x157, 0x16b, 0x181, 0x198, 0x1b0, 0x1ca, 0x1e5, 0x202, 0x220, 0x242, 0x264, 0x288 },
    { 0x158, 0x16c, 0x182, 0x199, 0x1b1, 0x1cb, 0x1e6, 0x203, 0x221, 0x243, 0x265, 0x289 },
    { 0x158, 0x16c, 0x183, 0x19a, 0x1b2, 0x1cc, 0x1e7, 0x204, 0x222, 0x244, 0x266, 0x28a },
    { 0x159, 0x16d, 0x183, 0x19a, 0x1b3, 0x1cd, 0x1e8, 0x205, 0x223, 0x245, 0x267, 0x28b },
    { 0x15a, 0x16e, 0x184, 0x19b, 0x1b3, 0x1ce, 0x1e9, 0x206, 0x224, 0x246, 0x268, 0x28c },
    { 0x15a, 0x16e, 0x185, 0x19c, 0x1b4, 0x1ce, 0x1ea, 0x207, 0x225, 0x247, 0x269, 0x28e },
    { 0x15b, 0x16f, 0x185, 0x19d, 0x1b5, 0x1cf, 0x1eb, 0x208, 0x226, 0x248, 0x26a, 0x28f },
    { 0x15b, 0x170, 0x186, 0x19d, 0x1b6, 0x1d0, 0x1ec, 0x209, 0x227, 0x249, 0x26b, 0x290 },
    { 0x15c, 0x170, 0x187, 0x19e, 0x1b7, 0x1d1, 0x1ec, 0x20a, 0x228, 0x24a, 0x26d, 0x291 },
    { 0x15d, 0x171, 0x188, 0x19f, 0x1b7, 0x1d2, 0x1ed, 0x20b, 0x229, 0x24b, 0x26e, 0x292 },
    { 0x15d, 0x172, 0x188, 0x1a0, 0x1b8, 0x1d3, 0x1ee, 0x20c, 0x22a, 0x24c, 0x26f, 0x293 },
    { 0x15e, 0x172, 0x189, 0x1a0, 0x1b9, 0x1d4, 0x1ef, 0x20d, 0x22b, 0x24d, 0x270, 0x295 },
    { 0x15f, 0x173, 0x18a, 0x1a1, 0x1ba, 0x1d4, 0x1f0, 0x20e, 0x22c, 0x24e, 0x271, 0x296 },
    { 0x15f, 0x174, 0x18a, 0x1a2, 0x1bb, 0x1d5, 0x1f1, 0x20f, 0x22d, 0x24f, 0x272, 0x297 },
    { 0x160, 0x174, 0x18b, 0x1a3, 0x1bb, 0x1d6, 0x1f2, 0x210, 0x22e, 0x250, 0x273, 0x298 },
    { 0x161, 0x175, 0x18c, 0x1a3, 0x1bc, 0x1d7, 0x1f3, 0x211, 0x22f, 0x251, 0x274, 0x299 },
    { 0x161, 0x176, 0x18c, 0x1a4, 0x1bd, 0x1d8, 0x1f4, 0x212, 0x230, 0x252, 0x276, 0x29b },
    { 0x162, 0x176, 0x18d, 0x1a5, 0x1be, 0x1d9, 0x1f5, 0x212, 0x231, 0x254, 0x277, 0x29c },
    { 0x162, 0x177, 0x18e, 0x1a6, 0x1bf, 0x1d9, 0x1f5, 0x213, 0x232, 0x255, 0x278, 0x29d },
    { 0x163, 0x178, 0x18f, 0x1a6, 0x1bf, 0x1da, 0x1f6, 0x214, 0x233, 0x256, 0x279, 0x29e },
    { 0x164, 0x179, 0x18f, 0x1a7, 0x1c0, 0x1db, 0x1f7, 0x215, 0x235, 0x257, 0x27a, 0x29f },
    { 0x164, 0x179, 0x190, 0x1a8, 0x1c1, 0x1dc, 0x1f8, 0x216, 0x236, 0x258, 0x27b, 0x2a1 },
    { 0x165, 0x17a, 0x191, 0x1a9, 0x1c2, 0x1dd, 0x1f9, 0x217, 0x237, 0x259, 0x27c, 0x2a2 },
    { 0x166, 0x17b, 0x192, 0x1aa, 0x1c3, 0x1de, 0x1fa, 0x218, 0x238, 0x25a, 0x27e, 0x2a3 },
    { 0x166, 0x17b, 0x192, 0x1aa, 0x1c3, 0x1df, 0x1fb, 0x219, 0x239, 0x25b, 0x27f, 0x2a4 },
    { 0x167, 0x17c, 0x193, 0x1ab, 0x1c4, 0x1e0, 0x1fc, 0x21a, 0x23a, 0x25c, 0x280, 0x2a6 },
    { 0x168, 0x17d, 0x194, 0x1ac, 0x1c5, 0x1e0, 0x1fd, 0x21b, 0x23b, 0x25d, 0x281, 0x2a7 },
    { 0x168, 0x17d, 0x194, 0x1ad, 0x1c6, 0x1e1, 0x1fe, 0x21c, 0x23c, 0x25e, 0x282, 0x2a8 },
    { 0x169, 0x17e, 0x195, 0x1ad, 0x1c7, 0x1e2, 0x1ff, 0x21d, 0x23d, 0x260, 0x283, 0x2a9 },
    { 0x16a, 0x17f, 0x196, 0x1ae, 0x1c8, 0x1e3, 0x1ff, 0x21e, 0x23e, 0x261, 0x284, 0x2ab },
    { 0x16a, 0x17f, 0x197, 0x1af, 0x1c8, 0x1e4, 0x200, 0x21f, 0x23f, 0x262, 0x286, 0x2ac }
};

static inline double s_apogeeFreq(double tone)
{
    uint_fast32_t noteI = (uint_fast32_t)(tone);
    int_fast32_t bendI = 0;
    int_fast32_t outHz = 0.0;
    double bendDec = tone - (int)tone;
    int_fast32_t octave;
    int_fast32_t scaleNote;

    bendI = (int_fast32_t)(bendDec * 32) + 32;

    noteI += bendI / 32;
    noteI -= 1;

    scaleNote = noteI % 12;
    octave = noteI / 12;

    outHz = s_apogee_freq_table[bendI % 32][scaleNote];

    while(octave > 1)
    {
        outHz *= 2;
        octave -= 1;
    }

    return (double)outHz;
}




/***************************************************************
 *            Windows 9x FM drivers frequency model            *
 ***************************************************************/

//static const double s_9x_opl_samplerate = 50000.0;
//static const double s_9x_opl_tune = 440.0;
static const uint_fast8_t s_9x_opl_pitchfrac = 8;

static const uint_fast32_t s_9x_opl_freq[12] =
{
    0xAB7, 0xB5A, 0xC07, 0xCBE, 0xD80, 0xE4D, 0xF27, 0x100E, 0x1102, 0x1205, 0x1318, 0x143A
};

static const int32_t s_9x_opl_uppitch = 31;
static const int32_t s_9x_opl_downpitch = 27;

static uint_fast32_t s_9x_opl_applypitch(uint_fast32_t freq, int_fast32_t pitch)
{
    int32_t diff;

    if(pitch > 0)
    {
        diff = (pitch * s_9x_opl_uppitch) >> s_9x_opl_pitchfrac;
        freq += (diff * freq) >> 15;
    }
    else if (pitch < 0)
    {
        diff = (-pitch * s_9x_opl_downpitch) >> s_9x_opl_pitchfrac;
        freq -= (diff * freq) >> 15;
    }

    return freq;
}

static inline double s_9xFreq(double tone)
{
    uint_fast32_t note = (uint_fast32_t)(tone);
    int_fast32_t bend;
    double bendDec = tone - (int)tone; // 0.0 ± 1.0 - one halftone

    uint_fast32_t freq;
    uint_fast32_t freqpitched;
    uint_fast32_t octave;

    uint_fast32_t bendMsb;
    uint_fast32_t bendLsb;

    bend = (int_fast32_t)(bendDec * 4096) + 8192; // convert to MIDI standard value

    bendMsb = (bend >> 7) & 0x7F;
    bendLsb = (bend & 0x7F);

    bend = (bendMsb << 9) | (bendLsb << 2);
    bend = (int16_t)(uint16_t)(bend + 0x8000);

    octave = note / 12;
    freq = s_9x_opl_freq[note % 12];
    if(octave < 5)
        freq >>= (5 - octave);
    else if (octave > 5)
        freq <<= (octave - 5);

    freqpitched = s_9x_opl_applypitch(freq, bend);
    freqpitched *= 2;

    return (double)freqpitched;
}




/***************************************************************
 *         HMI Sound Operating System frequency model          *
 ***************************************************************/

const size_t s_hmi_freqtable_size = 103;
static uint_fast32_t s_hmi_freqtable[s_hmi_freqtable_size] =
{
    0x0157, 0x016B, 0x0181, 0x0198, 0x01B0, 0x01CA, 0x01E5, 0x0202, 0x0220, 0x0241, 0x0263, 0x0287,
    0x0557, 0x056B, 0x0581, 0x0598, 0x05B0, 0x05CA, 0x05E5, 0x0602, 0x0620, 0x0641, 0x0663, 0x0687,
    0x0957, 0x096B, 0x0981, 0x0998, 0x09B0, 0x09CA, 0x09E5, 0x0A02, 0x0A20, 0x0A41, 0x0A63, 0x0A87,
    0x0D57, 0x0D6B, 0x0D81, 0x0D98, 0x0DB0, 0x0DCA, 0x0DE5, 0x0E02, 0x0E20, 0x0E41, 0x0E63, 0x0E87,
    0x1157, 0x116B, 0x1181, 0x1198, 0x11B0, 0x11CA, 0x11E5, 0x1202, 0x1220, 0x1241, 0x1263, 0x1287,
    0x1557, 0x156B, 0x1581, 0x1598, 0x15B0, 0x15CA, 0x15E5, 0x1602, 0x1620, 0x1641, 0x1663, 0x1687,
    0x1957, 0x196B, 0x1981, 0x1998, 0x19B0, 0x19CA, 0x19E5, 0x1A02, 0x1A20, 0x1A41, 0x1A63, 0x1A87,
    0x1D57, 0x1D6B, 0x1D81, 0x1D98, 0x1DB0, 0x1DCA, 0x1DE5, 0x1E02, 0x1E20, 0x1E41, 0x1E63, 0x1E87,
    0x1EAE, 0x1EB7, 0x1F02, 0x1F30, 0x1F60, 0x1F94, 0x1FCA
};

const size_t s_hmi_bendtable_size = 12;
static uint_fast32_t s_hmi_bendtable[s_hmi_bendtable_size] =
{
    0x144, 0x132, 0x121, 0x110, 0x101, 0xf8, 0xe5, 0xd8, 0xcc, 0xc1, 0xb6, 0xac
};

#define hmi_range_fix(formula, maxVal) \
    ( \
        (formula) < 0 ? \
        0 : \
        ( \
            (formula) >= (int32_t)maxVal ? \
            (int32_t)maxVal : \
            (formula) \
        )\
    )

static uint_fast32_t s_hmi_bend_calc(uint_fast32_t bend, int_fast32_t note)
{
    const int_fast32_t midi_bend_range = 1;
    uint_fast32_t bendFactor, outFreq, fmOctave, fmFreq, newFreq, idx;
    int_fast32_t noteMod12;

    note -= 12;
//    while(doNote >= 12) // ugly way to MOD 12
//        doNote -= 12;
    noteMod12 = (note % 12);

    outFreq = s_hmi_freqtable[note];

    fmOctave = outFreq & 0x1c00;
    fmFreq = outFreq & 0x3ff;

    if(bend < 64)
    {
        bendFactor = ((63 - bend) * 1000) >> 6;

        idx = hmi_range_fix(note - midi_bend_range, s_hmi_freqtable_size);
        newFreq = outFreq - s_hmi_freqtable[idx];

        if(newFreq > 719)
        {
            newFreq = fmFreq - s_hmi_bendtable[midi_bend_range - 1];
            newFreq &= 0x3ff;
        }

        newFreq = (newFreq * bendFactor) / 1000;
        outFreq -= newFreq;
    }
    else
    {
        bendFactor = ((bend - 64) * 1000) >> 6;

        idx = hmi_range_fix(note + midi_bend_range, s_hmi_freqtable_size);
        newFreq = s_hmi_freqtable[idx] - outFreq;

        if(newFreq > 719)
        {
            idx = hmi_range_fix(11 - noteMod12, s_hmi_bendtable_size);
            fmFreq = s_hmi_bendtable[idx];
            outFreq = (fmOctave + 1024) | fmFreq;

            idx = hmi_range_fix(note + midi_bend_range, s_hmi_freqtable_size);
            newFreq = s_hmi_freqtable[idx] - outFreq;
        }

        newFreq = (newFreq * bendFactor) / 1000;
        outFreq += newFreq;
    }

    return outFreq;
}
#undef hmi_range_fix

static inline double s_hmiFreq(double tone)
{
    int_fast32_t note = (int_fast32_t)(tone);
    double bendDec = tone - (int)tone; // 0.0 ± 1.0 - one halftone
    int_fast32_t bend;
    uint_fast32_t inFreq;
    uint_fast32_t freq;
    int_fast32_t octave;
    int_fast32_t octaveOffset = 0;

    bend = (int_fast32_t)(bendDec * 64.0) + 64;

    while(note < 12)
    {
        octaveOffset--;
        note += 12;
    }
    while(note > 114)
    {
        octaveOffset++;
        note -= 12;
    }

    if(bend == 64)
        inFreq = s_hmi_freqtable[note - 12];
    else
        inFreq = s_hmi_bend_calc(bend, note);

    freq = inFreq & 0x3FF;
    octave = (inFreq >> 10) & 0x07;

    octave += octaveOffset;

    while(octave > 0)
    {
        freq *= 2;
        octave -= 1;
    }

    return freq;
}




/***************************************************************
 *          Audio Interface Library frequency model            *
 ***************************************************************/

static const uint_fast16_t s_ail_freqtable[] = {
    0x02b2, 0x02b4, 0x02b7, 0x02b9, 0x02bc, 0x02be, 0x02c1, 0x02c3,
    0x02c6, 0x02c9, 0x02cb, 0x02ce, 0x02d0, 0x02d3, 0x02d6, 0x02d8,
    0x02db, 0x02dd, 0x02e0, 0x02e3, 0x02e5, 0x02e8, 0x02eb, 0x02ed,
    0x02f0, 0x02f3, 0x02f6, 0x02f8, 0x02fb, 0x02fe, 0x0301, 0x0303,
    0x0306, 0x0309, 0x030c, 0x030f, 0x0311, 0x0314, 0x0317, 0x031a,
    0x031d, 0x0320, 0x0323, 0x0326, 0x0329, 0x032b, 0x032e, 0x0331,
    0x0334, 0x0337, 0x033a, 0x033d, 0x0340, 0x0343, 0x0346, 0x0349,
    0x034c, 0x034f, 0x0352, 0x0356, 0x0359, 0x035c, 0x035f, 0x0362,
    0x0365, 0x0368, 0x036b, 0x036f, 0x0372, 0x0375, 0x0378, 0x037b,
    0x037f, 0x0382, 0x0385, 0x0388, 0x038c, 0x038f, 0x0392, 0x0395,
    0x0399, 0x039c, 0x039f, 0x03a3, 0x03a6, 0x03a9, 0x03ad, 0x03b0,
    0x03b4, 0x03b7, 0x03bb, 0x03be, 0x03c1, 0x03c5, 0x03c8, 0x03cc,
    0x03cf, 0x03d3, 0x03d7, 0x03da, 0x03de, 0x03e1, 0x03e5, 0x03e8,
    0x03ec, 0x03f0, 0x03f3, 0x03f7, 0x03fb, 0x03fe, 0xfe01, 0xfe03,
    0xfe05, 0xfe07, 0xfe08, 0xfe0a, 0xfe0c, 0xfe0e, 0xfe10, 0xfe12,
    0xfe14, 0xfe16, 0xfe18, 0xfe1a, 0xfe1c, 0xfe1e, 0xfe20, 0xfe21,
    0xfe23, 0xfe25, 0xfe27, 0xfe29, 0xfe2b, 0xfe2d, 0xfe2f, 0xfe31,
    0xfe34, 0xfe36, 0xfe38, 0xfe3a, 0xfe3c, 0xfe3e, 0xfe40, 0xfe42,
    0xfe44, 0xfe46, 0xfe48, 0xfe4a, 0xfe4c, 0xfe4f, 0xfe51, 0xfe53,
    0xfe55, 0xfe57, 0xfe59, 0xfe5c, 0xfe5e, 0xfe60, 0xfe62, 0xfe64,
    0xfe67, 0xfe69, 0xfe6b, 0xfe6d, 0xfe6f, 0xfe72, 0xfe74, 0xfe76,
    0xfe79, 0xfe7b, 0xfe7d, 0xfe7f, 0xfe82, 0xfe84, 0xfe86, 0xfe89,
    0xfe8b, 0xfe8d, 0xfe90, 0xfe92, 0xfe95, 0xfe97, 0xfe99, 0xfe9c,
    0xfe9e, 0xfea1, 0xfea3, 0xfea5, 0xfea8, 0xfeaa, 0xfead, 0xfeaf
};

static const uint_fast8_t s_ail_note_octave[] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01, 0x01,
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01,
    0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x02,
    0x02, 0x02, 0x02, 0x02, 0x03, 0x03, 0x03, 0x03,
    0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03,
    0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04,
    0x04, 0x04, 0x04, 0x04, 0x05, 0x05, 0x05, 0x05,
    0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05, 0x05,
    0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06,
    0x06, 0x06, 0x06, 0x06, 0x07, 0x07, 0x07, 0x07,
    0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07
};

static const uint_fast8_t s_ail_note_halftone[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0a, 0x0b, 0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b
};

static inline double s_ailFreq(double tone)
{
    int_fast32_t note = (int_fast32_t)(tone);
    double bendDec = tone - (int)tone; // 0.0 ± 1.0 - one halftone
    int_fast32_t pitch;
    uint_fast16_t freq;
    int_fast32_t octave;
    int_fast32_t octaveOffset = 0;
    uint_fast8_t halftones;

    pitch = (int_fast32_t)(bendDec * 4096) + 8192; // convert to MIDI standard value
    pitch = ((pitch - 0x2000) / 0x20) * 2;

    note -= 12;

    while(note < 0)
    {
        octaveOffset--;
        note += 12;
    }
    while(note > 95)
    {
        octaveOffset++;
        note -= 12;
    }

    pitch += (((uint_fast8_t)note) << 8) + 8;
    pitch /= 16;
    while (pitch < 12 * 16) {
        pitch += 12 * 16;
    }
    while (pitch > 96 * 16 - 1) {
        pitch -= 12 * 16;
    }

    halftones = (s_ail_note_halftone[pitch >> 4] << 4) + (pitch & 0x0f);
    freq = s_ail_freqtable[halftones];
    octave = s_ail_note_octave[pitch >> 4];

    if((freq & 0x8000) == 0)
    {
        if (octave > 0) {
            octave--;
        } else {
            freq /= 2;
        }
    }

    freq &= 0x3FF;

    octave += octaveOffset;

    while(octave > 0)
    {
        freq *= 2;
        octave -= 1;
    }

    return freq;
}







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
    memset(m_ins, 0, sizeof(uint16_t) * NUM_OF_CHANNELS);
    memset(m_keyBlockFNumCache, 0, sizeof(uint8_t) * NUM_OF_CHANNELS);
    memset(m_four_op_category, 0, NUM_OF_CHANNELS * 2);

    uint32_t p = 0;
    for(uint32_t b = 0; b < 18; ++b)
        m_four_op_category[p++] = 0;
    for(uint32_t b = 0; b < 5; ++b)
        m_four_op_category[p++] = ChanCat_Rhythm_Slave;

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
            WriteReg(data[a], static_cast<uint8_t>(data_opl2[a + 1]));
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
#ifdef ENABLE_HW_OPL_SERIAL_PORT
    case CHIP_SerialPort:
        chip.reset(&serialPortOpl());
        break;
#endif
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
    }

    initChip();
}

void Generator::WriteReg(uint16_t address, uint8_t byte)
{
    chip->writeReg(address, byte);
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

    WriteReg(0xB0 + g_Channels[cc], m_keyBlockFNumCache[c] & 0xDF);
}

void Generator::NoteOn(uint32_t c1, uint32_t c2, double tone, bool voice2ps4op) // Hertz range: 0..131071
{
    uint32_t cc1 = c1 % 23;
    uint32_t cc2 = c2 % 23;
    uint32_t octave = 0, ftone = 0, mul_offset = 0;

    double hertz;

    // Use different frequency formulas in depend on a volume model
    switch(m_volumeScale)
    {
    case VOLUME_DMX:
    case VOLUME_DMX_FIXED:
        hertz = s_dmxFreq(tone);
        break;

    case VOLUME_APOGEE:
    case VOLUME_APOGEE_FIXED:
        hertz = s_apogeeFreq(tone);
        break;

    case VOLUME_9X:
    case VOLUME_9X_GENERIC_FM:
        hertz = s_9xFreq(tone);
        break;

    case VOLUME_HMI:
    case VOLUME_HMI_OLD:
        hertz = s_hmiFreq(tone);
        break;

    case VOLUME_AIL:
        hertz = s_ailFreq(tone);
        break;

    default:
        hertz = s_commonFreq(tone);
    }

    if(hertz < 0)
        return;

    //Basic range until max of octaves reaching
    while((hertz >= 1023.5) && (octave < 0x1C00))
    {
        hertz /= 2.0;    // Calculate octave
        octave += 0x400;
    }
    //Extended range, rely on frequency multiplication increment
    while(hertz >= 1022.75)
    {
        hertz /= 2.0;    // Calculate octave
        mul_offset++;
    }
    ftone = octave + static_cast<uint32_t>(hertz + 0.5);
    uint16_t chn = g_Channels[cc1];

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

static inline uint_fast32_t brightnessToOPL(uint_fast32_t brightness)
{
    double b = static_cast<double>(brightness);
    double ret = ::round(127.0 * ::sqrt(b * (1.0 / 127.0))) / 2.0;
    return static_cast<uint_fast32_t>(ret);
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
    bool do_modulator = false;
    bool do_carrier = false;
    uint32_t mode = 1; // 2-op AM

    uint_fast32_t kslMod = srcMod & 0xC0;
    uint_fast32_t kslCar = srcCar & 0xC0;
    uint_fast32_t tlMod = srcMod & 63;
    uint_fast32_t tlCar = srcCar & 63;

    uint_fast32_t modulator;
    uint_fast32_t carrier;

    uint32_t volume = 0;
    uint32_t midiVolume = 0;

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


    // ------ Mix volumes and compute average ------

    switch(m_volumeScale)
    {
    default:
    case VOLUME_Generic:
    {
        // The formula below: SOLVE(V=127^3 * 2^( (A-63.49999) / 8), A)
        volume = velocity * ccvolume * ccexpr * 127;
        const double c1 = 11.541560327111707;
        const double c2 = 1.601379199767093e+02;
        uint_fast32_t minVolume = 8725 * 127;

        if(volume > minVolume)
        {
            double lv = std::log(static_cast<double>(volume));
            volume = static_cast<uint_fast32_t>(lv * c1 - c2);
        }
        else
            volume = 0;
    }
    break;

    case VOLUME_CMF:
    {
        volume = velocity * ccvolume * ccexpr;
        volume = volume * 127 / (127 * 127 * 127) / 2;
    }
    break;

    case VOLUME_DMX:
    case VOLUME_DMX_FIXED:
    {
        volume = (ccvolume * ccexpr * 127) / 16129;
        volume = (g_dmx_volume_model[volume] + 1) << 1;
        volume = (g_dmx_volume_model[(velocity < 128) ? velocity : 127] * volume) >> 9;
    }
    break;

    case VOLUME_APOGEE:
    case VOLUME_APOGEE_FIXED:
    {
        volume = 0;
        midiVolume = (ccvolume * ccexpr * 127 / 16129);
    }
    break;

    case VOLUME_9X:
    {
        volume = (ccvolume * ccexpr * 127) / 16129;
        volume = s_w9x_sb16_volume_model[volume >> 2];
    }
    break;

    case VOLUME_9X_GENERIC_FM:
    {
        volume = (ccvolume * ccexpr * 127) / 16129;
        volume = s_w9x_generic_fm_volume_model[volume >> 2];
    }
    break;

    case VOLUME_AIL:
    {
        midiVolume = (ccvolume * ccexpr) * 2;
        midiVolume >>= 8;
        if(midiVolume != 0)
            midiVolume++;

        velocity = (velocity & 0x7F) >> 3;
        velocity = s_ail_vel_graph[velocity];

        midiVolume = (midiVolume * velocity) * 2;
        midiVolume >>= 8;
        if(midiVolume != 0)
            midiVolume++;

        /*if(m_masterVolume < 127)
            midiVolume = (midiVolume * m_masterVolume) / 127;*/
    }
    break;

    case VOLUME_HMI:
    case VOLUME_HMI_OLD:
    {
        /* Temporarily copying DMX volume model. TODO: Reverse-engine the actual HMI volume model! */
        volume = (ccvolume * ccexpr * 127) / 16129;
        volume = (((volume * 128) / 127) * velocity) >> 7;
        volume = s_hmi_volume_table[volume >> 1];
    }
    break;
    }

    if(volume > 63)
        volume = 63;

    if(midiVolume > 127)
        midiVolume = 127;

    if(m_four_op_category[c] == ChanCat_Regular ||
       m_four_op_category[c] == ChanCat_Rhythm_Bass)
    {
        mode = m_patch.OPS[i].feedconn & 1; // 2-op FM or 2-op AM
    }
    else if(m_four_op_category[c] == ChanCat_4op_Master ||
            m_four_op_category[c] == ChanCat_4op_Slave)
    {
        uint32_t i0, i1;
        if(m_four_op_category[c] == ChanCat_4op_Master)
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

    do_modulator = do_ops[ mode ][ 0 ];
    do_carrier   = do_ops[ mode ][ 1 ];

    // ------ Compute the total level register output data ------

    if((m_volumeScale == VOLUME_APOGEE ||
        m_volumeScale == VOLUME_APOGEE_FIXED) &&
        mode <= 1)
    {
        if(do_carrier)
        {
            tlCar = 63 - tlCar;
            tlCar *= velocity + 0x80;
            tlCar = (midiVolume * tlCar) >> 15;
            tlCar = tlCar ^ 63;
        }

        if(do_modulator)
        {
            uint_fast32_t mod = tlCar;

            tlMod = 63 - tlMod;
            tlMod *= velocity + 0x80;

            if(m_volumeScale == VOLUME_APOGEE_FIXED || mode > 1)
                mod = tlMod; // Fix the AM voices bug

            // NOTE: Here is a bug of Apogee Sound System that makes modulator
            // to not work properly on AM instruments. The fix of this bug, you
            // need to replace the tlCar with tmMod in this formula.
            // Don't do the bug on 4-op voices.
            tlMod = (midiVolume * mod) >> 15;

            tlMod ^= 63;
        }
    }
    else if(m_volumeScale == VOLUME_DMX && mode <= 1)
    {
        tlCar = (63 - volume);

        if(do_modulator)
        {
            if(tlMod < tlCar)
                tlMod = tlCar;
        }
    }
    else if(m_volumeScale == VOLUME_9X)
    {
        if(do_carrier)
            tlCar += volume + s_w9x_sb16_volume_model[velocity >> 2];
        if(do_modulator)
            tlMod += volume + s_w9x_sb16_volume_model[velocity >> 2];

        if(tlCar > 0x3F)
            tlCar = 0x3F;
        if(tlMod > 0x3F)
            tlMod = 0x3F;
    }
    else if(m_volumeScale == VOLUME_9X_GENERIC_FM)
    {
        if(do_carrier)
            tlCar += volume + s_w9x_generic_fm_volume_model[velocity >> 2];
        if(do_modulator)
            tlMod += volume + s_w9x_generic_fm_volume_model[velocity >> 2];

        if(tlCar > 0x3F)
            tlCar = 0x3F;
        if(tlMod > 0x3F)
            tlMod = 0x3F;
    }
    else if(m_volumeScale == VOLUME_AIL)
    {
        uint_fast32_t v0_val = (~srcMod) & 0x3f;
        uint_fast32_t v1_val = (~srcCar) & 0x3f;

        if(do_modulator)
            v0_val = (v0_val * midiVolume) / 127;
        if(do_carrier)
            v1_val = (v1_val * midiVolume) / 127;

        tlMod = (~v0_val) & 0x3F;
        tlCar = (~v1_val) & 0x3F;
    }
    else if(m_volumeScale == VOLUME_HMI)
    {
        uint_fast32_t vol;

        if(do_modulator)
        {
            vol = (64 - volume) << 1;
            vol *= (64 - tlMod);
            tlMod = (8192 - vol) >> 7;
        }

        if(do_carrier)
        {
            vol = (64 - volume) << 1;
            vol *= (64 - tlCar);
            tlCar = (8192 - vol) >> 7;
        }
    }
    else if(m_volumeScale == VOLUME_HMI_OLD)
    {
        uint_fast32_t vol;

        if(m_patch.OPS[i].feedconn == 0 && !isDrum)
        {
            vol = (ccvolume * ccexpr * 64) / 16129;
            vol = (((vol * 128) / 127) * velocity) >> 7;
            vol = s_hmi_volume_table[vol >> 1];

            vol = (64 - vol) << 1;
            vol *= (64 - tlCar);
            tlMod = (8192 - vol) >> 7;
        }

        if(isDrum) // TODO: VERIFY A CORRECTNESS OF THIS!!!
            vol = s_hmi_volume_table[velocity >> 1];

        vol = (64 - volume) << 1;
        vol *= (64 - tlCar);
        tlCar = (8192 - vol) >> 7;
    }
    else
    {
        if(do_modulator)
            tlMod = 63 - volume + (volume * tlMod) / 63;
        if(do_carrier)
            tlCar = 63 - volume + (volume * tlCar) / 63;
    }

    if(brightness != 127)
    {
        brightness = brightnessToOPL(brightness);
        if(!do_modulator)
            tlMod = 63 - brightness + (brightness * tlMod) / 63;
        if(!do_carrier)
            tlCar = 63 - brightness + (brightness * tlCar) / 63;
    }

    modulator = (kslMod & 0xC0) | (tlMod & 63);
    carrier = (kslCar & 0xC0) | (tlCar & 63);

    if(o1 != 0xFFF)
        WriteReg(0x40 + o1, static_cast<uint8_t>(modulator));
    if(o2 != 0xFFF)
        WriteReg(0x40 + o2, static_cast<uint8_t>(carrier));

    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void Generator::Patch(uint32_t c, uint32_t i)
{
    uint32_t cc = c % 23;
    static const uint16_t data[4] = {0x20, 0x60, 0x80, 0xE0};
    m_ins[c] = static_cast<uint16_t>(i);
    uint16_t o1 = g_Operators[cc * 2 + 0],
             o2 = g_Operators[cc * 2 + 1];
    uint32_t x = m_patch.OPS[i].modulator_E862, y = m_patch.OPS[i].carrier_E862;

    for(uint32_t a = 0; a < 4; ++a, x >>= 8, y >>= 8)
    {
        if(o1 != 0xFFF)
            WriteReg(data[a] + o1, x & 0xFF);
        if(o2 != 0xFFF)
            WriteReg(data[a] + o2, y & 0xFF);
    }
}

void Generator::Pan(uint32_t c, uint32_t value)
{
    uint8_t cc = c % 23;
    if(g_Channels_pan[cc] != 0xFFF)
        WriteReg(0xC0 + g_Channels_pan[cc], static_cast<uint8_t>(m_patch.OPS[m_ins[c]].feedconn | value));
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

    if(pseudo_4op || natural_4op)
        Patch(adlchannel[1], i[1]);

    Pan(adlchannel[0], 0x30);

    if(pseudo_4op || natural_4op)
        Pan(adlchannel[1], 0x30);

    touchNote(adlchannel[0], channel.volume, channel.ccvolume, channel.ccexpr);

    if(pseudo_4op || natural_4op)
        touchNote(adlchannel[1], channel.volume, channel.ccvolume, channel.ccexpr);

    bend  = m_bend + m_patch.OPS[i[0]].finetune;
    NoteOn(adlchannel[0], adlchannel[1], (tone + bend + phase));

    if(pseudo_4op)
    {
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

    memset(&m_four_op_category, 0, sizeof(m_four_op_category));

    if(rythmModePercussionMode == 0)
    {
        for(size_t b = 0; b < 23; ++b)
        {
            m_four_op_category[b] =
                (b >= 18) ? ChanCat_Rhythm_Slave : ChanCat_Regular;
        }
    }
    else
    {
        for(size_t b = 0; b < 23; ++b)
        {
            m_four_op_category[b] =
                (b >= 18) ? static_cast<ChanCat>(ChanCat_Rhythm_Bass + (b - 18)) :
                (b >= 6 && b < 9) ? ChanCat_Rhythm_Slave : ChanCat_Regular;
        }
    }

    if(enabled && (m_chipType == OPLChipBase::CHIPTYPE_OPL3))
    {
        //Enable 4-operators mode
        WriteReg(0x104, 0xFF);
        uint32_t fours = 6;
        uint32_t nextfour = 0;

        for(uint32_t a = 0; a < fours; ++a)
        {
            m_four_op_category[nextfour    ] = ChanCat_4op_Master;
            m_four_op_category[nextfour + 3] = ChanCat_4op_Slave;

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
        for(uint32_t a = 0; a < 18; ++a)
            m_four_op_category[a] = 0;
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

void Generator::generate(int16_t *frames, unsigned nframes)
{
    chip->generate(frames, nframes);
    // 2x Gain by default
    for(size_t i = 0; i < nframes * 2; ++i)
        frames[i] *= 2;
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
        chan = cycle++;
        // Rotate cycles
        if(cycle == channels.size())
            cycle = 0;

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
