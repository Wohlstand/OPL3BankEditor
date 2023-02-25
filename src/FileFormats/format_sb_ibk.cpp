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

#include "format_sb_ibk.h"
#include "../common.h"
#include <QFileInfo>

/**
 * @brief Reader and Writer of the Sound Blaster IBK Bank format
 */
class SbIBK_impl : public FmBankFormatBase
{
public:
    static bool detectIBK(const char *magic);
    static bool detectSBI(const char *magic);
    static bool detectSBI4OP(const char *magic);
    static bool detectUNIXO2(QString filePath, BankFormats &format);
    static bool detectUNIXO3(QString filePath, BankFormats &format);
    // IBK/SBI for DOS
    static FfmtErrCode loadFileIBK(QString filePath, FmBank &bank);
    static FfmtErrCode saveFileIBK(QString filePath, FmBank &bank);
    static FfmtErrCode loadFileSBI(QString filePath, FmBank::Instrument &inst, bool *isDrum = nullptr);
    static FfmtErrCode saveFileSBI(QString filePath, FmBank::Instrument &inst, bool isDrum = false);
    // SB/O3 for UNIX
    static FfmtErrCode loadFileSBOP(QString filePath, FmBank &bank, BankFormats &format);
    static FfmtErrCode saveFileSBOP(QString filePath, FmBank &bank, bool fourOp = false, bool isDrum = false);
};

// DOS SBK and SBI
static const char *sbi_magic = "SBI\x1A";
static const char *ibk_magic = "IBK\x1A";

// UNIX SB and O3
static const char *top_magic = "2OP\x1A";
static const char *fop_magic = "4OP\x1A";
static const char *zero_magic = "\0\0\0\0";

// JuceOPLVSTi
static const char *vsti_magic = "SBI\x1D";
static const char vsti_inst_name[32] = "JuceOPLVSTi instrument         ";

bool SbIBK_impl::detectIBK(const char *magic)
{
    return (strncmp(magic, ibk_magic, 4) == 0);
}

bool SbIBK_impl::detectSBI(const char *magic)
{
    return (strncmp(magic, sbi_magic, 4) == 0) ||
           (strncmp(magic, vsti_magic, 4) == 0);
}

bool SbIBK_impl::detectSBI4OP(const char *magic)
{
    return (strncmp(magic, fop_magic, 4) == 0);
}


bool SbIBK_impl::detectUNIXO2(QString filePath, BankFormats &format)
{
    if(hasExt(filePath, ".sb"))
        return true;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.bytesAvailable();
    file.close();
    format = BankFormats::FORMAT_SB2OP;
    return (fileSize == 6656);
}

bool SbIBK_impl::detectUNIXO3(QString filePath, BankFormats &format)
{
    if(hasExt(filePath, ".o3"))
        return true;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.bytesAvailable();
    file.close();
    format = BankFormats::FORMAT_SB4OP;
    return (fileSize == 7680);
}

static void raw2sbi(FmBank::Instrument &ins, uint8_t *idata, bool fourOp = false, bool hasSbiExtraFields = true)
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

    if(!fourOp && hasSbiExtraFields)
    {
        //        BYTE percvoc;   /* Percussion voice number                    : JWO */
        ins.rhythm_drum_type  = idata[11];//47 //58

        //        char transpos;  /* Number of notes to transpose timbre, signed: JWO */
        ins.note_offset1 = char_p(idata)[12];//48   //59

        //        BYTE dpitch;    /* percussion pitch: MIDI Note 0 - 127        : JWO */
        ins.percNoteNum  = idata[13];//49
        ins.is_fixed_note = (ins.rhythm_drum_type != 0x00);
    }
    //            BYTE rsv[2];    /* unsused - so far */
    //            } SBTIMBRE;
}

static void sbi2raw(uint8_t *odata, FmBank::Instrument &ins, bool fourOp = false)
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

    odata[11] = ins.rhythm_drum_type;
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


