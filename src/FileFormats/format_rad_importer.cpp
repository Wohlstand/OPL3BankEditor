/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_rad_importer.h"
#include "../common.h"

static const char *rad_magic = "RAD by REALiTY!!";

bool RAD_Importer::detect(const QString &, char *magic)
{
    return (strncmp(magic, rad_magic, 16) == 0);
}

FfmtErrCode RAD_Importer::loadFile(QString filePath, FmBank &bank)
{
    char        magic[16];
    uint8_t     head[2];
    bool        has_description = false;
    FmBank::Instrument ins = FmBank::emptyInst();

    QFile file(filePath);
    memset(magic, 0, 16);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    // ========== HEADER ==========
    // Offset  00..0F:"RAD by REALiTY!!"
    if(file.read(magic, 16) != 16)
        return FfmtErrCode::ERR_BADFORMAT;
    if(strncmp(magic, rad_magic, 16) != 0)
        return FfmtErrCode::ERR_BADFORMAT;
    // Offset      10:BCD                - Version of RAD file. (Currently 10h for v1.0.)
    // Offset      11:Bit 7              - Set if a description follows.
    //                Bit 6              - Set if it's a "slow-timer" tune.
    //                Bits 4..0          - The initial speed of the tune.
    if(file.read(char_p(&head), 2) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(head[0] > 0x10)
        return FfmtErrCode::ERR_BADFORMAT;

    has_description = (head[1] >> 7) & 0x01;
    //bool    slow_timer_tune = (head[1] >> 6) & 0x01;
    //uint8_t initial_speed   =  head[1] & 0x1F;
    if(has_description)
    {
        // ========== DESCRIPTION ==========
        // The description follows the header if Bit 7 of Offset 11h is set.  The
        // description is a null-terminated string.  To save some space in the
        // tune file, the characters mean:
        //
        // Char 00      - End of description (null-terminator)
        // Char 01      - Move to start of next line (Carriage return)
        // Char 02..1F  - Output this many spaces
        // Char 20..FF  - Output this character
        uint8_t description_byte = 0;
        do
        {
            if(file.read(char_p(&description_byte), 1) != 1)
                return FfmtErrCode::ERR_BADFORMAT;
        } while(description_byte != 0);
    }

    bank.reset();
    bank.Ins_Melodic_box.clear();
    bank.Ins_Percussion_box.clear();

    uint8_t idata[11];
    uint8_t instrument_number = 0;
    do
    {
        // ========== INSTRUMENTS ==========
        memset(idata, 0, 11);
        // Offset      00:Instrument Number that follows (or 0 for end of list)
        if(file.read(char_p(&instrument_number), 1) != 1)
            return FfmtErrCode::ERR_BADFORMAT;
        if(instrument_number == 0)
            break;
        // Offset  01..0B:The instrument definition in HSC .INS format
        if(file.read(char_p(&idata), 11) != 11)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
        ins.setAVEKM(CARRIER1,      idata[0]);
        ins.setAVEKM(MODULATOR1,    idata[1]);
        ins.setKSLL(CARRIER1,       idata[2]);
        ins.setKSLL(MODULATOR1,     idata[3]);
        ins.setAtDec(CARRIER1,      idata[4]);
        ins.setAtDec(MODULATOR1,    idata[5]);
        ins.setSusRel(CARRIER1,     idata[6]);
        ins.setSusRel(MODULATOR1,   idata[7]);
        ins.setFBConn1(idata[8]);
        ins.setWaveForm(CARRIER1,   idata[9]);
        ins.setWaveForm(MODULATOR1, idata[10]);
        snprintf(ins.name, 32, "RAD Instr. %03u", instrument_number);
        bank.Ins_Melodic_box.push_back(ins);
    } while(instrument_number > 0);
    // ========== ORDER LIST ==========
    // ....
    // We have found all necessary to us instruments, therefore just stop reading the file
    // ....
    file.close();

    bank.Ins_Percussion = bank.Ins_Percussion_box.data();
    bank.Ins_Melodic    = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}

int RAD_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString RAD_Importer::formatName() const
{
    return "Reality ADlib Tracker Music file";
}

QString RAD_Importer::formatModuleName() const
{
    return "Reality ADlib music importer";
}

QString RAD_Importer::formatExtensionMask() const
{
    return "*.rad";
}

BankFormats RAD_Importer::formatId() const
{
    return BankFormats::FORMAT_RAD_IMPORTER;
}
