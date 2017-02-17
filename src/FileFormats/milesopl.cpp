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
B_fb_c          db ?    ;//Stores FeedBack-Connection field for BOTH 2op and 4op pairs! (0000 1111) (2000 1110)
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
O_fb_c          db ?    ;//Is always zero
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

struct GTL_Head // GTL file header entry structure
{
    uint8_t  patch;
    uint8_t  bank;
    uint32_t offset;
};

int MilesOPL::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    GTL_Head head;
    QVector<GTL_Head> heads;
    unsigned char   hdata[6];
    unsigned char   idata[24];
    heads.reserve(256);
    do
    {
        if(file.read(char_p(hdata), 6) != 6)
            return ERR_BADFORMAT;
        head.patch = hdata[0];
        head.bank  = hdata[1];
        head.offset= toUint32LE(hdata + 2);

        if((head.patch == 0xFF) || (head.bank == 0xFF))
            break;

        heads.push_back(head);
    }
    while( !file.atEnd() );

    bank.reset();

    unsigned int totalInsts = static_cast<unsigned int>(heads.size());
    for(unsigned int i = 0; i < totalInsts; i++)
    {
        GTL_Head &h = heads[i];
        int gmno = h.bank == 0x7F ? h.patch + 0x80 : h.patch;
        FmBank::Instrument &ins = (gmno < 128) ? bank.Ins_Melodic[gmno] : bank.Ins_Percussion[gmno - 128];

        if(!file.seek(h.offset))
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
        uint16_t insLen = 0;
        if(readLE(file, insLen) != 2)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        if((insLen < 14) || (insLen > 25))
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        insLen -= 2;

        memset(idata, 0, 24);
        if(file.read(char_p(idata), insLen) != insLen)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
        //Operators mode: length 12 - 2-op, 23 - 4-op
        ins.en_4op = ( insLen / 11) > 1;
        //NoteNum
        ins.percNoteNum  = (gmno < 128) ? 0 : idata[0];
        ins.note_offset1 = (gmno < 128) ? static_cast<char>(idata[0]) : 0;
        //OP1
        ins.setAVEKM(MODULATOR1,    idata[1]);
        ins.setKSLL(MODULATOR1,     idata[2]);
        ins.setAtDec(MODULATOR1,    idata[3]);
        ins.setSusRel(MODULATOR1,   idata[4]);
        ins.setWaveForm(MODULATOR1, idata[5]);
        //Feedback/Connection 1<->2
        ins.setFBConn1(idata[6]);
        //OP2
        ins.setAVEKM(CARRIER1,    idata[7]);
        ins.setKSLL(CARRIER1,     idata[8]);
        ins.setAtDec(CARRIER1,    idata[9]);
        ins.setSusRel(CARRIER1,   idata[10]);
        ins.setWaveForm(CARRIER1, idata[11]);

        if(ins.en_4op)
        {
            //OP3
            ins.setAVEKM(MODULATOR2,    idata[12]);
            ins.setKSLL(MODULATOR2,     idata[13]);
            ins.setAtDec(MODULATOR2,    idata[14]);
            ins.setSusRel(MODULATOR2,   idata[15]);
            ins.setWaveForm(MODULATOR2, idata[16]);
            //Feedback/Connection 3<->4
            uint8_t fb_c = idata[6]; //idata[17] is always zero, true FB field is bitwisely concoctated with idata[6]
            ins.setFBConn1( fb_c & 0x0F );
            ins.setFBConn2( (fb_c & 0x0E) | (fb_c >> 7) );
            //OP4
            ins.setAVEKM(CARRIER2,    idata[18]);
            ins.setKSLL(CARRIER2,     idata[19]);
            ins.setAtDec(CARRIER2,    idata[20]);
            ins.setSusRel(CARRIER2,   idata[21]);
            ins.setWaveForm(CARRIER2, idata[22]);
        }
    }
    file.close();

    return ERR_OK;
}

int MilesOPL::saveFile(QString filePath, FmBank &bank)
{
    return ERR_NOT_IMLEMENTED;
}