FfmtErrCode SbIBK_impl::loadFileIBK(QString filePath, FmBank &bank)
{
    char magic[4];
    memset(magic, 0, 4);
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();

    bank.deep_tremolo = false;
    bank.deep_vibrato = false;
    bank.volume_model = FmBank::VOLUME_Generic;

    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(strncmp(magic, ibk_magic, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    bool drumFlags[128];
    memset(drumFlags, 0, sizeof(bool) * 128);

    for(uint16_t i = 0; i < 128; i++)
    {
        uint8_t idata[16];
        if(file.read(char_p(idata), 16) != 16)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
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
    for(uint16_t i = 0; i < 128; i++)
    {
        FmBank::Instrument &ins = drumFlags[i] ?
                                  bank.Ins_Percussion[i] :
                                  bank.Ins_Melodic[i];
        if(file.read(ins.name, 9) != 9)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
    }

    file.close();
    //    typedef struct {                     /* 3204 Bytes (0x0C83) */
    //            char     sig[4];             /* signature: "IBK\x1A"  */
    //            SBTIMBRE snd[128];           /* Instrument block */
    //            char     name[128][9];       /* name block: NUL terminated strings */
    //            } IBKFMT;
    return FfmtErrCode::ERR_OK;
}

FfmtErrCode SbIBK_impl::saveFileIBK(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    /* Temporary bank to prevent crash if current bank has less than 128 instruments
     * (for example, imported from some small BNK file) */
    TmpBank tmp(bank, 128, 128);

    bool drumFlags[128];
    memset(drumFlags, 0, sizeof(bool) * 128);
    //Write header
    file.write(char_p(ibk_magic), 4);

    for(uint16_t i = 0; i < 128; i++)
    {
        drumFlags[i] = (tmp.insPercussion[i].rhythm_drum_type != 0);
        FmBank::Instrument &ins = drumFlags[i] ?
                                  tmp.insPercussion[i] :
                                  tmp.insMelodic[i];
        uint8_t odata[16];
        memset(odata, 0, 16);
        sbi2raw(odata, ins, false);
        if(file.write(char_p(&odata), 16) != 16)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    //store bank names
    for(uint16_t i = 0; i < 128; i++)
    {
        FmBank::Instrument &ins = drumFlags[i] ?
                                  tmp.insPercussion[i] :
                                  tmp.insMelodic[i];
        if(file.write(ins.name, 9) != 9)
            return FfmtErrCode::ERR_BADFORMAT;
    }
    file.close();

    return FfmtErrCode::ERR_OK;
}


FfmtErrCode SbIBK_impl::loadFileSBI(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    char magic[4];
    memset(magic, 0, 4);
    memset(&inst, 0, sizeof(FmBank::Instrument));
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bool isExtended = file.bytesAvailable() > 52;
    Q_UNUSED(isExtended);

    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    bool isSbiMagic = strncmp(magic, sbi_magic, 4) == 0;
    bool isVstiMagic = strncmp(magic, vsti_magic, 4) == 0;

    if(!isSbiMagic && !isVstiMagic)
        return FfmtErrCode::ERR_BADFORMAT;

    bool hasSbiExtraFields = true;
    if(isVstiMagic)
    {
        // JuceOPLVSTi SBI has the end filled with 0x20 junk
        hasSbiExtraFields = false;
    }

    //char tempName[32];
    //sprintf(tempName, "NONAME%03d", 0);
    //strncpy(inst.name, tempName, 32 );
    if(file.read(inst.name, 32) != 32)
        return FfmtErrCode::ERR_BADFORMAT;

    bool drumFlag = false;
    uint8_t idata[16];

    if(file.read(char_p(idata), 16) != 16)
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return FfmtErrCode::ERR_BADFORMAT;
    }

    FmBank::Instrument &ins = inst;
    if (hasSbiExtraFields)
        drumFlag = (idata[11] != 0x00);
    raw2sbi(ins, idata, false, hasSbiExtraFields);

    if(isDrum)
        *isDrum = drumFlag;

    if(isVstiMagic && strncmp(inst.name, vsti_inst_name, 32) == 0)
    {
        // it's the hardcoded JuceOPLVSTi name, replace with filename
        QString nameFromPath = QFileInfo(filePath).baseName();
        strncpy(inst.name, nameFromPath.toUtf8().data(), 32);
    }

    if(inst.name[0] == '\0')
    {
        QFileInfo i(filePath);
        strncpy(inst.name, i.baseName().toUtf8().data(), 32);
    }

    file.close();
    return FfmtErrCode::ERR_OK;
}

FfmtErrCode SbIBK_impl::saveFileSBI(QString filePath, FmBank::Instrument &inst, bool)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.write(char_p(sbi_magic), 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;
    if(file.write(inst.name, 32) != 32)
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t odata[16];
    memset(odata, 0, 16);
    sbi2raw(odata, inst, false);
    if(file.write(char_p(&odata), 16) != 16)
        return FfmtErrCode::ERR_BADFORMAT;

    file.close();

    return FfmtErrCode::ERR_OK;
}




FfmtErrCode SbIBK_impl::loadFileSBOP(QString filePath, FmBank &bank, BankFormats &format)
{
    char magic[4];
    bool valid = false;
    bool is4op = false;
    memset(magic, 0, 4);
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();

    qint64  fileSize = file.bytesAvailable();
    bool    fileIs4op = (fileSize == 7680);
    bool    fileIsPercussion = false;
    format = fileIs4op ? BankFormats::FORMAT_SB4OP : BankFormats::FORMAT_SB2OP;
    for(uint16_t i = 0; i < 128; i++)
    {
        uint8_t idata1[16];
        uint8_t idata2[16];
        char    tempName[32];

        if(file.read(magic, 4) != 4)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        if(strncmp(magic, zero_magic, 4) == 0)
        {
            format = fileIs4op ? BankFormats::FORMAT_SB4OP_DRUMS : BankFormats::FORMAT_SB2OP_DRUMS;
            //Skip empty instrument
            file.seek(file.pos() + (fileIs4op ? 56 : 48));
            fileIsPercussion = true; //Percussion banks are always begins from zero instruments
            continue;
        }

        is4op = (strncmp(magic, fop_magic, 4) == 0);
        valid |= is4op;
        valid |= (strncmp(magic, top_magic, 4) == 0);
        valid |= (strncmp(magic, sbi_magic, 4) == 0);

        if(!valid)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        if(file.read(tempName, 32) != 32)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        memset(idata1, 0, 16);
        memset(idata2, 0, 16);

        if(!fileIs4op && file.read(char_p(idata1), 16) != 16)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        if(fileIs4op && ((file.read(char_p(idata1), 11) != 11) || (file.read(char_p(idata2), 13) != 13)))
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        FmBank::Instrument &ins = fileIsPercussion ? bank.Ins_Percussion[i] : bank.Ins_Melodic[i];
        strncpy(ins.name, tempName, 30);
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
        ins.percNoteNum = uint8_t(tempName[31]);
    }
    file.close();
    return FfmtErrCode::ERR_OK;
}

FfmtErrCode SbIBK_impl::saveFileSBOP(QString filePath, FmBank &bank, bool fourOp, bool isDrum)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    /* Temporary bank to prevent crash if current bank has less than 128 instruments
     * (for example, imported from some small BNK file) */
    TmpBank tmp(bank, 128, 128);

    FmBank::Instrument *insts = isDrum ? tmp.insPercussion : tmp.insMelodic;
    for(uint16_t i = 0; i < 128; i++)
    {
        FmBank::Instrument &ins = insts[i];
        const char *magic = (isDrum && (i == 0)) ? zero_magic : (ins.en_4op ? fop_magic : top_magic);
        if(file.write(char_p(magic), 4) != 4)
            return FfmtErrCode::ERR_BADFORMAT;
        char tempName[32];
        memset(tempName, 0, 32);
        strncpy(tempName, ins.name, 31);
        tempName[30] = 0;
        tempName[31] = char(ins.percNoteNum);
        if(file.write(tempName, 32) != 32)
            return FfmtErrCode::ERR_BADFORMAT;
        uint8_t odata[16];
        memset(odata, 0, 16);
        sbi2raw(odata, ins, false);

        if(!fourOp)
        {
            if(file.write(char_p(&odata), 16) != 16)
                return FfmtErrCode::ERR_BADFORMAT;
        }
        else
        {
            if(file.write(char_p(&odata), 11) != 11)
                return FfmtErrCode::ERR_BADFORMAT;
            memset(odata, 0, 16);
            if(ins.en_4op)
                sbi2raw(odata, ins, true);
            if(file.write(char_p(&odata), 13) != 13)
                return FfmtErrCode::ERR_BADFORMAT;
        }
    }
    file.close();

    return FfmtErrCode::ERR_OK;
}



bool SbIBK_DOS::detect(const QString &, char *magic)
{
    return SbIBK_impl::detectIBK(magic);
}

FfmtErrCode SbIBK_DOS::loadFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::loadFileIBK(filePath, bank);
}

FfmtErrCode SbIBK_DOS::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileIBK(filePath, bank);
}

