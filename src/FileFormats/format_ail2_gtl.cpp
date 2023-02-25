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

#include "format_ail2_gtl.h"
#include "../common.h"

bool AIL_GTL::detect(const QString &filePath, char *)
{
    if(hasExt(filePath, ".opl"))
        return true;
    if(hasExt(filePath, ".ad"))
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
    uint8_t  patch  = 0;
    uint8_t  bank   = 0;
    uint32_t offset = 0;
};

FfmtErrCode AIL_GTL::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    GTL_Head head;
    QVector<GTL_Head> heads;
    uint8_t   hdata[6];
    uint8_t   idata[24];
    uint8_t   max_bank_number = 0;
    heads.reserve(256);
    do
    {
        if(file.read(char_p(hdata), 6) != 6)
            return FfmtErrCode::ERR_BADFORMAT;

        head.patch = hdata[0];
        head.bank  = hdata[1];
        head.offset = toUint32LE(hdata + 2);

        if((head.patch == 0xFF) || (head.bank == 0xFF))
            break;

        if(head.patch > 127)//Patch ID is more than 127
            return FfmtErrCode::ERR_BADFORMAT;

        if((head.bank != 0x7F) && (head.bank > max_bank_number) )
            max_bank_number = head.bank;

        heads.push_back(head);
    }
    while(!file.atEnd());

    bank.reset(max_bank_number + 1, 1);

    bank.deep_tremolo = true;
    bank.deep_vibrato = true;
    bank.volume_model = FmBank::VOLUME_AIL;

    {
        uint8_t bank_lsb_counter = 0;
        uint8_t bank_msb_counter = 0;
        for(FmBank::MidiBank &b : bank.Banks_Melodic)
        {
            b.lsb = bank_lsb_counter++;
            b.msb = bank_msb_counter;
            if(bank_lsb_counter == 0)
                bank_msb_counter++;
        }
    }

    uint32_t totalInsts = static_cast<uint32_t>(heads.size());
    for(uint32_t i = 0; i < totalInsts; i++)
    {
        GTL_Head &h = heads[int(i)];
        bool isPerc = (h.bank == 0x7F);
        int gmPatchId = isPerc ? h.patch : (h.patch + (h.bank * 128));
        FmBank::Instrument &ins = isPerc ? bank.Ins_Percussion[gmPatchId] : bank.Ins_Melodic[gmPatchId];

        if(!file.seek(h.offset))
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        uint16_t insLen = 0;
        if(readLE(file, insLen) != 2)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        if(insLen < 14)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        insLen -= 2;

        memset(idata, 0, 24);
        if(insLen < 24)
        {
            file.read(char_p(idata), insLen);
        }
        else
        {
            file.read(char_p(idata), 24);
            file.seek(file.pos() + (insLen - 24));
        }
        /*qint64 got = file.read(char_p(idata), insLen);*/
        //if(got != insLen)
        //{
        //    bank.reset();
        //    return FfmtErrCode::ERR_BADFORMAT;
        //}
        //Operators mode: length 12 - 2-op, 23 - 4-op
        ins.en_4op = (insLen / 11) > 1;
        //NoteNum
        ins.percNoteNum  = (isPerc) ? idata[0] : 0;
        ins.note_offset1 = (isPerc) ? 0 : static_cast<char>(idata[0]);
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
            uint8_t fb_c = idata[6]; //idata[17] is always zero, true FB field is bitwisely concatenated with idata[6]
            ins.setFBConn1(fb_c & 0x0F);
            ins.setFBConn2((fb_c & 0x0E) | (fb_c >> 7));
            //OP4
            ins.setAVEKM(CARRIER2,    idata[18]);
            ins.setKSLL(CARRIER2,     idata[19]);
            ins.setAtDec(CARRIER2,    idata[20]);
            ins.setSusRel(CARRIER2,   idata[21]);
            ins.setWaveForm(CARRIER2, idata[22]);
        }

        if(file.atEnd())
            break;//Nothing to read!
    }
    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode AIL_GTL::saveFile(QString filePath, FmBank &bank)
{
    FmBank::Instrument null;
    memset(&null, 0, sizeof(FmBank::Instrument));

    GTL_Head head;
    head.bank  = 0;
    head.patch = 0;
    head.offset = 0;
    QVector<GTL_Head> heads;

    //1) Count non-empty instruments
    for(int i = 0; i < bank.countMelodic(); i++)
    {
        if(memcmp(&bank.Ins_Melodic[i], &null, sizeof(FmBank::Instrument)) != 0)
        {
            FmBank::Instrument &ins = bank.Ins_Melodic[i];
            head.patch = i % 128;
            head.bank  = uint8_t(i / 128);
            heads.push_back(head);
            head.offset += (ins.en_4op && !ins.en_pseudo4op) ? 25 : 14;
        }
    }

    for(int i = 0; i < bank.countDrums(); i++)
    {
        if(memcmp(&bank.Ins_Percussion[i], &null, sizeof(FmBank::Instrument)) != 0)
        {
            FmBank::Instrument &ins = bank.Ins_Percussion[i];
            head.patch = i % 128;
            head.bank  = 0x7F;
            heads.push_back(head);
            head.offset += (ins.en_4op && !ins.en_pseudo4op) ? 25 : 14;
        }
    }

    // Close the header
    head.patch = 0xFF;
    head.bank  = 0xFF;
    head.offset = 0;
    heads.push_back(head);

    // Calculate the global offset
    uint32_t ins_offset = uint32_t((heads.size() * 6) - 4);

    // Open the file
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    //2) Build the header
    for(GTL_Head &h : heads)
    {
        file.write(char_p(&h.patch), 1);
        file.write(char_p(&h.bank),  1);
        uint32_t offset = ins_offset + h.offset;
        writeLE(file, offset);
    }

    file.seek(ins_offset);

    //3) Sequentially write all instruments into the file
    uint8_t odata[24];
    for(int i = 0; i < heads.size() - 1; i++)
    {
        GTL_Head &h = heads[i];
        FmBank::Instrument &ins = (h.bank != 0x7F) ?
                                bank.Ins_Melodic[h.patch + (h.bank * 128)] :
                                bank.Ins_Percussion[h.patch];
        bool is4op = (ins.en_4op && !ins.en_pseudo4op);
        uint16_t ins_len = is4op ? 25 : 14;

//        //Operators mode: length 12 - 2-op, 23 - 4-op
        writeLE(file, ins_len);
        ins_len -= 2;
//        //NoteNum
//        ins.percNoteNum  = (gmPatchId < 128) ? 0 : idata[0];
//        ins.note_offset1 = (gmPatchId < 128) ? static_cast<char>(idata[0]) : 0;
        odata[0] = h.bank == 0x7F ? clip_u8(ins.percNoteNum + ins.note_offset1, 0, 127) : uint8_t(ins.note_offset1);
//        //OP1
        odata[1] = ins.getAVEKM(MODULATOR1);
        odata[2] = ins.getKSLL(MODULATOR1);
        odata[3] = ins.getAtDec(MODULATOR1);
        odata[4] = ins.getSusRel(MODULATOR1);
        odata[5] = ins.getWaveForm(MODULATOR1);
//        //Feedback/Connection 1<->2
        odata[6] = ins.getFBConn1();
//        //OP2
        odata[7] = ins.getAVEKM(CARRIER1);
        odata[8] = ins.getKSLL(CARRIER1);
        odata[9] = ins.getAtDec(CARRIER1);
        odata[10] = ins.getSusRel(CARRIER1);
        odata[11] = ins.getWaveForm(CARRIER1);

        if(is4op)
        {
//            //OP3
            odata[12] = ins.getAVEKM(MODULATOR2);
            odata[13] = ins.getKSLL(MODULATOR2);
            odata[14] = ins.getAtDec(MODULATOR2);
            odata[15] = ins.getSusRel(MODULATOR2);
            odata[16] = ins.getWaveForm(MODULATOR2);
//            //Feedback/Connection 3<->4
//            uint8_t fb_c = idata[6]; //idata[17] is always zero, true FB field is bitwisely concatenated with idata[6]
            odata[17] = 0;
            odata[6] = uint8_t(ins.getFBConn1() | (ins.getFBConn2() << 7));
//            ins.setFBConn1(fb_c & 0x0F);
//            ins.setFBConn2((fb_c & 0x0E) | (fb_c >> 7));
//            //OP4
            odata[18] = ins.getAVEKM(CARRIER2);
            odata[19] = ins.getKSLL(CARRIER2);
            odata[20] = ins.getAtDec(CARRIER2);
            odata[21] = ins.getSusRel(CARRIER2);
            odata[22] = ins.getWaveForm(CARRIER2);
        }

        if(file.write(char_p(odata), ins_len) != ins_len)
            return FfmtErrCode::ERR_BADFORMAT;
    }
    file.close();

    return FfmtErrCode::ERR_OK;
}

int AIL_GTL::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString AIL_GTL::formatName() const
{
    return "Audio Interface Library (Miles) bank";
}

QString AIL_GTL::formatExtensionMask() const
{
    return "*.opl *.ad";
}

QString AIL_GTL::formatDefaultExtension() const
{
    return "opl";
}

BankFormats AIL_GTL::formatId() const
{
    return BankFormats::FORMAT_AIL2;
}
