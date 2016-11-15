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

static const char *sbi_magic = "SBI\x1A";
static const char *ibk_magic = "IBK\x1A";

bool SbIBK::detect(char *magic)
{
    return (strncmp(magic, ibk_magic, 4) == 0);
}

bool SbIBK::detectInst(char *magic)
{
    return (strncmp(magic, sbi_magic, 4) == 0);
}

bool SbIBK::detectSB2OP(QString filePath)
{
    if(hasExt(filePath, ".sb"))
        return true;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.bytesAvailable();
    file.close();
    return (fileSize == 6656);
}

bool SbIBK::detectSB4OP(QString filePath)
{
    if(hasExt(filePath, ".o3"))
        return true;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.bytesAvailable();
    file.close();
    return (fileSize == 7680);
}

static void raw2sbi(FmBank::Instrument &ins, unsigned char *idata, bool fourOp = false)
{
    int MODULATOR   = fourOp ? MODULATOR2 : MODULATOR1;
    int CARRIER     = fourOp ? CARRIER2 : CARRIER1;
    ins.setAVEKM(MODULATOR,    idata[0]);
    ins.setAVEKM(CARRIER,      idata[1]);
    //            BYTE modscal;   /* KSL, TL */
    //            BYTE carscal;
    ins.setKSLL(MODULATOR,     idata[2]);
    ins.setKSLL(CARRIER,       idata[3]);
    //            BYTE modad;     /* Attack/Decay */
    //            BYTE carad;
    ins.setAtDec(MODULATOR,    idata[4]);
    ins.setAtDec(CARRIER,      idata[5]);
    //            BYTE modsr;     /* Sustain/Release */
    //            BYTE carsr;
    ins.setSusRel(MODULATOR,   idata[6]);
    ins.setSusRel(CARRIER,     idata[7]);
    //            BYTE modwave;   /* Wave Select */
    //            BYTE carwave;
    ins.setWaveForm(MODULATOR, idata[8]);
    ins.setWaveForm(CARRIER,   idata[9]);

    //            BYTE feedback;  /* FB, Connection *(inverse of Adlib)* <- not true? */
    //                            /* The following was originally reserved...CL uses  */
    //                            /* the next byte the same way we do: BD=6,SD=7,TT=8 */
    //                            /* CY=9,HH=10                                       */
    if(fourOp)
        ins.setFBConn2(idata[10]);
    else
        ins.setFBConn1(idata[10]);

    //            BYTE percvoc;   /* Percussion voice number                    : JWO */
    if(!fourOp)
        ins.adlib_drum_number  = idata[11];

    //            char transpos;  /* Number of notes to transpose timbre, signed: JWO */
    if(fourOp)
        ins.note_offset2 = char_p(idata)[12];
    else
        ins.note_offset1 = char_p(idata)[12];

    //            BYTE dpitch;    /* percussion pitch: MIDI Note 0 - 127        : JWO */
    if(!fourOp)
        ins.percNoteNum  = idata[13];

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


int SbIBK::loadFile(QString filePath, FmBank &bank)
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

int SbIBK::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
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

int SbIBK::saveFile(QString filePath, FmBank &bank)
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
