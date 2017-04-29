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

#include "sb_ibk.h"
#include "../common.h"

/**
 * @brief Reader and Writer of the Sound Blaster IBK Bank format
 */
class SbIBK_impl : public FmBankFormatBase
{
public:
    static bool detectIBK(char* magic);
    static bool detectSBI(char* magic);
    static bool detectUNIXO2(QString filePath, Formats &format);
    static bool detectUNIXO3(QString filePath, Formats &format);
    // IBK/SBI for DOS
    static int  loadFileF(QString filePath, FmBank &bank);
    static int  loadFileInstF(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0);
    static int  saveFileF(QString filePath, FmBank &bank);
    // SB/O3 for UNIX
    static int  loadFileSBOP(QString filePath, FmBank &bank, Formats &format);
    static int  saveFileSBOP(QString filePath, FmBank &bank, bool fourOp = false);
};

// DOS SBK and SBI
static const char *sbi_magic = "SBI\x1A";
static const char *ibk_magic = "IBK\x1A";

// UNIX SB and O3
static const char *top_magic = "2OP\x1A";
static const char *fop_magic = "4OP\x1A";
static const char *zero_magic = "\0\0\0\0";

bool SbIBK_impl::detectIBK(char *magic)
{
    return (strncmp(magic, ibk_magic, 4) == 0);
}

bool SbIBK_impl::detectSBI(char *magic)
{
    return (strncmp(magic, sbi_magic, 4) == 0);
}

bool SbIBK_impl::detectUNIXO2(QString filePath, Formats &format)
{
    if(hasExt(filePath, ".sb"))
        return true;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.bytesAvailable();
    file.close();
    format = FORMAT_SB2OP;
    return (fileSize == 6656);
}

bool SbIBK_impl::detectUNIXO3(QString filePath, Formats &format)
{
    if(hasExt(filePath, ".o3"))
        return true;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.bytesAvailable();
    file.close();
    format = FORMAT_SB4OP;
    return (fileSize == 7680);
}

static void raw2sbi(FmBank::Instrument &ins, unsigned char *idata, bool fourOp = false)
{
    int MODULATOR   = fourOp ? MODULATOR2 : MODULATOR1;
    int CARRIER     = fourOp ? CARRIER2 : CARRIER1;
    ins.setAVEKM(MODULATOR,    idata[0]);//36   //47
    ins.setAVEKM(CARRIER,      idata[1]);//37   //48
    //            BYTE modscal;   /* KSL, TL */
    //            BYTE carscal;
    ins.setKSLL(MODULATOR,     idata[2]);//38   //49
    ins.setKSLL(CARRIER,       idata[3]);//39   //50
    //            BYTE modad;     /* Attack/Decay */
    //            BYTE carad;
    ins.setAtDec(MODULATOR,    idata[4]);//40   //51
    ins.setAtDec(CARRIER,      idata[5]);//41   //52
    //            BYTE modsr;     /* Sustain/Release */
    //            BYTE carsr;
    ins.setSusRel(MODULATOR,   idata[6]);//42   //53
    ins.setSusRel(CARRIER,     idata[7]);//43   //54
    //            BYTE modwave;   /* Wave Select */
    //            BYTE carwave;
    ins.setWaveForm(MODULATOR, idata[8]);//44   //55
    ins.setWaveForm(CARRIER,   idata[9]);//45   //56

    //            BYTE feedback;  /* FB, Connection *(inverse of Adlib)* <- not true? */
    //                            /* The following was originally reserved...CL uses  */
    //                            /* the next byte the same way we do: BD=6,SD=7,TT=8 */
    //                            /* CY=9,HH=10                                       */
    if(fourOp)
        ins.setFBConn2(idata[10]);//46  //57
    else
        ins.setFBConn1(idata[10]);//46  //57

    //            BYTE percvoc;   /* Percussion voice number                    : JWO */
    if(!fourOp)
    {
        ins.adlib_drum_number  = idata[11];//47 //58

    //            char transpos;  /* Number of notes to transpose timbre, signed: JWO */
        ins.note_offset1 = char_p(idata)[12];//48   //59

    //            BYTE dpitch;    /* percussion pitch: MIDI Note 0 - 127        : JWO */
        ins.percNoteNum  = idata[13];//49
    }
    //            BYTE rsv[2];    /* unsused - so far */
    //            } SBTIMBRE;
}