int SbIBK_DOS::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING_GM;
}

QString SbIBK_DOS::formatName() const
{
    return "Sound Blaster IBK file";
}

QString SbIBK_DOS::formatExtensionMask() const
{
    return "*.ibk";
}

QString SbIBK_DOS::formatDefaultExtension() const
{
    return "ibk";
}

BankFormats SbIBK_DOS::formatId() const
{
    return BankFormats::FORMAT_IBK;
}

bool SbIBK_DOS::detectInst(const QString &, char *magic)
{
    return SbIBK_impl::detectSBI(magic);
}

FfmtErrCode SbIBK_DOS::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    return SbIBK_impl::loadFileSBI(filePath, inst, isDrum);
}

FfmtErrCode SbIBK_DOS::saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum)
{
    return SbIBK_impl::saveFileSBI(filePath, inst, isDrum);
}

int SbIBK_DOS::formatInstCaps() const
{
    return int(FormatCaps::FORMAT_CAPS_EVERYTHING_GM);
}

QString SbIBK_DOS::formatInstName() const
{
    return "Sound Blaster Instrument [2OP DOS/UNIX]";
}

QString SbIBK_DOS::formatInstModuleName() const
{
    return "SB DOS/UNIX 2-Op instrument";
}

QString SbIBK_DOS::formatInstExtensionMask() const
{
    return "*.sbi";
}

