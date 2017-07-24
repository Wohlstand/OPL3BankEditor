/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_bisqwit.h"
#include "../common.h"

bool BisqwitBank::detect(const QString &filePath, char *)
{
    if(hasExt(filePath, ".adlraw"))
        return true;
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return false;
    qint64 fileSize = file.bytesAvailable();
    file.close();
    return (fileSize == 6400);
}

FfmtErrCode BisqwitBank::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;
    bank.reset();

    for(uint16_t i = 0; i < 256; i++)
    {
        bool isDrum = (i >= 128);
        uint8_t key = isDrum ? uint8_t(i - 128) : uint8_t(i);
        FmBank::Instrument &ins = isDrum ?
                                  bank.Ins_Percussion[key] :
                                  bank.Ins_Melodic[key];
        uint8_t idata[25];
        uint8_t *op1 = idata + 2;
        uint8_t *op2 = idata + 14;

        if(file.read(char_p(idata), 25) != 25)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
        ins.en_4op = (memcmp(op1, op2, 11) != 0);
        ins.percNoteNum  = (isDrum && (idata[0] == 0)) ? key : idata[0];

        ins.note_offset1 = int8_t(idata[1]);
        ins.setAVEKM(MODULATOR1,    op1[0]);
        ins.setAVEKM(CARRIER1,      op1[1]);
        ins.setAtDec(MODULATOR1,    op1[2]);
        ins.setAtDec(CARRIER1,      op1[3]);
        ins.setSusRel(MODULATOR1,   op1[4]);
        ins.setSusRel(CARRIER1,     op1[5]);
        ins.setWaveForm(MODULATOR1, op1[6]);
        ins.setWaveForm(CARRIER1,   op1[7]);
        ins.setKSLL(MODULATOR1,     op1[8]);
        ins.setKSLL(CARRIER1,       op1[9]);
        ins.setFBConn1(op1[10]);

        ins.note_offset2 = int8_t(idata[13]);
        ins.setAVEKM(MODULATOR2,    op2[0]);
        ins.setAVEKM(CARRIER2,      op2[1]);
        ins.setAtDec(MODULATOR2,    op2[2]);
        ins.setAtDec(CARRIER2,      op2[3]);
        ins.setSusRel(MODULATOR2,   op2[4]);
        ins.setSusRel(CARRIER2,     op2[5]);
        ins.setWaveForm(MODULATOR2, op2[6]);
        ins.setWaveForm(CARRIER2,   op2[7]);
        ins.setKSLL(MODULATOR2,     op2[8]);
        ins.setKSLL(CARRIER2,       op2[9]);
        ins.setFBConn2(op2[10]);
    }
    return FfmtErrCode::ERR_OK;
}

FfmtErrCode BisqwitBank::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    /* Temporary bank to prevent crash if current bank has less than 128 instruments
     * (for example, imported from some small BNK file) */
    TmpBank tmp(bank, 128, 128);
    for(uint16_t i = 0; i < 256; i++)
    {
        bool isDrum = (i >= 128);
        uint8_t key = isDrum ? uint8_t(i - 128) : uint8_t(i);
        FmBank::Instrument &ins = isDrum ?
                                  tmp.insPercussion[key] :
                                  tmp.insMelodic[key];
        uint8_t odata[25];
        uint8_t *op1 = odata + 2;
        uint8_t *op2 = odata + 14;

        odata[0] = (isDrum && (ins.percNoteNum != key)) ? ins.percNoteNum : 0;

        odata[1] = uint8_t(int8_t(ins.note_offset1));
        op1[0] = ins.getAVEKM(MODULATOR1);
        op1[1] = ins.getAVEKM(CARRIER1);
        op1[2] = ins.getAtDec(MODULATOR1);
        op1[3] = ins.getAtDec(CARRIER1);
        op1[4] = ins.getSusRel(MODULATOR1);
        op1[5] = ins.getSusRel(CARRIER1);
        op1[6] = ins.getWaveForm(MODULATOR1);
        op1[7] = ins.getWaveForm(CARRIER1);
        op1[8] = ins.getKSLL(MODULATOR1);
        op1[9] = ins.getKSLL(CARRIER1);
        op1[10] = ins.getFBConn1();

        if(!ins.en_4op)
        {
            odata[13] = odata[1];
            memcpy(op2, op1, 11);
        }
        else
        {
            odata[13] = uint8_t(int8_t(ins.note_offset2));
            op2[0] = ins.getAVEKM(MODULATOR2);
            op2[1] = ins.getAVEKM(CARRIER2);
            op2[2] = ins.getAtDec(MODULATOR2);
            op2[3] = ins.getAtDec(CARRIER2);
            op2[4] = ins.getSusRel(MODULATOR2);
            op2[5] = ins.getSusRel(CARRIER2);
            op2[6] = ins.getWaveForm(MODULATOR2);
            op2[7] = ins.getWaveForm(CARRIER2);
            op2[8] = ins.getKSLL(MODULATOR2);
            op2[9] = ins.getKSLL(CARRIER2);
            op2[10] = ins.getFBConn2();
        }

        if(file.write(char_p(&odata), 25) != 25)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    return FfmtErrCode::ERR_OK;
}

int BisqwitBank::formatCaps()
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString BisqwitBank::formatName()
{
    return "Bisqwit's ADLMIDI Raw bank";
}

QString BisqwitBank::formatExtensionMask()
{
    return "*.adlraw";
}

BankFormats BisqwitBank::formatId()
{
    return BankFormats::FORMAT_BISQWIT;
}
