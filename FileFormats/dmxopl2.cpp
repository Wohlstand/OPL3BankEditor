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

static const char* dmx_magic = "#OPL_II#";

bool DmxOPL2::detect(char* magic)
{
    return (strncmp(magic, dmx_magic, 8) == 0);
}

int DmxOPL2::loadFile(QString filePath, FmBank &bank)
{
    char magic[8]; memset(magic, 0, 8);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();

    if(file.read(magic, 8) != 8)
        return ERR_BADFORMAT;

    if( strncmp(magic, dmx_magic, 8) != 0 )
        return ERR_BADFORMAT;


    for(unsigned short i=0; i<175; i++)
    {
        FmBank::Instrument &ins = (i<128) ? bank.Ins_Melodic[i] : bank.Ins_Percussion[(i-128)+35];
        unsigned short  flags       = 0;
        unsigned char   fine_tuning = 0;
        unsigned char   note_number = 0;
        unsigned char   idata[32];

        if( readLE(file, flags) != 2)
            return ERR_BADFORMAT;

        if( file.read(char_p(&fine_tuning), 1) != 1 )
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
        if( file.read(char_p(&note_number), 1) != 1 )
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
        if( file.read(char_p(idata), 32) != 32 )
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        ins.fine_tune = char( int(fine_tuning) - 128 );
        ins.en_pseudo4op = ((flags & Dmx_DoubleVoice) != 0);
        ins.en_4op = ins.en_pseudo4op;
        ins.percNoteNum  = note_number;// + ((flags & Dmx_FixedPitch)!=0 ? 12 : 0);

        ins.OP[MODULATOR1].am  = (idata[0]>>7)&0x01;
        ins.OP[MODULATOR1].vib = (idata[0]>>6)&0x01;
        ins.OP[MODULATOR1].eg  = (idata[0]>>5)&0x01;
        ins.OP[MODULATOR1].ksr = (idata[0]>>4)&0x01;
        ins.OP[MODULATOR1].fmult = (idata[0])&0x0F;

        ins.OP[MODULATOR1].attack   = (idata[1]>>4)&0x0F;
        ins.OP[MODULATOR1].decay   = (idata[1])&0x0F;

        ins.OP[MODULATOR1].sustain   = 0x0F - ((idata[2]>>4)&0x0F);
        ins.OP[MODULATOR1].release   = (idata[2])&0x0F;

        ins.OP[MODULATOR1].waveform  = idata[3]&0x07;

        ins.OP[MODULATOR1].ksl   = (idata[4]>>6)&0x03;

        ins.OP[MODULATOR1].level = 0x3F-((idata[5]) & 0x3F);

        ins.connection1 =    idata[6] & 0x01;
        ins.feedback1 =     (idata[6]>>1) & 0x07;

        ins.OP[CARRIER1].am  = (idata[7]>>7)&0x01;
        ins.OP[CARRIER1].vib = (idata[7]>>6)&0x01;
        ins.OP[CARRIER1].eg  = (idata[7]>>5)&0x01;
        ins.OP[CARRIER1].ksr = (idata[7]>>4)&0x01;
        ins.OP[CARRIER1].fmult = (idata[7])&0x0F;

        ins.OP[CARRIER1].attack   = (idata[8]>>4)&0x0F;
        ins.OP[CARRIER1].decay   = (idata[8])&0x0F;

        ins.OP[CARRIER1].sustain   = 0x0F - ((idata[9]>>4)&0x0F);
        ins.OP[CARRIER1].release   = (idata[9])&0x0F;

        ins.OP[CARRIER1].waveform  = idata[10]&0x07;

        ins.OP[CARRIER1].ksl   = (idata[11]>>6)&0x03;

        ins.OP[CARRIER1].level = 0x3F-((idata[12]) & 0x3F);

        //13'th unused, but sadly :P, lucky number!!!

        ins.note_offset1 = toSint16LE(&idata[14]) + 12;//((short(idata[14])&0x00FF) & ((short(idata[15])<<8) && 0xFF00)) + 12;

        ins.OP[MODULATOR2].am  = (idata[16]>>7)&0x01;
        ins.OP[MODULATOR2].vib = (idata[16]>>6)&0x01;
        ins.OP[MODULATOR2].eg  = (idata[16]>>5)&0x01;
        ins.OP[MODULATOR2].ksr = (idata[16]>>4)&0x01;
        ins.OP[MODULATOR2].fmult = (idata[16])&0x0F;

        ins.OP[MODULATOR2].attack   = (idata[17]>>4)&0x0F;
        ins.OP[MODULATOR2].decay   = (idata[17])&0x0F;

        ins.OP[MODULATOR2].sustain   = 0x0F - ((idata[18]>>4)&0x0F);
        ins.OP[MODULATOR2].release   = (idata[18])&0x0F;

        ins.OP[MODULATOR2].waveform  = idata[19]&0x07;

        ins.OP[MODULATOR2].ksl   = (idata[20]>>6)&0x03;

        ins.OP[MODULATOR2].level = 0x3F-((idata[21]) & 0x3F);

        ins.connection2 =    idata[22] & 0x01;
        ins.feedback2 =     (idata[22]>>1) & 0x07;

        ins.OP[CARRIER2].am  = (idata[23]>>7)&0x01;
        ins.OP[CARRIER2].vib = (idata[23]>>6)&0x01;
        ins.OP[CARRIER2].eg  = (idata[23]>>5)&0x01;
        ins.OP[CARRIER2].ksr = (idata[23]>>4)&0x01;
        ins.OP[CARRIER2].fmult = (idata[23])&0x0F;

        ins.OP[CARRIER2].attack   = (idata[24]>>4)&0x0F;
        ins.OP[CARRIER2].decay   = (idata[24])&0x0F;

        ins.OP[CARRIER2].sustain   = 0x0F - ((idata[25]>>4)&0x0F);
        ins.OP[CARRIER2].release   = (idata[25])&0x0F;

        ins.OP[CARRIER2].waveform  = idata[26]&0x07;

        ins.OP[CARRIER2].ksl   = (idata[27]>>6)&0x03;

        ins.OP[CARRIER2].level = 0x3F-((idata[28]) & 0x3F);

        //29'th is unused

        ins.note_offset2 = toSint16LE(&idata[30]) + 12;//((short(idata[30])&0x00FF) & ((short(idata[31])<<8) && 0xFF00)) + 12;
    }

    //Instrument names
    for(unsigned short i=0; i<175; i++)
    {
        FmBank::Instrument &ins = (i<128) ? bank.Ins_Melodic[i] : bank.Ins_Percussion[(i-128)+35];
        if(file.read(ins.name, 32)!=32)
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

    for(unsigned short i=0; i<175; i++)
    {
        FmBank::Instrument &ins = (i<128) ? bank.Ins_Melodic[i] : bank.Ins_Percussion[(i-128)+35];
        unsigned short  flags       = 0;
        unsigned char   fine_tuning = 0;
        unsigned char   note_number = 0;
        unsigned char   odata[32];
        memset(odata, 0, 32);

        fine_tuning = uchar(int(ins.fine_tune)+128);
        flags |= (ins.en_4op && ins.en_pseudo4op) ? Dmx_DoubleVoice : 0;
        flags |= (ins.percNoteNum != 0) ? Dmx_FixedPitch : 0;
        flags |= (i==65) ? Dmx_Unknown : 0;
        note_number = ins.percNoteNum;

        odata[0] |= 0x80 & (uchar(ins.OP[MODULATOR1].am)<<7);
        odata[0] |= 0x40 & (uchar(ins.OP[MODULATOR1].vib)<<6);
        odata[0] |= 0x20 & (uchar(ins.OP[MODULATOR1].eg)<<5);
        odata[0] |= 0x10 & (uchar(ins.OP[MODULATOR1].ksr)<<4);
        odata[0] |= 0x0F &  uchar(ins.OP[MODULATOR1].fmult);

        odata[1] |= 0xF0 & uchar(ins.OP[MODULATOR1].attack<<4);
        odata[1] |= 0x0F & ins.OP[MODULATOR1].decay;

        odata[2] |= 0xF0 & (uchar(0x0F-ins.OP[MODULATOR1].sustain)<<4);
        odata[2] |= 0x0F & ins.OP[MODULATOR1].release;

        odata[3] |= 0x07 & ins.OP[MODULATOR1].waveform;

        odata[4] |= 0xC0 & (uchar(ins.OP[MODULATOR1].ksl)<<6);

        odata[5] |= 0x3F & uchar(0x3F-ins.OP[MODULATOR1].level);

        odata[6] |= uchar(ins.connection1);
        odata[6]  |= 0x0E & uchar(ins.feedback1<<1);

        odata[7] |= 0x80 & (uchar(ins.OP[CARRIER1].am)<<7);
        odata[7] |= 0x40 & (uchar(ins.OP[CARRIER1].vib)<<6);
        odata[7] |= 0x20 & (uchar(ins.OP[CARRIER1].eg)<<5);
        odata[7] |= 0x10 & (uchar(ins.OP[CARRIER1].ksr)<<4);
        odata[7] |= 0x0F &  uchar(ins.OP[CARRIER1].fmult);

        odata[8] |= 0xF0 & uchar(ins.OP[CARRIER1].attack<<4);
        odata[8] |= 0x0F & ins.OP[CARRIER1].decay;

        odata[9] |= 0xF0 & (uchar(0x0F-ins.OP[CARRIER1].sustain)<<4);
        odata[9] |= 0x0F & ins.OP[CARRIER1].release;

        odata[10] |= 0x07 & ins.OP[CARRIER1].waveform;

        odata[11] |= 0xC0 & (uchar(ins.OP[CARRIER1].ksl)<<6);

        odata[12] |= 0x3F & uchar(0x3F-ins.OP[CARRIER1].level);

        odata[13] = 0x00;//...but would to use this for something other?

        fromSint16LE(ins.note_offset1 - 12, &odata[14]);

        odata[16] |= 0x80 & (uchar(ins.OP[MODULATOR2].am)<<7);
        odata[16] |= 0x40 & (uchar(ins.OP[MODULATOR2].vib)<<6);
        odata[16] |= 0x20 & (uchar(ins.OP[MODULATOR2].eg)<<5);
        odata[16] |= 0x10 & (uchar(ins.OP[MODULATOR2].ksr)<<4);
        odata[16] |= 0x0F &  uchar(ins.OP[MODULATOR2].fmult);

        odata[17] |= 0xF0 & uchar(ins.OP[MODULATOR2].attack<<4);
        odata[17] |= 0x0F & ins.OP[MODULATOR2].decay;

        odata[18] |= 0xF0 & (uchar(0x0F-ins.OP[MODULATOR2].sustain)<<4);
        odata[18] |= 0x0F & ins.OP[MODULATOR2].release;

        odata[19] |= 0x07 & ins.OP[MODULATOR2].waveform;

        odata[20] |= 0xC0 & (uchar(ins.OP[MODULATOR2].ksl)<<6);

        odata[21] |= 0x3F & uchar(0x3F-ins.OP[MODULATOR2].level);

        odata[22] |= uchar(ins.connection2);
        odata[22]  |= 0x0E & uchar(ins.feedback2<<1);

        odata[23] |= 0x80 & (uchar(ins.OP[CARRIER2].am)<<7);
        odata[23] |= 0x40 & (uchar(ins.OP[CARRIER2].vib)<<6);
        odata[23] |= 0x20 & (uchar(ins.OP[CARRIER2].eg)<<5);
        odata[23] |= 0x10 & (uchar(ins.OP[CARRIER2].ksr)<<4);
        odata[23] |= 0x0F &  uchar(ins.OP[CARRIER2].fmult);

        odata[24] |= 0xF0 & uchar(ins.OP[CARRIER2].attack<<4);
        odata[24] |= 0x0F & ins.OP[CARRIER2].decay;

        odata[25] |= 0xF0 & (uchar(0x0F-ins.OP[CARRIER2].sustain)<<4);
        odata[25] |= 0x0F & ins.OP[CARRIER2].release;

        odata[26] |= 0x07 & ins.OP[CARRIER2].waveform;

        odata[27] |= 0xC0 & (uchar(ins.OP[CARRIER2].ksl)<<6);

        odata[28] |= 0x3F & uchar(0x3F-ins.OP[CARRIER2].level);

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
    for(unsigned short i=0; i<175; i++)
    {
        FmBank::Instrument &ins = (i<128) ? bank.Ins_Melodic[i] : bank.Ins_Percussion[(i-128)+35];
        if(file.write(ins.name, 32)!=32)
        {
            bank.reset();
        }
    }

    file.close();

    return ERR_OK;
}