QString SbIBK_DOS::formatInstDefaultExtension() const
{
    return "sbi";
}

InstFormats SbIBK_DOS::formatInstId() const
{
    return InstFormats::FORMAT_INST_SBI;
}




bool SbIBK_UNIX_READ::detect(const QString &filePath, char *)
{
    bool ret = false;
    ret = SbIBK_impl::detectUNIXO2(filePath, m_recentFormat);
    if(!ret)
        ret = SbIBK_impl::detectUNIXO3(filePath, m_recentFormat);
    return ret;
}

FfmtErrCode SbIBK_UNIX_READ::loadFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::loadFileSBOP(filePath, bank, m_recentFormat);
}

int SbIBK_UNIX_READ::formatCaps() const
{
    return int(FormatCaps::FORMAT_CAPS_OPEN) | int(FormatCaps::FORMAT_CAPS_IMPORT);
}

QString SbIBK_UNIX_READ::formatName() const
{
    return "SB UNIX bank (2 and/or 4 operators)";
}

QString SbIBK_UNIX_READ::formatModuleName() const
{
    return "SB UNIX bank reader (2-Op and 4-Op)";
}

QString SbIBK_UNIX_READ::formatExtensionMask() const
{
    return "*.sb *.o3";
}

BankFormats SbIBK_UNIX_READ::formatId() const
{
    return m_recentFormat;
}

