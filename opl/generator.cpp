#include "generator.h"
#include <qendian.h>
//#include <QtDebug>

const int DataSampleRateHz = 44100;
const int BufferSize      = 4096;

static const unsigned short Operators[23*2] =
    {0x000,0x003,0x001,0x004,0x002,0x005, // operators  0, 3,  1, 4,  2, 5
     0x008,0x00B,0x009,0x00C,0x00A,0x00D, // operators  6, 9,  7,10,  8,11
     0x010,0x013,0x011,0x014,0x012,0x015, // operators 12,15, 13,16, 14,17
     0x100,0x103,0x101,0x104,0x102,0x105, // operators 18,21, 19,22, 20,23
     0x108,0x10B,0x109,0x10C,0x10A,0x10D, // operators 24,27, 25,28, 26,29
     0x110,0x113,0x111,0x114,0x112,0x115, // operators 30,33, 31,34, 32,35
     0x010,0x013,   // operators 12,15
     0x014,0xFFF,   // operator 16
     0x012,0xFFF,   // operator 14
     0x015,0xFFF,   // operator 17
     0x011,0xFFF }; // operator 13

static const unsigned short Channels[23] =
    {0x000,0x001,0x002, 0x003,0x004,0x005, 0x006,0x007,0x008, // 0..8
     0x100,0x101,0x102, 0x103,0x104,0x105, 0x106,0x107,0x108, // 9..17 (secondary set)
     0x006,0x007,0x008,0xFFF,0xFFF }; // <- hw percussions, 0xFFF = no support for pitch/pan

unsigned char regBD = 0;
//unsigned char pit = 0;

const unsigned NumChannels = 23;

char four_op_category[NumChannels];
                            // 1 = quad-master, 2 = quad-slave, 0 = regular
                            // 3 = percussion BassDrum
                            // 4 = percussion Snare
                            // 5 = percussion Tom
                            // 6 = percussion Crash cymbal
                            // 7 = percussion Hihat
                            // 8 = percussion slave

//unsigned short ins[NumChannels];
unsigned short g_ins[NumChannels]; // index to adl[], cached, needed by Touch()
unsigned char  pit[NumChannels];  // value poked to B0, cached, needed by NoteOff)(

Generator::Generator(int sampleRate,
                     QObject *parent)
    :   QIODevice(parent)
{

    note = 57;

    m_patch =
    {
        {
            { 0x104C060,0x10455B1, 0x51,0x80, 0x4, +12 },
            { 0x10490A0,0x1045531, 0x52,0x80, 0x6, +12 },
        },
        0,
        OPL_PatchSetup::Flag_Pseudo4op,
        40000,
        0,
        -0.125000
    };

    memset(g_ins, 0, sizeof(unsigned short)*NumChannels);
    memset(pit, 0, sizeof(unsigned char)*NumChannels);
    memset(four_op_category, 0, NumChannels);
    unsigned p=0;
    for(unsigned b=0; b<18; ++b) four_op_category[p++] = 0;
    for(unsigned b=0; b< 5; ++b) four_op_category[p++] = 8;

    static const short data[] =
    { 0x004,96, 0x004,128,        // Pulse timer
      0x105, 0, 0x105,1, 0x105,0, // Pulse OPL3 enable
      0x001,32, 0x105,1           // Enable wave, OPL3 extensions
    };

    chip.Init(sampleRate);

    for(unsigned a=0; a< 18; ++a) chip.WriteReg(0xB0+Channels[a], 0x00);
    for(unsigned a=0; a< sizeof(data)/sizeof(*data); a+=2)
        chip.WriteReg(data[a], data[a+1]);


    DeepTremoloMode   = 0;
    DeepVibratoMode   = 0;
    unsigned char AdlPercussionMode = 0;

    chip.WriteReg(0x0BD, regBD = (DeepTremoloMode*0x80
                                + DeepVibratoMode*0x40
                                + AdlPercussionMode*0x20) );

    unsigned fours = 7;

    unsigned fours_this_card = std::min(fours, 6u);
    chip.WriteReg(0x104, (1 << fours_this_card) - 1);

    unsigned nextfour = 0;
    for(unsigned a=0; a<fours; ++a)
    {
        four_op_category[nextfour  ] = 1;
        four_op_category[nextfour+3] = 2;
        switch(a % 6)
        {
            case 0: case 1: nextfour += 1; break;
            case 2:         nextfour += 9-2; break;
            case 3: case 4: nextfour += 1; break;
            case 5:         nextfour += 23-9-2; break;
        }
    }

    //Shutup!
    for(unsigned c=0; c<NumChannels; ++c)
    {
        NoteOff(c);
        Touch_Real(c, 0);
    }
}

