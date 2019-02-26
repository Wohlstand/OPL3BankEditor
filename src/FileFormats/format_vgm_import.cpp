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

#include "format_vgm_import.h"
#include "ymf262_to_wopi.h"
#include "../common.h"

#include <QSet>
#include <QByteArray>
#include <algorithm>

const char magic_vgm[4] = {0x56, 0x67, 0x6D, 0x20};

bool VGM_Importer::detect(const QString &, char *magic)
{
    return (memcmp(magic_vgm, magic, 4) == 0);
}

FfmtErrCode VGM_Importer::loadFile(QString filePath, FmBank &bank)
{
    RawYmf262ToWopi pseudoOpl2;
    RawYmf262ToWopi pseudoOpl3;
    pseudoOpl3.shareInstruments(pseudoOpl2);

    char    magic[4];
    uint8_t numb[4];

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();
    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(memcmp(magic, magic_vgm, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    file.seek(0x34);
    file.read(char_p(numb), 4);

    uint32_t data_offset = toUint32LE(numb);
    if(data_offset == 0x0C)
        data_offset = 0x40;
    file.seek(data_offset);

    bank.Ins_Melodic_box.clear();

    uint32_t pcm_offset = 0;
    bool end = false;
    while(!end && !file.atEnd())
    {
        uint8_t cmd = 0x00;
        uint8_t reg = 0x00;
        uint8_t val = 0x00;
        file.read(char_p(&cmd), 1);
        switch(cmd)
        {
        case 0x5a: // YM3812, write value dd to register aa
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            pseudoOpl2.passReg(reg, val);
            break;

        case 0x5e:   // YMF262 port 0, write value dd to register aa
        case 0x5f: { // YMF262 port 1, write value dd to register aa
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            uint16_t regopl3 = reg;
            if(cmd == 0x5f) regopl3 |= 0x100u;
            pseudoOpl3.passReg(regopl3, val);
            break;
        }

        case 0x61://Wait samples
        case 0x62://Wait samples
        case 0x63://Wait samples
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F:
            if(cmd == 0x61)
                file.seek(file.pos() + 2);
            pseudoOpl2.doAnalyzeState();
            pseudoOpl3.doAnalyzeState();
            break;

        case 0x66://End of sound data
            end = 1;
            break;

        case 0x67://Data block to skip
            file.seek(file.pos() + 2);
            file.read(char_p(numb), 4);
            pcm_offset = toUint32LE(numb);
            file.seek(file.pos() + pcm_offset);
            break;
        }
    }

    const QList<FmBank::Instrument> &insts = pseudoOpl2.caughtInstruments();
    bank.Ins_Melodic_box.reserve(insts.size());
    for(const FmBank::Instrument &inst : insts)
        bank.Ins_Melodic_box.push_back(inst);
    bank.Ins_Melodic = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}

int VGM_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString VGM_Importer::formatName() const
{
    return "Video Game Music";
}

QString VGM_Importer::formatModuleName() const
{
    return "Video Game Music importer";
}

QString VGM_Importer::formatExtensionMask() const
{
    return "*.vgm";
}

BankFormats VGM_Importer::formatId() const
{
    return BankFormats::FORMAT_VGM_IMPORTER;
}
