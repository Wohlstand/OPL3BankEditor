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

#include <QSet>
#include <QByteArray>
#include <algorithm>

#include "format_imf_importer.h"
#include "../common.h"

#define NUM_OF_CHANNELS     23

static const uint16_t Operators[NUM_OF_CHANNELS * 2] =
{
    // Channels 0-2
    0x000, 0x003, 0x001, 0x004, 0x002, 0x005, // operators  0, 3,  1, 4,  2, 5
    // Channels 3-5
    0x008, 0x00B, 0x009, 0x00C, 0x00A, 0x00D, // operators  6, 9,  7,10,  8,11
    // Channels 6-8
    0x010, 0x013, 0x011, 0x014, 0x012, 0x015, // operators 12,15, 13,16, 14,17
    // Same for second card
    0x100, 0x103, 0x101, 0x104, 0x102, 0x105, // operators 18,21, 19,22, 20,23
    0x108, 0x10B, 0x109, 0x10C, 0x10A, 0x10D, // operators 24,27, 25,28, 26,29
    0x110, 0x113, 0x111, 0x114, 0x112, 0x115, // operators 30,33, 31,34, 32,35
    // Channel 18
    0x010, 0x013,  // operators 12,15
    // Channel 19
    0x014, 0xFFF,  // operator 16
    // Channel 19
    0x012, 0xFFF,  // operator 14
    // Channel 19
    0x015, 0xFFF,  // operator 17
    // Channel 19
    0x011, 0xFFF
}; // operator 13

bool IMF_Importer::detect(const QString &filePath, char *)
{
    if(hasExt(filePath, ".imf"))
        return true;
    return false;
}

FfmtErrCode IMF_Importer::loadFile(QString filePath, FmBank &bank)
{
    uint8_t ymram[0x100];
    bool    keys[9];
    memset(ymram, 0, 0x100);
    memset(keys, 0, sizeof(bool) * 9);

    QSet<QByteArray> cache;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();

    uint32_t imfLen = 0;
    if(readLE(file, imfLen) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.Ins_Melodic_box.clear();
    int insCount = 0;

    imfLen -= 4;
    imfLen -= imfLen % 4;

    while((imfLen > 0) && !file.atEnd())
    {
        imfLen -= 4;
        uint16_t    delay = 0;
        uint8_t     reg = 0;
        uint8_t     val = 0;

        if(readLE(file, delay) != 2)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        if((file.read(char_p(&reg), 1) != 1) || (file.read(char_p(&val), 1) != 1))
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        ymram[reg] = val;

        if((reg >= 0xB0) && (reg <= 0xB8))
            keys[reg & 0x0F] = (val >> 5) & 0x01;

        if(delay != 0)
        {
            for(uint8_t ch = 0; ch < 9; ch++)
            {
                if(!keys[ch])
                    continue;
                QByteArray insRaw;//Raw instrument
                FmBank::Instrument ins = FmBank::emptyInst();
                ins.setAVEKM(MODULATOR1,    ymram[0x20 + Operators[ch * 2]]);
                ins.setAVEKM(CARRIER1,      ymram[0x20 + Operators[(ch * 2) + 1]]);
                ins.setKSLL(MODULATOR1,     ymram[0x40 + Operators[ch * 2]]);
                ins.setKSLL(CARRIER1,       ymram[0x40 + Operators[(ch * 2) + 1]]);
                ins.setAtDec(MODULATOR1,    ymram[0x60 + Operators[ch * 2]]);
                ins.setAtDec(CARRIER1,      ymram[0x60 + Operators[(ch * 2) + 1]]);
                ins.setSusRel(MODULATOR1,   ymram[0x80 + Operators[ch * 2]]);
                ins.setSusRel(CARRIER1,     ymram[0x80 + Operators[(ch * 2) + 1]]);
                ins.setWaveForm(MODULATOR1, ymram[0xE0 + Operators[ch * 2]]);
                ins.setWaveForm(CARRIER1,   ymram[0xE0 + Operators[(ch * 2) + 1]]);
                ins.setFBConn1(ymram[0xC0 + ch]);

                /* Maximize key volume */
                uint8_t olevels[2] =
                {
                    ins.OP[MODULATOR1].level,
                    ins.OP[CARRIER1].level
                };

                uint8_t dec = 0;
                if(ins.connection1 == FmBank::Instrument::Connections::AM)
                {
                    dec = 63 - std::max({olevels[MODULATOR1], olevels[CARRIER1]});
                    ins.OP[MODULATOR1].level= olevels[MODULATOR1] + dec;
                    ins.OP[CARRIER1].level  = olevels[CARRIER1] + dec;
                }
                else
                    ins.OP[CARRIER1].level = 63;

                insRaw.append((char)ins.getAVEKM(MODULATOR1));
                insRaw.append((char)ins.getAVEKM(CARRIER1));
                insRaw.append((char)ins.getKSLL(MODULATOR1));
                insRaw.append((char)ins.getKSLL(CARRIER1));
                insRaw.append((char)ins.getAtDec(MODULATOR1));
                insRaw.append((char)ins.getAtDec(CARRIER1));
                insRaw.append((char)ins.getSusRel(MODULATOR1));
                insRaw.append((char)ins.getSusRel(CARRIER1));
                insRaw.append((char)ins.getWaveForm(MODULATOR1));
                insRaw.append((char)ins.getWaveForm(CARRIER1));
                insRaw.append((char)ins.getFBConn1());

                if(!cache.contains(insRaw))
                {
                    snprintf(ins.name, 32, "Ins-%03d, channel %d", insCount++, (int)(ch));
                    bank.Ins_Melodic_box.push_back(ins);
                    bank.Ins_Melodic = bank.Ins_Melodic_box.data();
                    cache.insert(insRaw);
                }
            }
        }
    }

    file.close();

    return FfmtErrCode::ERR_OK;
}

int IMF_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString IMF_Importer::formatName() const
{
    return "Id-Software Music File";
}

QString IMF_Importer::formatModuleName() const
{
    return "Id-Software Music importer";
}

QString IMF_Importer::formatExtensionMask() const
{
    return "*.imf";
}

BankFormats IMF_Importer::formatId() const
{
    return BankFormats::FORMAT_IMF_IMPORTER;
}
