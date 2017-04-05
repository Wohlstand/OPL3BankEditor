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

#include "dmxopl2.h"
#include "../common.h"

static const char *dmx_magic = "#OPL_II#";

bool DmxOPL2::detect(char *magic)
{
    return (strncmp(magic, dmx_magic, 8) == 0);
}

int DmxOPL2::loadFile(QString filePath, FmBank &bank)
{
    char magic[8];
    memset(magic, 0, 8);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();

    if(file.read(magic, 8) != 8)
        return ERR_BADFORMAT;

    if(strncmp(magic, dmx_magic, 8) != 0)
        return ERR_BADFORMAT;


    for(unsigned short i = 0; i < 175; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  bank.Ins_Melodic[i] :
                                  bank.Ins_Percussion[(i - 128) + 35];
        unsigned short  flags       = 0;
        unsigned char   fine_tuning = 0;
        unsigned char   note_number = 0;
        unsigned char   idata[32];

        if(readLE(file, flags) != 2)
            return ERR_BADFORMAT;

        if(file.read(char_p(&fine_tuning), 1) != 1)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
        if(file.read(char_p(&note_number), 1) != 1)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
        if(file.read(char_p(idata), 32) != 32)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        ins.fine_tune = char(int(fine_tuning) - 128);
        ins.en_pseudo4op = ((flags & Dmx_DoubleVoice) != 0);
        ins.en_4op = ins.en_pseudo4op;
        ins.percNoteNum = note_number;

        ins.setAVEKM(MODULATOR1,    idata[0]);
        ins.setAtDec(MODULATOR1,    idata[1]);
        ins.setSusRel(MODULATOR1,   idata[2]);
        ins.setWaveForm(MODULATOR1, idata[3]);
        ins.setKSL(MODULATOR1,      idata[4]);
        ins.setLevel(MODULATOR1,    idata[5]);

        ins.setFBConn1(idata[6]);

        ins.setAVEKM(CARRIER1,      idata[7]);
        ins.setAtDec(CARRIER1,      idata[8]);
        ins.setSusRel(CARRIER1,     idata[9]);
        ins.setWaveForm(CARRIER1,   idata[10]);
        ins.setKSL(CARRIER1,        idata[11]);
        ins.setLevel(CARRIER1,      idata[12]);
        //13'th byte is unused, but sadly :P, lucky number, it MUST BE USED!!!
        ins.note_offset1 = toSint16LE(&idata[14]) + 12;

        ins.setAVEKM(MODULATOR2,    idata[16]);
        ins.setAtDec(MODULATOR2,    idata[17]);
        ins.setSusRel(MODULATOR2,   idata[18]);
        ins.setWaveForm(MODULATOR2, idata[19]);
        ins.setKSL(MODULATOR2,      idata[20]);
        ins.setLevel(MODULATOR2,    idata[21]);

        ins.setFBConn2(idata[22]);

        ins.setAVEKM(CARRIER2,      idata[23]);
        ins.setAtDec(CARRIER2,      idata[24]);
        ins.setSusRel(CARRIER2,     idata[25]);
        ins.setWaveForm(CARRIER2,   idata[26]);
        ins.setKSL(CARRIER2,        idata[27]);
        ins.setLevel(CARRIER2,      idata[28]);
        //29'th byte is unused
        ins.note_offset2 = toSint16LE(&idata[30]) + 12;
    }

    //Instrument names
    for(unsigned short i = 0; i < 175; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  bank.Ins_Melodic[i] :
                                  bank.Ins_Percussion[(i - 128) + 35];
        if(file.read(ins.name, 32) != 32)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
    }

    file.close();

    return ERR_OK;
}

int DmxOPL2::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return ERR_NOFILE;

    //Write header
    file.write(char_p(dmx_magic), 8);

    for(unsigned short i = 0; i < 175; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  bank.Ins_Melodic[i] :
                                  bank.Ins_Percussion[(i - 128) + 35];
        unsigned short  flags       = 0;
        unsigned char   fine_tuning = 0;
        unsigned char   note_number = 0;
        unsigned char   odata[32];
        memset(odata, 0, 32);

        fine_tuning = uchar(int(ins.fine_tune) + 128);
        flags |= (ins.en_4op && ins.en_pseudo4op) ? Dmx_DoubleVoice : 0;
        flags |= (ins.percNoteNum != 0) ? Dmx_FixedPitch : 0;
        flags |= (i == 65) ? Dmx_Unknown : 0;
        note_number = ins.percNoteNum;

        odata[0]  = ins.getAVEKM(MODULATOR1);
        odata[1]  = ins.getAtDec(MODULATOR1);
        odata[2]  = ins.getSusRel(MODULATOR1);
        odata[3]  = ins.getWaveForm(MODULATOR1);
        odata[4]  = ins.getKSL(MODULATOR1);
        odata[5]  = ins.getLevel(MODULATOR1);

        odata[6]  = ins.getFBConn1();

        odata[7]  = ins.getAVEKM(CARRIER1);
        odata[8]  = ins.getAtDec(CARRIER1);
        odata[9]  = ins.getSusRel(CARRIER1);
        odata[10] = ins.getWaveForm(CARRIER1);
        odata[11] = ins.getKSL(CARRIER1);
        odata[12] = ins.getLevel(CARRIER1);
        odata[13] = 0x00;//...but would to use this for something other?

        fromSint16LE(ins.note_offset1 - 12, &odata[14]);

        odata[16] = ins.getAVEKM(MODULATOR2);
        odata[17] = ins.getAtDec(MODULATOR2);
        odata[18] = ins.getSusRel(MODULATOR2);
        odata[19] = ins.getWaveForm(MODULATOR2);
        odata[20] = ins.getKSL(MODULATOR2);
        odata[21] = ins.getLevel(MODULATOR2);

        odata[22] = ins.getFBConn2();

        odata[23] = ins.getAVEKM(CARRIER2);
        odata[24] = ins.getAtDec(CARRIER2);
        odata[25] = ins.getSusRel(CARRIER2);
        odata[26] = ins.getWaveForm(CARRIER2);
        odata[27] = ins.getKSL(CARRIER2);
        odata[28] = ins.getLevel(CARRIER2);
        odata[29] = 0x00;//...but would to use this for something other?

        fromSint16LE(ins.note_offset2 - 12, &odata[30]);

        if(writeLE(file, flags) != 2)
            return ERR_BADFORMAT;

        if(file.write(char_p(&fine_tuning), 1) != 1)
            return ERR_BADFORMAT;

        if(file.write(char_p(&note_number), 1) != 1)
            return ERR_BADFORMAT;

        if(file.write(char_p(&odata), 32) != 32)
            return ERR_BADFORMAT;
    }

    //Instrument names
    for(unsigned short i = 0; i < 175; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                  bank.Ins_Melodic[i] :
                                  bank.Ins_Percussion[(i - 128) + 35];
        if(file.write(ins.name, 32) != 32)
            bank.reset();
    }

    file.close();

    return ERR_OK;
}
