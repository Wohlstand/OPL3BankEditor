#ifndef BANK_H
#define BANK_H

#define CARRIER1    1
#define MODULATOR1  0
#define CARRIER2    3
#define MODULATOR2  2

class FmBank
{
public:
    FmBank();
    FmBank(const FmBank &fb);
    void reset();

    struct Operator
    {
        unsigned char attack;
        unsigned char decay;
        unsigned char sustain;
        unsigned char release;
        unsigned char waveform;
        unsigned char fmult;
        unsigned char level;
        unsigned char ksl;
        bool vib;
        bool am;
        bool eg;
        bool ksr;
    };

    struct Instrument
    {
        Operator OP[4];
        unsigned char feedback1;
        unsigned char feedback2;
        enum Connections
        {
            AM = true,
            FM = false
        };
        bool connection1;
        bool connection2;
        unsigned char percNoteNum;
        bool en_4op;
    };

    Instrument Ins_Melodic[128];
    Instrument Ins_Percussion[128];
};

#endif // BANK_H
