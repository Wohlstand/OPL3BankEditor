/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2022 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "ffmt_base.h"

FmBankFormatBase::FmBankFormatBase() {}

FmBankFormatBase::~FmBankFormatBase()
{}

bool FmBankFormatBase::detect(const QString&, char*)
{
    return false;
}

bool FmBankFormatBase::detectInst(const QString &, char *)
{
    return false;
}

FfmtErrCode FmBankFormatBase::loadFile(QString, FmBank &)
{
    return FfmtErrCode::ERR_NOT_IMPLEMENTED;
}

FfmtErrCode FmBankFormatBase::saveFile(QString, FmBank &)
{
    return FfmtErrCode::ERR_NOT_IMPLEMENTED;
}

FfmtErrCode FmBankFormatBase::loadFileInst(QString, FmBank::Instrument &, bool *)
{
    return FfmtErrCode::ERR_NOT_IMPLEMENTED;
}

FfmtErrCode FmBankFormatBase::saveFileInst(QString, FmBank::Instrument &, bool)
{
    return FfmtErrCode::ERR_NOT_IMPLEMENTED;
}

int FmBankFormatBase::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_NOTHING;
}

int FmBankFormatBase::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_NOTHING;
}

QString FmBankFormatBase::formatInstName() const
{
    return "Unknown format";
}

QString FmBankFormatBase::formatInstModuleName() const
{
    return formatInstName();
}

QString FmBankFormatBase::formatInstExtensionMask() const
{
    return "*.*";
}

QString FmBankFormatBase::formatInstDefaultExtension() const
{
    return QString();
}

QString FmBankFormatBase::formatName() const
{
    return "Unknown format";
}

QString FmBankFormatBase::formatModuleName() const
{
    return formatName();
}

QString FmBankFormatBase::formatExtensionMask() const
{
    return "*.*";
}

QString FmBankFormatBase::formatDefaultExtension() const
{
    return QString();
}

BankFormats FmBankFormatBase::formatId() const
{
    return BankFormats::FORMAT_UNKNOWN;
}

InstFormats FmBankFormatBase::formatInstId() const
{
    return InstFormats::FORMAT_INST_UNKNOWN;
}

