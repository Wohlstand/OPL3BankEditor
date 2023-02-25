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

#include "format_misc_hsc.h"
#include "../common.h"
#include <QFileInfo>

bool Misc_HSC::detectInst(const QString& filePath, char*)
{
    return filePath.endsWith(".ins", Qt::CaseInsensitive) &&
           QFileInfo(filePath).size() == 12;
}

FfmtErrCode Misc_HSC::loadFileInst(QString filePath, FmBank::Instrument& inst, bool* isDrum)
{
    Q_UNUSED(isDrum);
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t idata[12];

    if(file.read((char*)idata, 12) != 12)
        return FfmtErrCode::ERR_BADFORMAT;

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

FfmtErrCode Misc_HSC::saveFileInst(QString filePath, FmBank::Instrument& inst, bool isDrum)
{
    Q_UNUSED(isDrum);
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t idata[12];

    for(int op = 0; op < 2; ++op)
    {
        idata[0 + op] = inst.getAVEKM(op);
        idata[2 + op] = inst.getKSLL(op);
        idata[4 + op] = inst.getAtDec(op);
        idata[6 + op] = inst.getSusRel(op);
        idata[9 + op] = inst.getWaveForm(op);
    }

    idata[8] = inst.getFBConn1();

    if(file.write((char*)idata, 12) != 12 || !file.flush())
        return FfmtErrCode::ERR_BADFORMAT;

    return FfmtErrCode::ERR_OK;
}

int Misc_HSC::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString Misc_HSC::formatInstName() const
{
    return "HSC instrument";
}

QString Misc_HSC::formatInstExtensionMask() const
{
    return "*.ins";
}

QString Misc_HSC::formatInstDefaultExtension() const
{
    return "ins";
}

InstFormats Misc_HSC::formatInstId() const
{
    return InstFormats::FORMAT_INST_HSC;
}
