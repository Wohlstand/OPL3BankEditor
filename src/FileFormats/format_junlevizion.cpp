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

#include "format_junlevizion.h"
#include "../common.h"

static const char *jv_magic = "Junglevision Patch File\x1A\0\0\0\0\0\0\0\0";

bool JunleVizion::detect(const QString &, char *magic)
{
    return (strncmp(magic, jv_magic, 32) == 0);
}

FfmtErrCode JunleVizion::loadFile(QString filePath, FmBank &bank)
{
    uint16_t count_melodic     = 0;
    uint16_t count_percusive   = 0;
    uint16_t startAt_melodic   = 0;
    uint16_t startAt_percusive = 0;
    char magic[32];
    memset(magic, 0, 32);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();

    bank.deep_tremolo = true;
    bank.deep_vibrato = true;
    bank.volume_model = FmBank::VOLUME_9X;

    if(file.read(magic, 32) != 32)
        return FfmtErrCode::ERR_BADFORMAT;

    if(strncmp(magic, jv_magic, 32) != 0)
        return FfmtErrCode::ERR_BADFORMAT;
    if(readLE(file, count_melodic) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(readLE(file, count_percusive) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(readLE(file, startAt_melodic) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(readLE(file, startAt_percusive) != 2)
        return FfmtErrCode::ERR_BADFORMAT;

    if(count_melodic > 128)
        return FfmtErrCode::ERR_BADFORMAT;
    if(count_percusive > 128)
        return FfmtErrCode::ERR_BADFORMAT;

    if((count_melodic + startAt_melodic) > 128)
        return FfmtErrCode::ERR_BADFORMAT;
    if((count_percusive + startAt_percusive) > 128)
        return FfmtErrCode::ERR_BADFORMAT;

    uint16_t total = count_melodic + count_percusive;
    for(uint16_t i = 0; i < total; i++)
    {
        FmBank::Instrument &ins = (i < count_melodic) ? bank.Ins_Melodic[i + startAt_melodic] : bank.Ins_Percussion[(i - count_melodic) + startAt_percusive];
        uint8_t idata[24];
        if(file.read(char_p(idata), 24) != 24)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        //Operators mode: 0 - 2-op, 1 - 4-op
        ins.en_4op = (idata[0] == 1);
        //NoteNum
        ins.percNoteNum = idata[1];
        //OP1
        ins.setAVEKM(MODULATOR1,    idata[2]);
        ins.setKSLL(MODULATOR1,     idata[3]);
        ins.setAtDec(MODULATOR1,    idata[4]);
        ins.setSusRel(MODULATOR1,   idata[5]);
        ins.setWaveForm(MODULATOR1, idata[6]);
        //Feedback/Connection 1<->2
        ins.setFBConn1(idata[7]);
        //OP2
        ins.setAVEKM(CARRIER1, idata[8]);
        ins.setKSLL(CARRIER1,  idata[9]);
        ins.setAtDec(CARRIER1, idata[10]);
        ins.setSusRel(CARRIER1, idata[11]);
        ins.setWaveForm(CARRIER1, idata[12]);

        //OP3
        ins.setAVEKM(MODULATOR2,    idata[13]);
        ins.setKSLL(MODULATOR2,     idata[14]);
        ins.setAtDec(MODULATOR2,    idata[15]);
        ins.setSusRel(MODULATOR2,   idata[16]);
        ins.setWaveForm(MODULATOR2, idata[17]);
        //Feedback/Connection 3<->4
        ins.setFBConn2(idata[18]);
        //OP4
        ins.setAVEKM(CARRIER2, idata[19]);
        ins.setKSLL(CARRIER2,  idata[20]);
        ins.setAtDec(CARRIER2, idata[21]);
        ins.setSusRel(CARRIER2, idata[22]);
        ins.setWaveForm(CARRIER2, idata[23]);
    }
    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode JunleVizion::saveFile(QString filePath, FmBank &bank)
{
    FmBank::Instrument null;
    memset(&null, 0, sizeof(FmBank::Instrument));

    uint16_t count_melodic     = 128;
    uint16_t count_percusive   = 128;
    uint16_t startAt_melodic   = 0;
    uint16_t startAt_percusive = 0;

    /* Temporary bank to prevent crash if current bank has less than 128 instruments
     * (for example, imported from some small BNK file) */
    TmpBank tmp(bank, 128, 128);

    //Find begin
    for(uint16_t i = 0; i < 128; i++)
    {
        if((memcmp(&tmp.insPercussion[i], &null, sizeof(FmBank::Instrument)) != 0) && (startAt_melodic == 0))
        {
            startAt_percusive = i;
            break;
        }
        count_percusive -= 1;
    }

    //Find tail
    if(count_percusive != 0)
    {
        for(uint16_t i = 127; i >= startAt_percusive; i--)
        {
            if((memcmp(&tmp.insPercussion[i], &null, sizeof(FmBank::Instrument)) != 0) && (startAt_melodic == 0))
                break;
            count_percusive -= 1;
        }
    }
    else
        startAt_percusive = 0;

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    //Write header
    file.write(char_p(jv_magic), 32);

    writeLE(file, count_melodic);
    writeLE(file, count_percusive);
    writeLE(file, startAt_melodic);
    writeLE(file, startAt_percusive);

    uint16_t total = count_melodic + count_percusive;
    bool had4op = false;
    for(uint16_t i = 0; i < total; i++)
    {
        FmBank::Instrument &ins = (i < count_melodic) ?
                                    tmp.insMelodic[i + startAt_melodic] :
                                    tmp.insPercussion[(i - count_melodic) + startAt_percusive];
        uint8_t odata[24];
        memset(odata, 0, 24);
        had4op |= ins.en_4op;
        //Operators mode: 0 - 2-op, 1 - 4-op
        odata[0] = uint8_t(ins.en_4op);
        //NoteNum
        odata[1] = clip_u8(ins.percNoteNum + ins.note_offset1, 0, 127);

        //OP1
        odata[2]  = ins.getAVEKM(MODULATOR1);
        odata[3]  = ins.getKSLL(MODULATOR1);
        odata[4]  = ins.getAtDec(MODULATOR1);
        odata[5]  = ins.getSusRel(MODULATOR1);
        odata[6]  = ins.getWaveForm(MODULATOR1) | (had4op ? 0x80 : 0x00);
        //Feedback/Connection 1<->2
        odata[7]  = ins.getFBConn1() | (had4op ? (0x30 & uint8_t(3) << 4) : 0);
        //OP2
        odata[8]  = ins.getAVEKM(CARRIER1);
        odata[9]  = ins.getKSLL(CARRIER1);
        odata[10] = ins.getAtDec(CARRIER1);
        odata[11] = ins.getSusRel(CARRIER1);
        odata[12] = ins.getWaveForm(CARRIER1) | (had4op ? 0x80 : 0x00);

        //OP3
        odata[13] = ins.getAVEKM(MODULATOR2);
        odata[14] = ins.getKSLL(MODULATOR2);
        odata[15] = ins.getAtDec(MODULATOR2);
        odata[16] = ins.getSusRel(MODULATOR2);
        odata[17] = ins.getWaveForm(MODULATOR2) | (ins.en_4op ? 0x80 : 0x00);
        //Feedback/Connection 3<->4
        odata[18] = ins.getFBConn2() | (had4op ? (0x30 & uint8_t(3) << 4) : 0);
        //OP4
        odata[19] = ins.getAVEKM(CARRIER2);
        odata[20] = ins.getKSLL(CARRIER2);
        odata[21] = ins.getAtDec(CARRIER2);
        odata[22] = ins.getSusRel(CARRIER2);
        odata[23] = ins.getWaveForm(CARRIER2) | (ins.en_4op ? 0x80 : 0x00);

        if(file.write(char_p(odata), 24) != 24)
            return FfmtErrCode::ERR_BADFORMAT;
    }
    file.close();

    return FfmtErrCode::ERR_OK;
}

int JunleVizion::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING_GM;
}

QString JunleVizion::formatName() const
{
    return "Junlevision bank";
}

QString JunleVizion::formatExtensionMask() const
{
    return "*.op3";
}

QString JunleVizion::formatDefaultExtension() const
{
    return "op3";
}

BankFormats JunleVizion::formatId() const
{
    return BankFormats::FORMAT_JUNGLEVIZION;
}