bool SbIBK_UNIX_READ::detectInst(const QString &, char *magic)
{
    return SbIBK_impl::detectSBI4OP(magic);
}

FfmtErrCode SbIBK_UNIX_READ::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *)
{
    char    magic[4];
    char    tempName[32];
    memset(magic, 0, 4);
    memset(&inst, 0, sizeof(FmBank::Instrument));
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(strncmp(magic, fop_magic, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.read(tempName, 32) != 32)
        return FfmtErrCode::ERR_BADFORMAT;

    //bool    drumFlag = false;
    uint8_t idata[16];

    FmBank::Instrument &ins = inst;
    ins.en_4op = true;
    strncpy(ins.name, tempName, 30);

    //Read 1'st and 2'nd operators
    memset(idata, 0, 16);
    if(file.read(char_p(idata), 11) != 11)
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return FfmtErrCode::ERR_BADFORMAT;
    }
    raw2sbi(ins, idata, false);

    //Read 3'st and 4'nd operators
    memset(idata, 0, 16);
    if(file.read(char_p(idata), 13) != 13)
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return FfmtErrCode::ERR_BADFORMAT;
    }
    raw2sbi(ins, idata, true);

    //drumFlag = (idata[11] != 0x00);
    //if(isDrum)
    //    *isDrum = drumFlag;
    ins.percNoteNum = uint8_t(tempName[31]);

    if(inst.name[0] == '\0')
    {
        QFileInfo i(filePath);
        strncpy(inst.name, i.baseName().toUtf8().data(), 32);
    }

    file.close();
    return FfmtErrCode::ERR_OK;
}

FfmtErrCode SbIBK_UNIX_READ::saveFileInst(QString filePath, FmBank::Instrument &inst, bool)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    if(file.write(char_p(fop_magic), 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    char tempName[32];
    memset(tempName, 0, 32);
    strncpy(tempName, inst.name, 31);
    tempName[30] = 0;
    tempName[31] = char(inst.percNoteNum);

    if(file.write(tempName, 32) != 32)
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t odata[16];
    //Write 1'st and 2'nd operators
    memset(odata, 0, 16);
    sbi2raw(odata, inst, false);
    if(file.write(char_p(&odata), 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;

    //Write 3'st and 4'nd operators
    memset(odata, 0, 16);
    sbi2raw(odata, inst, true);
    if(file.write(char_p(&odata), 13) != 13)
        return FfmtErrCode::ERR_BADFORMAT;

    file.close();

    return FfmtErrCode::ERR_OK;
}

int SbIBK_UNIX_READ::formatInstCaps() const
{
    return int(FormatCaps::FORMAT_CAPS_EVERYTHING);
}

QString SbIBK_UNIX_READ::formatInstName() const
{
    return "Sound Blaster Instrument [4OP UNIX]";
}

QString SbIBK_UNIX_READ::formatInstModuleName() const
{
    return "SB UNIX 4-Op instrument";
}

QString SbIBK_UNIX_READ::formatInstExtensionMask() const
{
    return "*.sbi";
}

QString SbIBK_UNIX_READ::formatInstDefaultExtension() const
{
    return "sbi";
}

InstFormats SbIBK_UNIX_READ::formatInstId() const
{
    return InstFormats::FORMAT_INST_SBIex;
}




FfmtErrCode SbIBK_UNIX2OP_SAVE::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileSBOP(filePath, bank, false, false);
}

int SbIBK_UNIX2OP_SAVE::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE|
           (int)FormatCaps::FORMAT_CAPS_GM_BANK|
           (int)FormatCaps::FORMAT_CAPS_MELODIC_ONLY;
}

QString SbIBK_UNIX2OP_SAVE::formatName() const
{
    return "SB UNIX 2-op bank [Melodic]";
}

QString SbIBK_UNIX2OP_SAVE::formatModuleName() const
{
    return "SB UNIX 2-Op Melodic bank writer";
}

QString SbIBK_UNIX2OP_SAVE::formatExtensionMask() const
{
    return "*.sb";
}

QString SbIBK_UNIX2OP_SAVE::formatDefaultExtension() const
{
    return "sb";
}

BankFormats SbIBK_UNIX2OP_SAVE::formatId() const
{
    return BankFormats::FORMAT_SB2OP;
}



FfmtErrCode SbIBK_UNIX2OP_DRUMS_SAVE::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileSBOP(filePath, bank, false, true);
}

