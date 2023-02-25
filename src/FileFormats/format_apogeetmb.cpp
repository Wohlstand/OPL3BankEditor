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

#include "format_apogeetmb.h"
#include "../common.h"

bool ApogeeTMB::detect(const QString &filePath, char* /*magic*/)
{
    if(hasExt(filePath, ".tmb"))
        return true;
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return false;
    qint64 fileSize = file.bytesAvailable();
    file.close();
    return (fileSize == (256 * 13));
}

FfmtErrCode ApogeeTMB::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();

    bank.deep_tremolo = false;
    bank.deep_vibrato = false;
    bank.volume_model = FmBank::VOLUME_APOGEE;

    for(uint16_t i = 0; i < 256; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  bank.Ins_Melodic[i] :
                                  bank.Ins_Percussion[(i - 128)];
        uint8_t   idata[13];

        if(file.read(char_p(idata), 13) != 13)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        ins.setAVEKM(MODULATOR1,    idata[0]);
        ins.setAVEKM(CARRIER1,      idata[1]);
        ins.setKSLL(MODULATOR1,     idata[2]);
        ins.setKSLL(CARRIER1,       idata[3]);
        ins.setAtDec(MODULATOR1,    idata[4]);
        ins.setAtDec(CARRIER1,      idata[5]);
        ins.setSusRel(MODULATOR1,   idata[6]);
        ins.setSusRel(CARRIER1,     idata[7]);
        ins.setWaveForm(MODULATOR1, idata[8]);
        ins.setWaveForm(CARRIER1,   idata[9]);
        ins.setFBConn1(idata[10]);

        if(i < 128)
            ins.note_offset1 = char_p(idata)[11];
        else
            ins.percNoteNum  = idata[11];

        ins.velocity_offset = char_p(idata)[12];
    }

    file.close();
    return FfmtErrCode::ERR_OK;
}

FfmtErrCode ApogeeTMB::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    /* Temporary bank to prevent crash if current bank has less than 128 instruments
     * (for example, imported from some small BNK file) */
    TmpBank tmp(bank, 128, 128);

    for(uint16_t i = 0; i < 256; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  tmp.insMelodic[i] :
                                  tmp.insPercussion[(i - 128)];
        uint8_t   odata[13];
        memset(odata, 0, 13);
        odata[0] = ins.getAVEKM(MODULATOR1);
        odata[1] = ins.getAVEKM(CARRIER1);
        odata[2] = ins.getKSLL(MODULATOR1);
        odata[3] = ins.getKSLL(CARRIER1);
        odata[4] = ins.getAtDec(MODULATOR1);
        odata[5] = ins.getAtDec(CARRIER1);
        odata[6] = ins.getSusRel(MODULATOR1);
        odata[7] = ins.getSusRel(CARRIER1);
        odata[8] = ins.getWaveForm(MODULATOR1);
        odata[9] = ins.getWaveForm(CARRIER1);
        odata[10] = ins.getFBConn1();
        char *sodata = char_p(odata);

        if(i < 128)
            sodata[11] = int8_t(ins.note_offset1);
        else
            odata[11] = clip_u8(ins.percNoteNum + ins.note_offset1, 0, 127);

        sodata[12] = ins.velocity_offset;

        if(file.write(char_p(&odata), 13) != 13)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    file.close();
    return FfmtErrCode::ERR_OK;
}

int ApogeeTMB::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING_GM;
}

QString ApogeeTMB::formatName() const
{
    return "Apogee Sound System timbre bank";
}

QString ApogeeTMB::formatExtensionMask() const
{
    return "*.tmb";
}

QString ApogeeTMB::formatDefaultExtension() const
{
    return "tmb";
}

BankFormats ApogeeTMB::formatId() const
{
    return BankFormats::FORMAT_APOGEE;
}
