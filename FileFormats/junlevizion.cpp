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

#include "junlevizion.h"
#include "../common.h"

static const char* jv_magic = "Junglevision Patch File\x1A\0\0\0\0\0\0\0\0";

bool JunleVizion::detect(char *magic)
{
    return (strncmp(magic, jv_magic, 32) == 0);
}

int JunleVizion::loadFile(QString filePath, FmBank &bank)
{
    unsigned short count_melodic     = 0;
    unsigned short count_percusive   = 0;
    unsigned short startAt_melodic   = 0;
    unsigned short startAt_percusive = 0;
    char magic[32]; memset(magic, 0, 32);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();

    if(file.read(magic, 32) != 32)
        return ERR_BADFORMAT;

    if( strncmp(magic, jv_magic, 32) != 0 )
        return ERR_BADFORMAT;
    if( readLE(file, count_melodic) != 2)
        return ERR_BADFORMAT;
    if( readLE(file, count_percusive) != 2)
        return ERR_BADFORMAT;
    if( readLE(file, startAt_melodic) != 2)
        return ERR_BADFORMAT;
    if( readLE(file, startAt_percusive) != 2)
        return ERR_BADFORMAT;

    if(count_melodic>128)
        return ERR_BADFORMAT;
    if(count_percusive>128)
        return ERR_BADFORMAT;

    if( (count_melodic+startAt_melodic)>128)
        return ERR_BADFORMAT;
    if( (count_percusive+startAt_percusive)>128)
        return ERR_BADFORMAT;
    unsigned short total = count_melodic + count_percusive;
    for(unsigned short i=0; i<total; i++)
    {
        FmBank::Instrument &ins = (i<count_melodic) ? bank.Ins_Melodic[i+startAt_melodic] : bank.Ins_Percussion[(i-count_melodic)+startAt_percusive];
        unsigned char idata[24];
        if( file.read(char_p(idata), 24) != 24 )
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
//        0   0 - Single, 1 - Dobule instrument
        ins.en_4op = (idata[0]==1);
//        1   NoteNum
        ins.percNoteNum = idata[1];
//        //OP1
//        2   AM/VIB/EG/KSR/Multiple bits   [1-bit Tremolo, 1-bit Vibrato, 1-bit Systain sound, 1-bit Envelope scale (KSR), 4-bit Frequency Multiplication]
        ins.OP[MODULATOR1].am  = (idata[2]>>7)&0x01;
        ins.OP[MODULATOR1].vib = (idata[2]>>6)&0x01;
        ins.OP[MODULATOR1].eg  = (idata[2]>>5)&0x01;
        ins.OP[MODULATOR1].ksr = (idata[2]>>4)&0x01;
        ins.OP[MODULATOR1].fmult = (idata[2])&0x0F;
//        3   KSL/attenuation settings      [2-bit KSL(KeyScaleRate), 6-bit Level (0 max, 63 minimal)]
        ins.OP[MODULATOR1].ksl   = (idata[3]>>6)&0x03;
        ins.OP[MODULATOR1].level = 0x3F-((idata[3]) & 0x3F);
//        4   Attack/decay rates            [4-bit attak (0-max, F-zero),   4-bit decay(0-max, F-zero)]
        ins.OP[MODULATOR1].attack   = (idata[4]>>4)&0x0F;
        ins.OP[MODULATOR1].decay   = (idata[4])&0x0F;
//        5   Sustain/release rates         [4-bit systain(0-max, F-zero), 4-bit release(0-max, F-zero)]
        ins.OP[MODULATOR1].sustain   = 0x0F - ((idata[5]>>4)&0x0F);
        ins.OP[MODULATOR1].release   = (idata[5])&0x0F;
//        6   Wave select settings          [4-bit unused, 4-bit wave-from-0-to-7]
        ins.OP[MODULATOR1].waveform  = idata[6]&0x07;

//        7   Feedback/connection bits  &~0x30  [0000-WAT???  000-feedback - 0-fm,1-am]
        ins.connection1 =  idata[7] & 0x01;
        ins.feedback1   = (idata[7]>>1) & 0x07;

//        //OP2
//        8   AM/VIB/EG/KSR/Multiple bits
        ins.OP[CARRIER1].am  = (idata[8]>>7)&0x01;
        ins.OP[CARRIER1].vib = (idata[8]>>6)&0x01;
        ins.OP[CARRIER1].eg  = (idata[8]>>5)&0x01;
        ins.OP[CARRIER1].ksr = (idata[8]>>4)&0x01;
        ins.OP[CARRIER1].fmult = (idata[8])&0x0F;
//        9   KSL/attenuation settings
        ins.OP[CARRIER1].ksl   = (idata[9]>>6)&0x03;
        ins.OP[CARRIER1].level = 0x3F-((idata[9]) & 0x3F);
//        10  Attack/decay rates
        ins.OP[CARRIER1].attack   = (idata[10]>>4)&0x0F;
        ins.OP[CARRIER1].decay   = (idata[10])&0x0F;
//        11  Sustain/release rates
        ins.OP[CARRIER1].sustain   = 0x0F - ((idata[11]>>4)&0x0F);
        ins.OP[CARRIER1].release   = (idata[11])&0x0F;
//        12  Wave select settings
        ins.OP[CARRIER1].waveform  = idata[12]&0x07;

//        //OP3
//        13  AM/VIB/EG/KSR/Multiple bits
        ins.OP[MODULATOR2].am  = (idata[13]>>7)&0x01;
        ins.OP[MODULATOR2].vib = (idata[13]>>6)&0x01;
        ins.OP[MODULATOR2].eg  = (idata[13]>>5)&0x01;
        ins.OP[MODULATOR2].ksr = (idata[13]>>4)&0x01;
        ins.OP[MODULATOR2].fmult = (idata[13])&0x0F;
//        14  KSL/attenuation settings
        ins.OP[MODULATOR2].ksl   = (idata[14]>>6)&0x03;
        ins.OP[MODULATOR2].level = 0x3F-((idata[14]) & 0x3F);
//        15  Attack/decay rates
        ins.OP[MODULATOR2].attack   = (idata[15]>>4)&0x0F;
        ins.OP[MODULATOR2].decay   = (idata[15])&0x0F;
//        16  Sustain/release rates
        ins.OP[MODULATOR2].sustain   = 0x0F - ((idata[16]>>4)&0x0F);
        ins.OP[MODULATOR2].release   = (idata[16])&0x0F;
//        17  Wave select settings
        ins.OP[MODULATOR2].waveform  = idata[17]&0x07;

//        18  Feedback/connection bits  &~0x30
        ins.connection2 =  idata[18] & 0x01;
        ins.feedback2   = (idata[18]>>1)&0x07;

//        19  AM/VIB/EG/KSR/Multiple bits
        ins.OP[CARRIER2].am  = (idata[19]>>7)&0x01;
        ins.OP[CARRIER2].vib = (idata[19]>>6)&0x01;
        ins.OP[CARRIER2].eg  = (idata[19]>>5)&0x01;
        ins.OP[CARRIER2].ksr = (idata[19]>>4)&0x01;
        ins.OP[CARRIER2].fmult = (idata[19])&0x0F;
//        20  KSL/attenuation settings
        ins.OP[CARRIER2].ksl   = (idata[20]>>6)&0x03;
        ins.OP[CARRIER2].level = 0x3F-((idata[20]) & 0x3F);
//        21  Attack/decay rates
        ins.OP[CARRIER2].attack   = (idata[21]>>4)&0x0F;
        ins.OP[CARRIER2].decay   = (idata[21])&0x0F;
//        22  Sustain/release rates
        ins.OP[CARRIER2].sustain   = 0x0F - ((idata[22]>>4)&0x0F);
        ins.OP[CARRIER2].release   = (idata[22])&0x0F;
//        23  Wave select settings
        ins.OP[CARRIER2].waveform  = idata[23]&0x07;
    }

    file.close();

    return ERR_OK;
}

