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

int MilesOPL::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    QByteArray data = file.readAll();
    file.close();

    bank.reset();
    //FILE* fp = std::fopen(fn, "rb");
    //std::fseek(fp, 0, SEEK_END);
    //std::vector<unsigned char> data(std::ftell(fp));
    //std::rewind(fp);
    //std::fread(&data[0], 1, data.size(), fp),
    //std::fclose(fp);
    /*
    for(unsigned a=0; a<2000; ++a)
    {
        unsigned gmnumber  = data[a*6+0];
        unsigned gmnumber2 = data[a*6+1];
        unsigned offset    = *(unsigned*)&data[a*6+2];

        if(gmnumber == 0xFF) break;
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

        char name2[512]; sprintf(name2, "%s%c%u", prefix,
            (gmno<128?'M':'P'), gmno&127);

        insdata tmp[200];

        const unsigned inscount = (length-3)/11;
        for(unsigned i=0; i<inscount; ++i)
        {
            unsigned o = offset + 3 + i*11;
            tmp[i].finetune = (gmno < 128 && i == 0) ? notenum : 0;
            tmp[i].diff = false;
            tmp[i].data[0] = data[o+0];  // 20
            tmp[i].data[8] = data[o+1];  // 40 (vol)
            tmp[i].data[2] = data[o+2];  // 60
            tmp[i].data[4] = data[o+3];  // 80
            tmp[i].data[6] = data[o+4];  // E0
            tmp[i].data[1] = data[o+6];  // 23
            tmp[i].data[9] = data[o+7]; // 43 (vol)
            tmp[i].data[3] = data[o+8]; // 63
            tmp[i].data[5] = data[o+9]; // 83
            tmp[i].data[7] = data[o+10]; // E3

            unsigned fb_c = data[offset+3+5];
            tmp[i].data[10] = fb_c;
            if(i == 1)
            {
                tmp[0].data[10] = fb_c & 0x0F;
                tmp[1].data[10] = (fb_c & 0x0E) | (fb_c >> 7);
            }
        }
        if(inscount == 1) tmp[1] = tmp[0];
        if(inscount <= 2)
        {
            struct ins tmp2;
            tmp2.notenum  = gmno < 128 ? 0 : data[offset+3];
            tmp2.pseudo4op = false;
            tmp2.fine_tune = 0.0;
            std::string name;
            if(midi_index >= 0) name = std::string(1,'\377')+MidiInsName[midi_index];
            size_t resno = InsertIns(tmp[0], tmp[1], tmp2, name, name2);
            SetBank(bank, gmno, resno);
        }
    }*/

    return ERR_NOT_IMLEMENTED;
}

int MilesOPL::saveFile(QString filePath, FmBank &bank)
{
    return ERR_NOT_IMLEMENTED;
}
