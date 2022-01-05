/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_dro_importer.h"
#include "ymf262_to_wopi.h"
#include "../common.h"

template <class T>
static bool readUIntLE(QIODevice &in, T *p)
{
    uint8_t buf[sizeof(T)];
    if(in.read((char *)buf, sizeof(T)) != sizeof(T))
        return false;
    T x = 0;
    for(size_t i = 0; i < sizeof(T); ++i)
        x = (x << 8) | buf[sizeof(T) - 1 - i];
    *p = x;
    return true;
}

bool DRO_Importer::detect(const QString &filePath, char* magic)
{
    Q_UNUSED(filePath);
    return !memcmp(magic, "DBRAWOPL", 8);
}

FfmtErrCode DRO_Importer::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    char magic[8];
    if(file.read(magic, 8) != 8 || memcmp(magic, "DBRAWOPL", 8) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    uint16_t majorVersion = 0;
    uint16_t minorVersion = 0;
    if(!readUIntLE(file, &majorVersion) || !readUIntLE(file, &minorVersion))
        return FfmtErrCode::ERR_BADFORMAT;

    if(majorVersion < 2)
        return loadFileV1(file, bank);

    if(majorVersion == 2 && minorVersion == 0)
        return loadFileV2(file, bank);

    return FfmtErrCode::ERR_BADFORMAT;
}

int DRO_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString DRO_Importer::formatName() const
{
    return "DOSBox Raw OPL";
}

QString DRO_Importer::formatModuleName() const
{
    return "DOSBox Raw OPL importer";
}

QString DRO_Importer::formatExtensionMask() const
{
    return "*.dro";
}

BankFormats DRO_Importer::formatId() const
{
    return BankFormats::FORMAT_DRO_IMPORTER;
}

enum OplMode
{
    OplMode2,
    OplMode2x2,
    OplMode3
};

FfmtErrCode DRO_Importer::loadFileV1(QFile &file, FmBank &bank)
{
    uint32_t lengthMs;
    uint32_t lengthBytes;
    uint32_t hardwareType;
    if(!readUIntLE(file, &lengthMs) || !readUIntLE(file, &lengthBytes) || !readUIntLE(file, &hardwareType))
        return FfmtErrCode::ERR_BADFORMAT;

    if((hardwareType >> 8) != 0)
    {
        // if MSB of hardwareType are non-zero, consider them song data (old format)
        file.seek(file.pos() - 3);
        hardwareType &= 0xff;
    }

    unsigned oplMode = hardwareType & 0xff;
    if(oplMode > 2)
        return FfmtErrCode::ERR_BADFORMAT;

    RawYmf262ToWopi chip[2];
    unsigned nchip = 1;
    if(oplMode == OplMode2x2)
    {
        nchip = 2;
        chip[1].shareInstruments(chip[0]);
    }

    unsigned chipSelect = 0;

    for(uint32_t i = 0; i < lengthBytes;)
    {
        uint8_t reg;
        if(!file.getChar((char *)&reg))
            return FfmtErrCode::ERR_BADFORMAT;
        ++i;

        uint8_t data[2];
        unsigned ndata = 1;

        if(reg == 1 || reg == 4)
            ndata = 2;
        else if(reg == 2 || reg == 3)
            ndata = 0;

        if(file.read((char *)data, ndata) != ndata)
            return FfmtErrCode::ERR_BADFORMAT;
        i += ndata;

        if(reg == 0 || reg == 1) // short delay/long delay
        {
            for (unsigned c = 0; c < nchip; ++c)
                chip[c].doAnalyzeState();
        }
        else if(reg == 2) // select low chip
            chipSelect = 0;
        else if(reg == 3) // select high chip
            chipSelect = 1;
        else // OPL register
        {
            if(reg == 4) // escape
            {
                reg = data[0];
                data[0] = data[1];
            }

            if(chipSelect == 0)
                chip[0].passReg(reg, data[0]);
            else
            {
                if(oplMode == OplMode2x2)
                    chip[1].passReg(reg, data[0]);
                else if(oplMode == OplMode3)
                    chip[0].passReg(reg | 0x100u, data[0]);
            }
        }
    }

    for (unsigned c = 0; c < nchip; ++c)
        chip[c].doAnalyzeState();

    bank.reset();
    bank.Ins_Melodic_box.clear();
    for(const FmBank::Instrument &ins : chip[0].caughtInstruments())
        bank.Ins_Melodic_box.push_back(ins);
    bank.Ins_Melodic = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode DRO_Importer::loadFileV2(QFile &file, FmBank &bank)
{
    uint32_t lengthPairs;
    uint32_t lengthMs;
    uint8_t hardwareType;
    uint8_t format;
    uint8_t compression;
    uint8_t shortDelayCode;
    uint8_t longDelayCode;
    uint8_t lengthCodeMap;
    uint8_t codeMap[256];

    if(!readUIntLE(file, &lengthPairs) ||
       !readUIntLE(file, &lengthMs) ||
       !readUIntLE(file, &hardwareType) ||
       !readUIntLE(file, &format) ||
       !readUIntLE(file, &compression) ||
       !readUIntLE(file, &shortDelayCode) ||
       !readUIntLE(file, &longDelayCode) ||
       !readUIntLE(file, &lengthCodeMap) || (lengthCodeMap >= 0x80) ||
       file.read((char *)codeMap, lengthCodeMap) != lengthCodeMap)
        return FfmtErrCode::ERR_BADFORMAT;

    if(format != 0 || compression != 0)
        return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;

    if(hardwareType > 2)
        return FfmtErrCode::ERR_BADFORMAT;

    RawYmf262ToWopi chip[2];
    unsigned nchip = 1;
    if(hardwareType == OplMode2x2)
    {
        nchip = 2;
        chip[1].shareInstruments(chip[0]);
    }

    for(uint32_t i = 0; i < lengthPairs; ++i)
    {
        uint8_t data[2];
        if(file.read((char *)data, 2) != 2)
            return FfmtErrCode::ERR_BADFORMAT;

        if(data[0] == shortDelayCode || data[0] == longDelayCode)
        {
            for (unsigned c = 0; c < nchip; ++c)
                chip[c].doAnalyzeState();
        }
        else
        {
            unsigned chipSelect = data[0] >> 7;
            unsigned regIndex = data[0] & 0x7f;

            if(regIndex >= lengthCodeMap)
                return FfmtErrCode::ERR_BADFORMAT;

            uint8_t reg = codeMap[regIndex];
            uint8_t val = data[1];

            if(chipSelect && hardwareType == OplMode2x2)
                chip[1].passReg(reg, val);
            else if(chipSelect && hardwareType == OplMode3)
                chip[0].passReg(reg | 0x100u, val);
            else if(!chipSelect)
                chip[0].passReg(reg, val);
        }
    }

    for (unsigned c = 0; c < nchip; ++c)
        chip[c].doAnalyzeState();

    bank.reset();
    bank.Ins_Melodic_box.clear();
    for(const FmBank::Instrument &ins : chip[0].caughtInstruments())
        bank.Ins_Melodic_box.push_back(ins);
    bank.Ins_Melodic = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}