int SbIBK_UNIX2OP_DRUMS_SAVE::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE|
           (int)FormatCaps::FORMAT_CAPS_GM_BANK|
           (int)FormatCaps::FORMAT_CAPS_PERCUSSION_ONLY;
}

QString SbIBK_UNIX2OP_DRUMS_SAVE::formatName() const
{
    return "SB UNIX 2-op bank [Percussion]";
}

QString SbIBK_UNIX2OP_DRUMS_SAVE::formatModuleName() const
{
    return "SB UNIX 2-Op Percussion bank writer";
}

QString SbIBK_UNIX2OP_DRUMS_SAVE::formatExtensionMask() const
{
    return "*.sb";
}

QString SbIBK_UNIX2OP_DRUMS_SAVE::formatDefaultExtension() const
{
    return "sb";
}

BankFormats SbIBK_UNIX2OP_DRUMS_SAVE::formatId() const
{
    return BankFormats::FORMAT_SB2OP_DRUMS;
}




FfmtErrCode SbIBK_UNIX4OP_SAVE::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileSBOP(filePath, bank, true, false);
}

int SbIBK_UNIX4OP_SAVE::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE|
           (int)FormatCaps::FORMAT_CAPS_GM_BANK|
           (int)FormatCaps::FORMAT_CAPS_MELODIC_ONLY;
}

QString SbIBK_UNIX4OP_SAVE::formatName() const
{
    return "SB UNIX 4-op bank [Melodic]";
}

QString SbIBK_UNIX4OP_SAVE::formatModuleName() const
{
    return "SB UNIX 4-Op Melodic bank writer";
}

QString SbIBK_UNIX4OP_SAVE::formatExtensionMask() const
{
    return "*.o3";
}

QString SbIBK_UNIX4OP_SAVE::formatDefaultExtension() const
{
    return "o3";
}

BankFormats SbIBK_UNIX4OP_SAVE::formatId() const
{
    return BankFormats::FORMAT_SB4OP;
}


FfmtErrCode SbIBK_UNIX4OP_DRUMS_SAVE::saveFile(QString filePath, FmBank &bank)
{
    return SbIBK_impl::saveFileSBOP(filePath, bank, true, true);
}

int SbIBK_UNIX4OP_DRUMS_SAVE::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE|
           (int)FormatCaps::FORMAT_CAPS_GM_BANK|
           (int)FormatCaps::FORMAT_CAPS_PERCUSSION_ONLY;
}

QString SbIBK_UNIX4OP_DRUMS_SAVE::formatName() const
{
    return "SB UNIX 4-op bank [Percussion]";
}

QString SbIBK_UNIX4OP_DRUMS_SAVE::formatModuleName() const
{
    return "SB UNIX 4-Op Percussion bank writer";
}

QString SbIBK_UNIX4OP_DRUMS_SAVE::formatExtensionMask() const
{
    return "*.o3";
}

QString SbIBK_UNIX4OP_DRUMS_SAVE::formatDefaultExtension() const
{
    return "o3";
}

BankFormats SbIBK_UNIX4OP_DRUMS_SAVE::formatId() const
{
    return BankFormats::FORMAT_SB4OP_DRUMS;
}