int JunleVizion::saveFile(QString filePath, FmBank &bank)
{
    FmBank::Instrument null;
    memset(&null, 0, sizeof(FmBank::Instrument));

    unsigned short count_melodic     = 128;
    unsigned short count_percusive   = 128;
    unsigned short startAt_melodic   = 0;
    unsigned short startAt_percusive = 0;

    //Find begin
    for(unsigned short i=0; i<128; i++)
    {
        if( (memcmp(&bank.Ins_Percussion[i], &null, sizeof(FmBank::Instrument))!=0) && (startAt_melodic==0) )
        {
            startAt_percusive = i;
            break;
        }
        count_percusive -= 1;
    }

    //Find tail
    if( count_percusive != 0 )
    {
        for(unsigned short i=127; i>=startAt_percusive; i--)
        {
            if( (memcmp(&bank.Ins_Percussion[i], &null, sizeof(FmBank::Instrument))!=0) && (startAt_melodic==0) )
            {
                break;
            }
            count_percusive -= 1;
        }
    } else {
        startAt_percusive = 0;
    }

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return ERR_NOFILE;

    //Write header
    file.write(char_p(jv_magic), 32);

    writeLE(file, count_melodic);
    writeLE(file, count_percusive);
    writeLE(file, startAt_melodic);
    writeLE(file, startAt_percusive);

    unsigned short total = count_melodic + count_percusive;
    bool had4op=false;
    for(unsigned short i=0; i<total; i++)
    {
        FmBank::Instrument &ins = (i<count_melodic) ? bank.Ins_Melodic[i+startAt_melodic] : bank.Ins_Percussion[(i-count_melodic)+startAt_percusive];
        unsigned char odata[24];
        memset(odata, 0, 24);
        had4op |= ins.en_4op;
//        0   0 - Single, 1 - Dobule instrument
        odata[0] = uchar(ins.en_4op);
//        1   NoteNum
        odata[1] = ins.percNoteNum;

//        //OP1
//        2   AM/VIB/EG/KSR/Multiple bits   [1-bit Tremolo, 1-bit Vibrato, 1-bit Systain sound, 1-bit Envelope scale (KSR), 4-bit Frequency Multiplication]
        odata[2] |= 0x80 & (uchar(ins.OP[MODULATOR1].am)<<7);
        odata[2] |= 0x40 & (uchar(ins.OP[MODULATOR1].vib)<<6);
        odata[2] |= 0x20 & (uchar(ins.OP[MODULATOR1].eg)<<5);
        odata[2] |= 0x10 & (uchar(ins.OP[MODULATOR1].ksr)<<4);
        odata[2] |= 0x0F &  uchar(ins.OP[MODULATOR1].fmult);
//        3   KSL/attenuation settings      [2-bit KSL(KeyScaleRate), 6-bit Level (0 max, 63 minimal)]
        odata[3] |= 0xC0 & (uchar(ins.OP[MODULATOR1].ksl)<<6);
        odata[3] |= 0x3F & uchar(0x3F-ins.OP[MODULATOR1].level);
//        4   Attack/decay rates            [4-bit attak (0-max, F-zero),   4-bit decay(0-max, F-zero)]
        odata[4] |= 0xF0 & uchar(ins.OP[MODULATOR1].attack<<4);
        odata[4] |= 0x0F & ins.OP[MODULATOR1].decay;
//        5   Sustain/release rates         [4-bit systain(0-max, F-zero), 4-bit release(0-max, F-zero)]
        odata[5] |= 0xF0 & (uchar(0x0F-ins.OP[MODULATOR1].sustain)<<4);
        odata[5] |= 0x0F & ins.OP[MODULATOR1].release;
//        6   Wave select settings          [4-bit unused, 4-bit wave-from-0-to-7]
        odata[6] |= 0x07 & ins.OP[MODULATOR1].waveform;
        odata[6] |= had4op ? 0x80 : 0x00;

//        7   Feedback/connection bits  &~0x30  [0000-WAT???  000-feedback - 0-fm,1-am]
        odata[7] |= uchar(ins.connection1);
        odata[7]  |= 0x0E & uchar(ins.feedback1<<1);
        odata[7]  |= had4op ? (0x30 & uchar(3)<<4) : 0;

//        //OP2
//        8   AM/VIB/EG/KSR/Multiple bits
        odata[8] |= 0x80 & (uchar(ins.OP[CARRIER1].am)<<7);
        odata[8] |= 0x40 & (uchar(ins.OP[CARRIER1].vib)<<6);
        odata[8] |= 0x20 & (uchar(ins.OP[CARRIER1].eg)<<5);
        odata[8] |= 0x10 & (uchar(ins.OP[CARRIER1].ksr)<<4);
        odata[8] |= 0x0F &  uchar(ins.OP[CARRIER1].fmult);

//        9   KSL/attenuation settings
        odata[9] |= 0xC0 & (uchar(ins.OP[CARRIER1].ksl)<<6);
        odata[9] |= 0x3F & uchar(0x3F-ins.OP[CARRIER1].level);

//        10  Attack/decay rates
        odata[10] |= 0xF0 & uchar(ins.OP[CARRIER1].attack<<4);
        odata[10] |= 0x0F & ins.OP[CARRIER1].decay;

//        11  Sustain/release rates
        odata[11] |= 0xF0 & (uchar(0x0F-ins.OP[CARRIER1].sustain)<<4);
        odata[11] |= 0x0F & ins.OP[CARRIER1].release;

//        12  Wave select settings
        odata[12] |= 0x07 & ins.OP[CARRIER1].waveform;
        odata[12] |= had4op ? 0x80 : 0x00;

//        //OP3
//        13  AM/VIB/EG/KSR/Multiple bits
        odata[13] |= 0x80 & (uchar(ins.OP[MODULATOR2].am)<<7);
        odata[13] |= 0x40 & (uchar(ins.OP[MODULATOR2].vib)<<6);
        odata[13] |= 0x20 & (uchar(ins.OP[MODULATOR2].eg)<<5);
        odata[13] |= 0x10 & (uchar(ins.OP[MODULATOR2].ksr)<<4);
        odata[13] |= 0x0F &  uchar(ins.OP[MODULATOR2].fmult);
//        14  KSL/attenuation settings
        odata[14] |= 0xC0 & (uchar(ins.OP[MODULATOR2].ksl)<<6);
        odata[14] |= 0x3F & uchar(0x3F-ins.OP[MODULATOR2].level);
//        15  Attack/decay rates
        odata[15] |= 0xF0 & uchar(ins.OP[MODULATOR2].attack<<4);
        odata[15] |= 0x0F & ins.OP[MODULATOR2].decay;
//        16  Sustain/release rates
        odata[16] |= 0xF0 & (uchar(0x0F-ins.OP[MODULATOR2].sustain)<<4);
        odata[16] |= 0x0F & ins.OP[MODULATOR2].release;
//        17  Wave select settings
        odata[17] |= 0x07 & ins.OP[MODULATOR2].waveform;
        odata[17] |= ins.en_4op ? 0x80 : 0x00;

//        18  Feedback/connection bits  &~0x30
        odata[18] |= uchar(ins.connection2);
        odata[18] |= 0x0E & uchar(ins.feedback2 << 1);
        odata[18] |= had4op ? (0x30 & uchar(3)<<4) : 0;

//        19  AM/VIB/EG/KSR/Multiple bits
        odata[19] |= 0x80 & (uchar(ins.OP[CARRIER2].am)<<7);
        odata[19] |= 0x40 & (uchar(ins.OP[CARRIER2].vib)<<6);
        odata[19] |= 0x20 & (uchar(ins.OP[CARRIER2].eg)<<5);
        odata[19] |= 0x10 & (uchar(ins.OP[CARRIER2].ksr)<<4);
        odata[19] |= 0x0F &  uchar(ins.OP[CARRIER2].fmult);
//        20  KSL/attenuation settings
        odata[20] |= 0xC0 & (uchar(ins.OP[CARRIER2].ksl)<<6);
        odata[20] |= 0x3F & uchar(0x3F-ins.OP[CARRIER2].level);
//        21  Attack/decay rates
        odata[21] |= 0xF0 & uchar(ins.OP[CARRIER2].attack<<4);
        odata[21] |= 0x0F & ins.OP[CARRIER2].decay;
//        22  Sustain/release rates
        odata[22] |= 0xF0 & (uchar(0x0F-ins.OP[CARRIER2].sustain)<<4);
        odata[22] |= 0x0F & ins.OP[CARRIER2].release;
//        23  Wave select settings
        odata[23] |= 0x07 & ins.OP[CARRIER2].waveform;
        odata[23] |= ins.en_4op ? 0x80 : 0x00;

        if( file.write(char_p(odata), 24) != 24 )
        {
            return ERR_BADFORMAT;
        }
    }

    file.close();

    return ERR_OK;
}
