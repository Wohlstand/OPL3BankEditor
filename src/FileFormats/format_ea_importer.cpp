/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2026 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_ea_importer.h"
#include "../common.h"

static const char *ea_magic = " !\"#$%()*+,-012345@ABCDEHIJKLMPQRSTU`abcdehijklmpqrstu";

bool EaAdLib_Importer::detect(const QString &filePath, char *)
{
    uint8_t     read_buffer[60];
    QFile       file(filePath);

    memset(read_buffer, 0, 60);

    if(hasExt(filePath, ".dat"))
        return true;

    if(!file.open(QIODevice::ReadOnly))
        return false;

    file.seek(0xD9);

    if(file.read((char*)read_buffer, 60) != 60)
        return false;

    file.close();

    return memcmp(read_buffer, ea_magic, 55) == 0;
}

FfmtErrCode EaAdLib_Importer::loadFile(QString filePath, FmBank &bank)
{
    uint16_t sources[20 + 8], offset;
    uint8_t bytes[10];
    long insno;
    bool isDrum = false;
    uint8_t drumKey = 0;
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    // Copy also the unused instruments
    sources[20] = 0x245;
    sources[21] = 0x24F;
    sources[22] = 0x263;
    sources[23] = 0x277;
    sources[24] = 0x281;
    sources[25] = 0x28B;
    sources[26] = 0x29F;
    sources[27] = 0x2A9;

    bank.reset(1, 1);

    bank.deep_tremolo = false;
    bank.deep_vibrato = false;
    bank.volume_model = FmBank::VOLUME_CMF;

    bank.Ins_Melodic_box.clear();
    bank.Ins_Percussion_box.clear();


    for(size_t i = 0; i < 20; ++i)
    {
        file.seek(0x150 + i);
        if(file.read((char*)bytes, 1) != 1)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        insno = (long)bytes[0];

        file.seek(0x187 + insno * 2);

        if(file.read((char*)bytes, 2) != 2)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        offset = toUint16LE(bytes);
        sources[i] = offset;
    }


    for(size_t i = 0; i < 20 + 8; ++i)
    {
        offset = sources[i];

        file.seek(offset);

        if(file.read((char*)bytes, 10) != 10)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        switch(i)
        {
        case 10:
            drumKey = 0x49;
            isDrum = true;
            break;
        case 18:
            drumKey = 0x17;
            isDrum = true;
            break;
        case 16:
            drumKey = 0x0C;
            isDrum = true;
            break;
        case 17:
            drumKey = 0x01;
            isDrum = true;
            break;
        default:
            isDrum = false;
            break;
        }

        FmBank::Instrument ins = FmBank::emptyInst();

        ins.setAVEKM(MODULATOR1, bytes[0]);
        ins.setKSLL(MODULATOR1, bytes[1]);
        ins.setAtDec(MODULATOR1, bytes[2]);
        ins.setSusRel(MODULATOR1, bytes[3]);

        ins.setAVEKM(CARRIER1, bytes[4]);
        ins.setKSLL(CARRIER1, bytes[5]);
        ins.setAtDec(CARRIER1, bytes[6]);
        ins.setSusRel(CARRIER1, bytes[7]);

        ins.setFBConn1(bytes[8]);
        ins.note_offset1 = int8_t(bytes[9] + 12);

        bank.Ins_Melodic_box.push_back(ins);
        bank.Ins_Melodic = bank.Ins_Melodic_box.data();

        if(isDrum)
        {
            ins.is_fixed_note = true;
            ins.percNoteNum = drumKey;
            bank.Ins_Percussion_box.push_back(ins);
            bank.Ins_Percussion = bank.Ins_Percussion_box.data();
        }
    }

    //Automatically create missing banks
    bank.autocreateMissingBanks();

    return FfmtErrCode::ERR_OK;
}

int EaAdLib_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString EaAdLib_Importer::formatName() const
{
    return "EA AdLib Bank File";
}

QString EaAdLib_Importer::formatModuleName() const
{
    return "EA AdLib instruments importer";
}

QString EaAdLib_Importer::formatExtensionMask() const
{
    return "*.dat";
}

BankFormats EaAdLib_Importer::formatId() const
{
    return BankFormats::FORMAT_EA_IMPORTER;
}