static void sbi2raw(unsigned char *odata, FmBank::Instrument &ins, bool fourOp = false)
{
    int MODULATOR   = fourOp ? MODULATOR2  : MODULATOR1;
    int CARRIER     = fourOp ? CARRIER2    : CARRIER1;
    odata[0] = ins.getAVEKM(MODULATOR);
    odata[1] = ins.getAVEKM(CARRIER);
    odata[2] = ins.getKSLL(MODULATOR);
    odata[3] = ins.getKSLL(CARRIER);
    odata[4] = ins.getAtDec(MODULATOR);
    odata[5] = ins.getAtDec(CARRIER);
    odata[6] = ins.getSusRel(MODULATOR);
    odata[7] = ins.getSusRel(CARRIER);
    odata[8] = ins.getWaveForm(MODULATOR);
    odata[9] = ins.getWaveForm(CARRIER);

    if(fourOp)
        odata[10] = ins.getFBConn2();
    else
        odata[10] = ins.getFBConn1();

    odata[11] = ins.adlib_drum_number;
    char *sodata = char_p(odata);

    if(fourOp)
        sodata[12]  = char(ins.note_offset2);
    else
        sodata[12]  = char(ins.note_offset1);

    odata[13]   = ins.percNoteNum;
    //Two reserved bytes!
    odata[14] = 0;
    odata[15] = 0;
}


int SbIBK_impl::loadFileF(QString filePath, FmBank &bank)
{
    char magic[4];
    memset(magic, 0, 4);
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();

    if(file.read(magic, 4) != 4)
        return ERR_BADFORMAT;

    if(strncmp(magic, ibk_magic, 4) != 0)
        return ERR_BADFORMAT;

    bool drumFlags[128];
    memset(drumFlags, 0, sizeof(bool) * 128);

    for(unsigned short i = 0; i < 128; i++)
    {
        unsigned char   idata[16];

        if(file.read(char_p(idata), 16) != 16)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        char tempName[10];
        sprintf(tempName, "NONAME%03d", i);
        strncpy(bank.Ins_Melodic[i].name, tempName, 9);
        strncpy(bank.Ins_Percussion[i].name, tempName, 9);
        FmBank::Instrument &ins = (idata[11] == 0x00) ? bank.Ins_Melodic[i] : bank.Ins_Percussion[i];
        drumFlags[i] = (idata[11] != 0x00);
        raw2sbi(ins, idata, false);
    }

    //fetch bank names
    for(unsigned short i = 0; i < 128; i++)
    {
        FmBank::Instrument &ins = drumFlags[i] ?
                                  bank.Ins_Percussion[i] :
                                  bank.Ins_Melodic[i];

        if(file.read(ins.name, 9) != 9)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
    }

    file.close();
    //    typedef struct {                     /* 3204 Bytes (0x0C83) */
    //            char     sig[4];             /* signature: "IBK\x1A"  */
    //            SBTIMBRE snd[128];           /* Instrument block */
    //            char     name[128][9];       /* name block: NUL terminated strings */
    //            } IBKFMT;
    return ERR_OK;
}

int SbIBK_impl::loadFileInstF(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    char magic[4];
    memset(magic, 0, 4);
    memset(&inst, 0, sizeof(FmBank::Instrument));
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bool isExtended = file.bytesAvailable() > 52;
    Q_UNUSED(isExtended);

    if(file.read(magic, 4) != 4)
        return ERR_BADFORMAT;

    if(strncmp(magic, sbi_magic, 4) != 0)
        return ERR_BADFORMAT;

    //char tempName[32];
    //sprintf(tempName, "NONAME%03d", 0);
    //strncpy(inst.name, tempName, 32 );
    if(file.read(inst.name, 32) != 32)
        return ERR_BADFORMAT;

    bool drumFlag = false;
    unsigned char   idata[16];

    if(file.read(char_p(idata), 16) != 16)
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return ERR_BADFORMAT;
    }

    FmBank::Instrument &ins = inst;
    drumFlag = (idata[11] != 0x00);
    raw2sbi(ins, idata, false);

    if(isDrum)
        *isDrum = drumFlag;

    file.close();
    return ERR_OK;
}

