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

#include "format_misc_sgi.h"
#include "../common.h"
#include <QFileInfo>

bool Misc_SGI::detectInst(const QString& filePath, char* magic)
{
    (void)magic;

    //By name extension
    if(filePath.endsWith(".sgi", Qt::CaseInsensitive))
        return true;

    //By file size :-P
    QFileInfo f(filePath);
    uint64_t size = f.size();

    if(size == 26)
        return true;

    return false;
}

FfmtErrCode Misc_SGI::loadFileInst(QString filePath, FmBank::Instrument& inst, bool* isDrum)
{
    Q_UNUSED(isDrum);
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t idata[26];

    if(file.read((char*)idata, 26) != 26)
        return FfmtErrCode::ERR_BADFORMAT;

    for(unsigned op = 0; op < 2; ++op)
    {
        // FmBank::Operator &o = inst.OP[op];
        unsigned i = 12 * (op ^ 1);
        inst.setAtDec(op, (idata[i + 1] & 0xf) | ((idata[i + 0] & 0xf) << 4));
        inst.setSusRel(op, (idata[i + 3]  & 0xf) | ((idata[i + 2]  & 0xf) << 4));
        inst.setWaveForm(op, idata[i + 4] & 3);
        inst.setKSLL(op, (idata[i + 7]  & 0x3f) | ((idata[i + 6]  & 3) << 6));
        inst.setAVEKM(op, (idata[i + 5]  & 0x0f) | ((idata[i + 8]  & 1) << 4) |
                      ((idata[i + 11] & 1) << 5) | ((idata[i + 10] & 1) << 6) |
                      ((idata[i + 9]  & 1) << 7));
    }

    inst.setFBConn1((idata[25] & 1) | ((idata[24] & 7) << 1));

    std::string name = QFileInfo(filePath).baseName().toStdString();
    size_t namelen = name.size();
    namelen = (namelen < 32) ? namelen : 32;
    memcpy(inst.name, name.data(), namelen);

    return FfmtErrCode::ERR_OK;
}

int Misc_SGI::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN |
           (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString Misc_SGI::formatInstName() const
{
    return "SGI instrument";
}

QString Misc_SGI::formatInstExtensionMask() const
{
    return "*.sgi";
}

InstFormats Misc_SGI::formatInstId() const
{
    return InstFormats::FORMAT_INST_SGI;
}
