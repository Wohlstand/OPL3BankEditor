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

#include "apogeetmb.h"
#include "../common.h"

bool ApogeeTMB::detect(QString filePath)
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

int ApogeeTMB::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();

    for(unsigned short i = 0; i < 256; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  bank.Ins_Melodic[i] :
                                  bank.Ins_Percussion[(i - 128)];
        unsigned char   idata[13];

        if(file.read(char_p(idata), 13) != 13)
        {
            bank.reset();
            return ERR_BADFORMAT;
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
    return ERR_OK;
}

int ApogeeTMB::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return ERR_NOFILE;

    for(unsigned short i = 0; i < 256; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  bank.Ins_Melodic[i] :
                                  bank.Ins_Percussion[(i - 128)];
        unsigned char   odata[13];
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
            sodata[11] = char(ins.note_offset1);
        else
            odata[11] = ins.percNoteNum;

        sodata[12] = ins.velocity_offset;

        if(file.write(char_p(&odata), 13) != 13)
            return ERR_BADFORMAT;
    }

    file.close();
    return ERR_OK;
}