int SbIBK_impl::saveFileF(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return ERR_NOFILE;

    bool drumFlags[128];
    memset(drumFlags, 0, sizeof(bool) * 128);
    //Write header
    file.write(char_p(ibk_magic), 4);

    for(unsigned short i = 0; i < 128; i++)
    {
        drumFlags[i] = (bank.Ins_Percussion[i].adlib_drum_number != 0);
        FmBank::Instrument &ins = drumFlags[i] ?
                                  bank.Ins_Percussion[i] :
                                  bank.Ins_Melodic[i];
        unsigned char   odata[16];
        memset(odata, 0, 16);
        sbi2raw(odata, ins, false);

        if(file.write(char_p(&odata), 16) != 16)
            return ERR_BADFORMAT;
    }

    //store bank names
    for(unsigned short i = 0; i < 128; i++)
    {
        FmBank::Instrument &ins = drumFlags[i] ?
                                  bank.Ins_Percussion[i] :
                                  bank.Ins_Melodic[i];

        if(file.write(ins.name, 9) != 9)
            return ERR_BADFORMAT;
    }

    file.close();
    return ERR_OK;
}


int SbIBK_impl::loadFileSBOP(QString filePath, FmBank &bank, Formats &format)
{
    char magic[4];
    bool valid = false;
    bool is4op = false;
    memset(magic, 0, 4);
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();

    qint64  fileSize = file.bytesAvailable();
    bool    fileIs4op = (fileSize == 7680);
    bool    fileIsPercussion = false;
    format = fileIs4op ? FORMAT_SB4OP : FORMAT_SB2OP;
    for(uint16_t i = 0; i < 128; i++)
    {
        uint8_t idata1[16];
        uint8_t idata2[16];
        char    tempName[32];

        if(file.read(magic, 4) != 4)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        if(strncmp(magic, zero_magic, 4) == 0)
        {
            //Skip empty instrument
            file.seek(file.pos() + (fileIs4op ? 56 : 48));
            fileIsPercussion = true; //Percussion banks are always begins from zero instruments
            continue;
        }

        is4op =  (strncmp(magic, fop_magic, 4) == 0);
        valid |= is4op;
        valid |= (strncmp(magic, top_magic, 4) == 0);
        valid |= (strncmp(magic, sbi_magic, 4) == 0);

        if(!valid)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        if(file.read(tempName, 32) != 32)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        memset(idata1, 0, 16);
        memset(idata2, 0, 16);

        if(!fileIs4op && file.read(char_p(idata1), 16) != 16)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        if(fileIs4op && ((file.read(char_p(idata1), 11) != 11) || (file.read(char_p(idata2), 13) != 13)))
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        FmBank::Instrument &ins = fileIsPercussion ? bank.Ins_Percussion[i] : bank.Ins_Melodic[i];
        strncpy(ins.name, tempName, 15);
        /*
         0-15: voice name
         16-24: unused
         25: echo delay
         26: echo attenuation
         27: chorus spread
         28: transpose
         29: fixed duration
         30: extra voice
         31: fixed key
        */
        idata1[11] = 0;
        ins.en_4op = is4op;
        raw2sbi(ins, idata1, false);
        if(is4op)
            raw2sbi(ins, idata2, true);
        //???? Very weird acting: in 2-op banks it is usually 64, in 4-op - zero
        //ins.note_offset1 = fileIsPercussion ? 0 :tempName[28];
        ins.percNoteNum = uchar(tempName[31]);
    }
    file.close();
    return ERR_OK;
}

int SbIBK_impl::saveFileSBOP(QString, FmBank &, bool)
{
    return ERR_NOT_IMLEMENTED;
}






SbIBK_DOS::SbIBK_DOS() : FmBankFormatBase()
{}