Generator::~Generator()
{}

void Generator::NoteOff(unsigned c)
{
    unsigned cc = c%23;
    if(cc >= 18)
    {
        regBD &= ~(0x10 >> (cc-18));
        chip.WriteReg(0xBD, regBD);
        return;
    }
    chip.WriteReg(0xB0 + Channels[cc], pit[c] & 0xDF);
}

void Generator::NoteOn(unsigned c, double hertz) // Hertz range: 0..131071
{
    unsigned cc = c%23;
    unsigned x = 0x2000;
    if(hertz < 0 || hertz > 131071) // Avoid infinite loop
        return;
    while(hertz >= 1023.5) { hertz /= 2.0; x += 0x400; } // Calculate octave
    x += (int)(hertz + 0.5);
    unsigned chn = Channels[cc];
    if(cc >= 18)
    {
        regBD |= (0x10 >> (cc-18));
        chip.WriteReg(0x0BD, regBD);
        x &= ~0x2000;
        //x |= 0x800; // for test
    }
    if(chn != 0xFFF)
    {
        chip.WriteReg(0xA0 + chn, x & 0xFF);
        chip.WriteReg(0xB0 + chn, pit[c] = x >> 8);
    }
}

void Generator::Touch_Real(unsigned c, unsigned volume)
{
    if(volume > 63) volume = 63;
    unsigned /*card = c/23,*/ cc = c%23;
    unsigned i = g_ins[c], o1 = Operators[cc*2], o2 = Operators[cc*2+1];
    unsigned x = m_patch.OPS[i].modulator_40,
             y = m_patch.OPS[i].carrier_40;
    bool do_modulator;
    bool do_carrier;

    unsigned mode = 1; // 2-op AM
    if(four_op_category[c] == 0 || four_op_category[c] == 3)
    {
        mode = m_patch.OPS[i].feedconn & 1; // 2-op FM or 2-op AM
    }
    else if(four_op_category[c] == 1 || four_op_category[c] == 2)
    {
        unsigned i0, i1;
        if ( four_op_category[c] == 1 )
        {
            i0 = i;
            i1 = g_ins[c + 3];
            mode = 2; // 4-op xx-xx ops 1&2
        }
        else
        {
            i0 = g_ins[c - 3];
            i1 = i;
            mode = 6; // 4-op xx-xx ops 3&4
        }
        mode += (m_patch.OPS[i0].feedconn & 1) + (m_patch.OPS[i1].feedconn & 1) * 2;
    }
    static const bool do_ops[10][2] =
      { { false, true },  /* 2 op FM */
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

    chip.WriteReg(0x40+o1, do_modulator ? (x|63) - volume + volume*(x&63)/63 : x);
    if(o2 != 0xFFF)
    chip.WriteReg(0x40+o2, do_carrier   ? (y|63) - volume + volume*(y&63)/63 : y);
    // Correct formula (ST3, AdPlug):
    //   63-((63-(instrvol))/63)*chanvol
    // Reduces to (tested identical):
    //   63 - chanvol + chanvol*instrvol/63
    // Also (slower, floats):
    //   63 + chanvol * (instrvol / 63.0 - 1)
}

void Generator::Touch(unsigned c, unsigned volume) // Volume maxes at 127*127*127
{
    // The formula below: SOLVE(V=127^3 * 2^( (A-63.49999) / 8), A)
    Touch_Real(c, volume>8725  ? std::log(volume)*11.541561 + (0.5 - 104.22845) : 0);
    // The incorrect formula below: SOLVE(V=127^3 * (2^(A/63)-1), A)
    //Touch_Real(c, volume>11210 ? 91.61112 * std::log(4.8819E-7*volume + 1.0)+0.5 : 0);
}

void Generator::Patch(unsigned c, unsigned i)
{
    unsigned cc = c%23;
    static const unsigned char data[4] = {0x20,0x60,0x80,0xE0};
    g_ins[c] = i;
    unsigned o1 = Operators[cc*2+0], o2 = Operators[cc*2+1];
    unsigned x = m_patch.OPS[i].modulator_E862, y = m_patch.OPS[i].carrier_E862;
    for(unsigned a=0; a<4; ++a)
    {
        chip.WriteReg(data[a]+o1, x&0xFF); x>>=8;
        if(o2 != 0xFFF)
        chip.WriteReg(data[a]+o2, y&0xFF); y>>=8;
    }
}

void Generator::Pan(unsigned c, unsigned value)
{
    unsigned cc = c%23;
    if(Channels[cc] != 0xFFF)
        chip.WriteReg(0xC0 + Channels[cc], m_patch.OPS[g_ins[c]].feedconn | value);
}

void Generator::PlayNoteF(int noteID, int chan2op1, int chan2op2, int chan4op1, int chan4op2)
{
    int tone = noteID;
    if(m_patch.tone)
    {
        if(m_patch.tone < 20)
            tone += m_patch.tone;
        else if(m_patch.tone < 128)
            tone = m_patch.tone;
        else
            tone -= m_patch.tone-128;
    }
    int i[2] = { 0, 1 };
    bool pseudo_4op  = m_patch.flags & OPL_PatchSetup::Flag_Pseudo4op;
    bool natural_4op = m_patch.flags & OPL_PatchSetup::Flag_True4op;

    int  adlchannel[2] = { chan4op1, chan4op2 };
    if(/*i[0] == i[1]*/!natural_4op || pseudo_4op )
    {
        adlchannel[0] = chan2op1;
        adlchannel[1] = chan2op2;
    }

    g_ins[adlchannel[0]] = i[0];
    g_ins[adlchannel[1]] = i[1];

    //0x0014F671,0x0007F131, 0x92,0x00, 0x2, +0
    double bend = 0.0;
    double phase = 0.0;

    Patch(adlchannel[0], i[0]);
    Patch(adlchannel[1], i[1]);

    Pan(adlchannel[0], 0x30);
    Pan(adlchannel[1], 0x30);

    Touch_Real(adlchannel[0], 63);
    Touch_Real(adlchannel[1], 63);

    bend  = 0.0 + m_patch.OPS[i[0]].finetune;
    NoteOn(adlchannel[0], 172.00093 * std::exp(0.057762265 * (tone + bend + phase)));

    if( pseudo_4op )
    {
        bend  = 0.0 + m_patch.OPS[i[1]].finetune + m_patch.voice2_fine_tune;
        NoteOn(adlchannel[1], 172.00093 * std::exp(0.057762265 * (tone + bend + phase)));
    }
}

void Generator::MuteNote()
{
    for(unsigned c=0; c<NumChannels; ++c)
    {
        NoteOff(c);
    }
}

void Generator::PlayNote()
{
    int channels1[3]     = {7, 15, 17};
    int channels2[3]     = {6,  8, 16};
    int channels1_4op[3] = {1,  2, 9};
    int channels2_4op[3] = {4,  5, 12};
    static int chan = 0;

    PlayNoteF(note, channels1[chan],  channels2[chan],  channels1_4op[chan],  channels2_4op[chan]);

    /* Rotating channels to have nicer poliphony on key spam */
    chan++; if(chan>2) chan=0;
}

void Generator::PlayMajorChord()
{
    PlayNoteF(note,   7,  6,    1,4);
    PlayNoteF(note+4, 15, 8,    2,5);
    PlayNoteF(note-5, 17, 16,   9,12);
}

void Generator::PlayMinorChord()
{
    PlayNoteF(note,   7,  6,    1,4);
    PlayNoteF(note+3, 15, 8,    2,5);
    PlayNoteF(note-5, 17, 16,   9,12);
}

void Generator::changePatch(const FmBank::Instrument &instrument, bool isDrum)
{
    //Shutup everything
    for(unsigned c=0; c < NumChannels; ++c)
    {
        NoteOff(c);
    }

    /*
        "const adldata adl[%u] =\n"
        "{ //    ,---------+-------- Wave select settings\n"
        "  //    | ,-------ч-+------ Sustain/release rates\n"
        "  //    | | ,-----ч-ч-+---- Attack/decay rates\n"
        "  //    | | | ,---ч-ч-ч-+-- AM/VIB/EG/KSR/Multiple bits\n"
        "  //    | | | |   | | | |\n"
        "  //    | | | |   | | | |     ,----+-- KSL/attenuation settings\n"
        "  //    | | | |   | | | |     |    |    ,----- Feedback/connection bits\n"
        "  //    | | | |   | | | |     |    |    |\n"
    */
//    unsigned int  modulator_E862, carrier_E862;  // See below
//    unsigned char modulator_40, carrier_40; // KSL/attenuation settings
//    unsigned char feedconn; // Feedback/connection bits for the channel
//    signed char   finetune;
    /*
    (i->first.data[6] << 24)
  + (i->first.data[4] << 16)
  + (i->first.data[2] << 8)
  + (i->first.data[0] << 0);
    */
    m_patch.OPS[0].modulator_E862 =
                 (uint(instrument.OP[MODULATOR1].waveform) << 24)
                |(uint( (0xF0&(uchar(0x0F-instrument.OP[MODULATOR1].sustain)<<4))
                       | (0x0F & instrument.OP[MODULATOR1].release) ) << 16)

                |(uint( (0xF0&uchar(instrument.OP[MODULATOR1].attack<<4))
                       |(0x0F&instrument.OP[MODULATOR1].decay)) << 8)
               |uint( ( 0x80 & (uchar(instrument.OP[MODULATOR1].am)<<7) )
                    | ( 0x40 & (uchar(instrument.OP[MODULATOR1].vib)<<6) )
                    | ( 0x20 & (uchar(instrument.OP[MODULATOR1].eg)<<5) )
                    | ( 0x10 & (uchar(instrument.OP[MODULATOR1].ksr)<<4) )
                    | ( 0x0F &  uchar(instrument.OP[MODULATOR1].fmult) ) );
    m_patch.OPS[0].modulator_40 = 0;
    m_patch.OPS[0].modulator_40 |= 0xC0 & (uchar(instrument.OP[MODULATOR1].ksl)<<6);
    m_patch.OPS[0].modulator_40 |= 0x3F & uchar(0x3F-instrument.OP[MODULATOR1].level);

    m_patch.OPS[0].carrier_E862 =
                 (uint(instrument.OP[CARRIER1].waveform) << 24)

            |(uint( (0xF0&(uchar(0x0F-instrument.OP[CARRIER1].sustain)<<4))
                   | (0x0F & instrument.OP[CARRIER1].release) ) << 16)

            |(uint( (0xF0&uchar(instrument.OP[CARRIER1].attack<<4))
                   |(0x0F&instrument.OP[CARRIER1].decay)) << 8)

           |uint( ( 0x80 & (uchar(instrument.OP[CARRIER1].am)<<7) )
                | ( 0x40 & (uchar(instrument.OP[CARRIER1].vib)<<6) )
                | ( 0x20 & (uchar(instrument.OP[CARRIER1].eg)<<5) )
                | ( 0x10 & (uchar(instrument.OP[CARRIER1].ksr)<<4) )
                | ( 0x0F &  uchar(instrument.OP[CARRIER1].fmult) ) );
    m_patch.OPS[0].carrier_40 = 0;
    m_patch.OPS[0].carrier_40 |= 0xC0 & (uchar(instrument.OP[CARRIER1].ksl) << 6);
    m_patch.OPS[0].carrier_40 |= 0x3F & uchar(0x3F-instrument.OP[CARRIER1].level);

    m_patch.OPS[0].feedconn  = 0;
    m_patch.OPS[0].feedconn |= uchar(instrument.connection1);
    m_patch.OPS[0].feedconn |= 0x0E & uchar(instrument.feedback1 << 1);


    m_patch.OPS[1].modulator_E862 =
                 (uint(instrument.OP[MODULATOR2].waveform) << 24)
                |(uint( (0xF0&(uchar(0x0F-instrument.OP[MODULATOR2].sustain)<<4))
                       | (0x0F & instrument.OP[MODULATOR2].release) ) << 16)

                |(uint( (0xF0&uchar(instrument.OP[MODULATOR2].attack<<4))
                       |(0x0F&instrument.OP[MODULATOR2].decay)) << 8)

           |uint( ( 0x80 & (uchar(instrument.OP[MODULATOR2].am)<<7) )
                | ( 0x40 & (uchar(instrument.OP[MODULATOR2].vib)<<6) )
                | ( 0x20 & (uchar(instrument.OP[MODULATOR2].eg)<<5) )
                | ( 0x10 & (uchar(instrument.OP[MODULATOR2].ksr)<<4) )
                | ( 0x0F &  uchar(instrument.OP[MODULATOR2].fmult) ) );
    m_patch.OPS[1].modulator_40 = 0;
    m_patch.OPS[1].modulator_40 |= 0xC0 & (uchar(instrument.OP[MODULATOR2].ksl)<<6);
    m_patch.OPS[1].modulator_40 |= 0x3F & uchar(0x3F-instrument.OP[MODULATOR2].level);

    m_patch.OPS[1].carrier_E862 =
                 (uint(instrument.OP[CARRIER2].waveform) << 24)
            |(uint( (0xF0&(uchar(0x0F-instrument.OP[CARRIER2].sustain)<<4))
                   | (0x0F & instrument.OP[CARRIER2].release) ) << 16)

            |(uint( (0xF0&uchar(instrument.OP[CARRIER2].attack<<4))
                   |(0x0F&instrument.OP[CARRIER2].decay)) << 8)

            | uint( ( 0x80 & (uchar(instrument.OP[CARRIER2].am)<<7) )
                  | ( 0x40 & (uchar(instrument.OP[CARRIER2].vib)<<6) )
                  | ( 0x20 & (uchar(instrument.OP[CARRIER2].eg)<<5) )
                  | ( 0x10 & (uchar(instrument.OP[CARRIER2].ksr)<<4) )
                  | ( 0x0F &  uchar(instrument.OP[CARRIER2].fmult) ) );
    m_patch.OPS[1].carrier_40 = 0;
    m_patch.OPS[1].carrier_40 |= 0xC0 & (uchar(instrument.OP[CARRIER2].ksl) << 6);
    m_patch.OPS[1].carrier_40 |= 0x3F & uchar(0x3F-instrument.OP[CARRIER2].level);

    m_patch.OPS[1].feedconn  = 0;
    m_patch.OPS[1].feedconn |= uchar(instrument.connection2);
    m_patch.OPS[1].feedconn |= 0x0E & uchar(instrument.feedback2 << 1);

    if(isDrum)
    {
        if(instrument.percNoteNum && instrument.percNoteNum < 20 )
        {
            uchar nnum = instrument.percNoteNum;
            while( nnum && nnum<20)
            {
                nnum += 12;
                m_patch.OPS[0].finetune -= 12;
                m_patch.OPS[1].finetune -= 12;
            }
        }
    }
    else
    {
        m_patch.OPS[0].finetune = 0;
    }

    m_patch.ms_sound_kon  = 40000;
    m_patch.ms_sound_koff = 0;
    m_patch.voice2_fine_tune = 0.0;
    m_patch.flags = 0;
    m_patch.tone = 0;

    if(instrument.en_4op)
    {
        m_patch.flags |= OPL_PatchSetup::Flag_True4op;
    }
}

void Generator::changeNote(int newnote)
{
    note = newnote;
}

void Generator::changeDeepTremolo(bool enabled)
{
    DeepTremoloMode   = uchar(enabled);
    unsigned char AdlPercussionMode = 0;

    chip.WriteReg(0x0BD, regBD = (DeepTremoloMode*0x80
                                + DeepVibratoMode*0x40
                                + AdlPercussionMode*0x20) );
}

void Generator::changeDeepVibrato(bool enabled)
{
    DeepVibratoMode   = uchar(enabled);
    unsigned char AdlPercussionMode = 0;

    chip.WriteReg(0x0BD, regBD = (DeepTremoloMode*0x80
                                + DeepVibratoMode*0x40
                                + AdlPercussionMode*0x20) );
}

void Generator::start()
{
    open(QIODevice::ReadOnly);
    saySomething("Meow");
}

void Generator::stop()
{
    close();
}

qint64 Generator::readData(char *data, qint64 len)
{
    short* _out = (short*)data;
    qint64 total = 0, lenS = (len/4);
    int samples[4096];
    if(lenS > BufferSize/4)
        lenS = BufferSize/4;
    unsigned long lenL = 512;
    //unsigned long lenBytes = lenL*4;
    //FILE* shit = fopen("C:/_Repos/OPL3BankEditor/_AudioOutputQtExample/shit.raw", "ab");
    while( (total+512) < lenS )
    {
        chip.GenerateArr(samples, &lenL);
        short out;
        int offset;
        for(unsigned long p = 0; p < lenL; ++p)
        {
            for(unsigned w=0; w<2; ++w)
            {
                out    = samples[p*2+w];
                offset = total+p*2+w;
                _out[offset] = out;
                //fwrite(&out, 1, 2, shit);
            }
        }
        total += lenL;
    };

    saySomething(QString::number(total*4));
    return total*4;
}

qint64 Generator::writeData(const char *data, qint64 len)
{
    Q_UNUSED(data);
    Q_UNUSED(len);
    return 0;
}

qint64 Generator::bytesAvailable() const
{
    return 2048;// + QIODevice::bytesAvailable();
}



//________________________Old_code________________________
//  adlchannel[ccount] = c;
//    for(unsigned ccount = 0; ccount < 2; ++ccount)
//    {
//        if(ccount == 1)
//        {
//            if(i[0] == i[1]) {/*twoChans=false;*/ break; } // No secondary channel
//            if(adlchannel[0] == -1) { /*twoChans=false;*/ break;} // No secondary if primary failed
//        }

//        int c = -1;
//        //long bs = -0x7FFFFFFFl;
//        for(int a = adlchannel[0]; a <= adlchannel[1]/*(int)NumChannels*/; a += 3)
//        {
//            if(ccount == 1 && a == adlchannel[0]) continue;
//            // ^ Don't use the same channel for primary&secondary

//            if(i[0] == i[1] || pseudo_4op)
//            {
//                // Only use regular channels
//                int expected_mode = 0;
//                if(four_op_category[a] != expected_mode)
//                    continue;
//            }
//            else
//            {
//                if(ccount == 0)
//                {
//                    // Only use four-op master channels
//                    if(four_op_category[a] != 1)
//                        continue;
//                }
//                else
//                {
//                    // The secondary must be played on a specific channel.
//                    if(a != adlchannel[0] + 3)
//                        continue;
//                }
//            }
//            //long s = CalculateAdlChannelGoodness(a, i[ccount], MidCh);
//            //if(s > bs) { bs=s; c = a; } // Best candidate wins
//            c = a;
//        }

//        if(c < 0)
//        {
//            //UI.PrintLn("ignored unplaceable note");
//            continue; // Could not play this note. Ignore it.
//        }
//        //PrepareAdlChannelForNewNote(c, i[ccount]);
//        g_ins[c] = i[ccount];
//        adlchannel[ccount] = c;
//    }
//    if(adlchannel[0] < 0 && adlchannel[1] < 0)
//    {
//        saySomething("crap");
//        // The note could not be played, at all.
//        //return;
//    }
