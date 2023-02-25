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

#include "format_misc_cif.h"
#include "../common.h"
#include <QFileInfo>

static const char* CIF_magic = "<CUD-FM-Instrument>\x1a";

bool Misc_CIF::detectInst(const QString& filePath, char* magic)
{
    Q_UNUSED(filePath);
    return memcmp(magic, CIF_magic, 20) == 0;
}

FfmtErrCode Misc_CIF::loadFileInst(QString filePath, FmBank::Instrument& inst, bool* isDrum)
{
    Q_UNUSED(isDrum);
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    char magic[20];

    if(file.read((char*)magic, 20) != 20 || memcmp(magic, CIF_magic, 20) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t idata[12];

    if(file.read((char*)idata, 12) != 12)
        return FfmtErrCode::ERR_BADFORMAT;

    file.read(inst.name, 20);

    for(int op = 0; op < 2; ++op)
    {
        inst.setAVEKM(op, idata[0 + op]);
        inst.setKSLL(op, idata[2 + op]);
        inst.setAtDec(op, idata[4 + op]);
        inst.setSusRel(op, idata[6 + op]);
        inst.setWaveForm(op, idata[9 + op]);
    }

    inst.setFBConn1(idata[8]);

    return FfmtErrCode::ERR_OK;
}

int Misc_CIF::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN |
           (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString Misc_CIF::formatInstName() const
{
    return "CIF instrument";
}

QString Misc_CIF::formatInstExtensionMask() const
{
    return "*.cif";
}

InstFormats Misc_CIF::formatInstId() const
{
    return InstFormats::FORMAT_INST_CIF;
}
