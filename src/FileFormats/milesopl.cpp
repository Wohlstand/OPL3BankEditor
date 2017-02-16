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

#include "milesopl.h"
#include "../common.h"

bool MilesOPL::detect(QString filePath)
{
    if( hasExt(filePath, ".opl") )
        return true;
    if( hasExt(filePath, ".ad") )
        return true;

    return false;
}

/*
==================================================================================
 File specification, extracted from AIL source codes (most of them are ASM-coded)
==================================================================================

struct GTL Head                  // GTL file header entry structure
{
    uint8_t  patch;
    uint8_t  bank;
    uint32_t offset;
}
//- Length is 6 bytes

//Look for timbre until .patch will be equal == 0xFF) break;

Note:
    DW - Define Word - uint16_t
    DB - Define Byte - uint8_t

BNK             STRUC           ;.BNK-style timbre definition
B_length        dw ?            ; lenght of timbre entry
B_transpose     db ? ; relative signed offset per each melodic note, and absolute note number on the drums patches
B_mod_AVEKM     db ?            ;op_0 = FM modulator
B_mod_KSLTL     db ?
B_mod_AD        db ?
B_mod_SR        db ?
B_mod_WS        db ?
B_fb_c          db ?
B_car_AVEKM     db ?            ;op_1 = FM carrier
B_car_KSLTL     db ?
B_car_AD        db ?
B_car_SR        db ?
B_car_WS        db ?
                ENDS

OPL3BNK         STRUC           ;.BNK-style OPL3 timbre definition
                BNK <>
O_mod_AVEKM     db ?            ;op_2
O_mod_KSLTL     db ?
O_mod_AD        db ?
O_mod_SR        db ?
O_mod_WS        db ?
O_fb_c          db ?
O_car_AVEKM     db ?            ;op_3
O_car_KSLTL     db ?
O_car_AD        db ?
O_car_SR        db ?
O_car_WS        db ?
                ENDS

=== HOW IT WORKS ====

AIL Volime model:

sbb al,-1               ;(error = 1/127 units; round up if !0)
mov vol,al              ;AX=composite (vol+expression) volume

mul cl                  ;calculate right-channel volume
shl ax,1                ;(AX*2)/256 = AX/128 ÷ AX/127

*/