bool SbIBK_DOS::detect(const QString &, char *magic)
{
    return SbIBK_impl::detectIBK(magic);
}

bool SbIBK_DOS::detectInst(const QString &, char *magic)
{
    return SbIBK_impl::detectSBI(magic);
}

int SbIBK_DOS::loadFile(QString filePath, FmBank &bank)
{
   return SbIBK_impl::loadFileF(filePath, bank);
}

int SbIBK_DOS::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    return SbIBK_impl::loadFileInstF(filePath, inst, isDrum);
}



int SbIBK_DOS::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileF(filePath, bank);
}

int SbIBK_DOS::formatCaps()
{
    return FORMAT_CAPS_EVERYTHING;
}

int SbIBK_DOS::formatInstCaps()
{
    return FORMAT_CAPS_OPEN|FORMAT_CAPS_IMPORT;
}

QString SbIBK_DOS::formatInstName()
{
    return "Sound Blaster Instrument";
}

QString SbIBK_DOS::formatInstExtensionMask()
{
    return "*.sbi";
}

FmBankFormatBase::InsFormats SbIBK_DOS::formatInstId()
{
    return FORMAT_INST_SBI;
}

QString SbIBK_DOS::formatName()
{
    return "Sound Blaster IBK file";
}

QString SbIBK_DOS::formatExtensionMask()
{
    return "*.ibk";
}

FmBankFormatBase::Formats SbIBK_DOS::formatId()
{
    return FORMAT_IBK;
}



SbIBK_UNIX_READ::SbIBK_UNIX_READ() : FmBankFormatBase()
{}

bool SbIBK_UNIX_READ::detect(const QString &filePath, char *)
{
    bool ret = false;
    ret = SbIBK_impl::detectUNIXO2(filePath, m_recentFormat);
    if(!ret)
        ret = SbIBK_impl::detectUNIXO3(filePath, m_recentFormat);
    return ret;
}

int SbIBK_UNIX_READ::loadFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::loadFileSBOP(filePath, bank, m_recentFormat);
}

int SbIBK_UNIX_READ::formatCaps()
{
    return FORMAT_CAPS_OPEN|FORMAT_CAPS_IMPORT;
}

QString SbIBK_UNIX_READ::formatName()
{
    return "SoundBlaster UNIX 2 or 4 operators bank";
}

QString SbIBK_UNIX_READ::formatExtensionMask()
{
    return "*.sb *.o3";
}

FmBankFormatBase::Formats SbIBK_UNIX_READ::formatId()
{
    return m_recentFormat;
}




SbIBK_UNIX2OP_SAVE::SbIBK_UNIX2OP_SAVE() : FmBankFormatBase()
{}

int SbIBK_UNIX2OP_SAVE::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileSBOP(filePath, bank, false);
}

int SbIBK_UNIX2OP_SAVE::formatCaps()
{
    return FORMAT_CAPS_SAVE;
}

QString SbIBK_UNIX2OP_SAVE::formatName()
{
    return "SoundBlaster UNIX 2-operators bank";
}

QString SbIBK_UNIX2OP_SAVE::formatExtensionMask()
{
    return "*.sb";
}

FmBankFormatBase::Formats SbIBK_UNIX2OP_SAVE::formatId()
{
    return FORMAT_SB2OP;
}




SbIBK_UNIX4OP_SAVE::SbIBK_UNIX4OP_SAVE() : FmBankFormatBase()
{}

int SbIBK_UNIX4OP_SAVE::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileSBOP(filePath, bank, true);
}

int SbIBK_UNIX4OP_SAVE::formatCaps()
{
    return FORMAT_CAPS_SAVE;
}

QString SbIBK_UNIX4OP_SAVE::formatName()
{
    return "SoundBlaster UNIX 4-operators bank";
}

QString SbIBK_UNIX4OP_SAVE::formatExtensionMask()
{
    return "*.o3";
}

FmBankFormatBase::Formats SbIBK_UNIX4OP_SAVE::formatId()
{
    return FORMAT_SB4OP;
}