int MilesOPL::loadFile(QString filePath, FmBank &bank)
{
#warning AIL OPL bank format is under construction
#if 0 //WIP
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    QByteArray fileData = file.readAll();
    unsigned char*  data = (unsigned char*)fileData.data();

    file.close();

    bank.reset();
    //FILE* fp = std::fopen(fn, "rb");
    //std::fseek(fp, 0, SEEK_END);
    //std::vector<unsigned char> data(std::ftell(fp));
    //std::rewind(fp);
    //std::fread(&data[0], 1, data.size(), fp),
    //std::fclose(fp);
    for(unsigned a=0; a<2000; ++a)
    {
        unsigned gmnumber  = data[a*6+0];
        unsigned gmnumber2 = data[a*6+1];
        unsigned offset    = *(unsigned*)&data[a*6+2];

        if(gmnumber == 0xFF)
            break;
        int gmno = gmnumber2==0x7F ? gmnumber+0x80 : gmnumber;
        int midi_index = gmno < 128 ? gmno
                       : gmno < 128+35 ? -1
                       : gmno < 128+88 ? gmno-35
                       : -1;

        unsigned length = data[offset] + data[offset+1]*256;
        signed char notenum = data[offset+2];

        //printf("%02X %02X %08X ", gmnumber,gmnumber2, offset);
        //for(unsigned b=0; b<length; ++b)
        //{
        //if(b > 3 && (b-3)%11 == 0) printf("\n                        ");
        //printf("%02X ", data[offset+b]);
        //}
        //printf("\n");

        if(gmnumber2 != 0 && gmnumber2 != 0x7F) continue;

        /*
        char name2[512]; sprintf(name2, "%s%c%u", prefix,
            (gmno<128?'M':'P'), gmno&127);
        */

        //insdata tmp[200];

        const unsigned inscount = (length-3)/11;
        for(unsigned i=0; i<inscount; ++i)
        {
            FmBank::Instrument &ins = (i<128) ?
                                       bank.Ins_Melodic[i] :
                                       bank.Ins_Percussion[(i-128)+35];
            unsigned o = offset + 3 + i*11;
            //ins.finetune = (gmno < 128 && i == 0) ? notenum : 0;
            ins.setAVEKM(MODULATOR1, data[o+0]);
            //tmp[i].data[0] = data[o+0]; // 20 //Mod1
            ins.setKSLL(MODULATOR1, data[o+1]);
            //tmp[i].data[8] = data[o+1]; // 40 (vol)//Mod1
            ins.setAtDec(MODULATOR1, data[o+2]);
            //tmp[i].data[2] = data[o+2]; // 60//Mod1
            ins.setSusRel(MODULATOR1, data[o+3]);
            //tmp[i].data[4] = data[o+3]; // 80//Mod1
            ins.setWaveForm(MODULATOR1, data[o+4]);
            //tmp[i].data[6] = data[o+4]; // E0//Mod1

            ins.setAVEKM(CARRIER1, data[o+6]);
            //tmp[i].data[1] = data[o+6]; // 23//Car1
            ins.setKSLL(CARRIER1, data[o+7]);
            //tmp[i].data[9] = data[o+7]; // 43 (vol)//Car1
            ins.setAtDec(CARRIER1, data[o+8]);
            //tmp[i].data[3] = data[o+8]; // 63//Car1
            ins.setSusRel(CARRIER1, data[o+9]);
            //tmp[i].data[5] = data[o+9]; // 83
            ins.setWaveForm(CARRIER1, data[o+10]);
            //tmp[i].data[7] = data[o+10];// E3

            /*
            tmp.data[0] = data[offset + 0];//setAVEKM(MODULATOR1
            tmp.data[1] = data[offset + 1];//setAVEKM(CARRIER1
            tmp.data[2] = data[offset + 4];//setAtDec(MODULATOR1
            tmp.data[3] = data[offset + 5];//setAtDec(CARRIER1
            tmp.data[4] = data[offset + 6];//setSusRel(MODULATOR1
            tmp.data[5] = data[offset + 7];//setSusRel(CARRIER1
            tmp.data[6] = data[offset + 8];//setWaveForm(MODULATOR1
            tmp.data[7] = data[offset + 9];//setWaveForm(CARRIER1
            tmp.data[8] = data[offset + 2];//setKSLL(MODULATOR1
            tmp.data[9] = data[offset + 3];//setKSLL(CARRIER1
            tmp.data[10] = data[offset + 10];//setFBConn1(idata[10]);
            */

//    (i->first.data[6] << 24) Wave select settings
//  + (i->first.data[4] << 16) Sustain/release rates
//  + (i->first.data[2] << 8)  Attack/decay rates
//  + (i->first.data[0] << 0); AM/VIB/EG/KSR/Multiple bits

//    (i->first.data[7] << 24) Wave select settings
//  + (i->first.data[5] << 16) Sustain/release rates
//  + (i->first.data[3] << 8)  Attack/decay rates
//  + (i->first.data[1] << 0); AM/VIB/EG/KSR/Multiple bits
//    i->first.data[8],    KSL/attenuation settings\n"
//    i->first.data[9],    KSL/attenuation settings\n"
//    i->first.data[10],   Feedback/connection bits
//    i->first.finetune,
//    i->first.diff?"true":"false");

            unsigned fb_c = data[offset+3+5];
            ins.setFBConn1(fb_c);
            //tmp[i].data[10] = fb_c;
            /*
            if(i == 1)
            {
                tmp[0].data[10] = fb_c & 0x0F;
                tmp[1].data[10] = (fb_c & 0x0E) | (fb_c >> 7);
            }*/
        }

        if(inscount == 1) tmp[1] = tmp[0];
        if(inscount <= 2)
        {
            struct ins tmp2;
            tmp2.notenum  = gmno < 128 ? 0 : data[offset+3];
            tmp2.pseudo4op = false;
            tmp2.fine_tune = 0.0;
            std::string name;
            if(midi_index >= 0) name = std::string(1,'\377') + MidiInsName[midi_index];
            size_t resno = InsertIns(tmp[0], tmp[1], tmp2, name, name2);
            SetBank(bank, gmno, resno);
        }
    }
#endif
    return ERR_NOT_IMLEMENTED;
}

int MilesOPL::saveFile(QString filePath, FmBank &bank)
{
    return ERR_NOT_IMLEMENTED;
}
