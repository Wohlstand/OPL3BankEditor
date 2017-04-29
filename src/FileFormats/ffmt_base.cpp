/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "../common.h"

#include <memory>
#include <list>
#include "format_adlibbnk.h"
#include "format_apogeetmb.h"
#include "format_dmxopl2.h"
#include "format_junlevizion.h"
#include "format_milesopl.h"
#include "format_sb_ibk.h"
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
    return FfmtErrCode::ERR_NOT_IMLEMENTED;
}

FfmtErrCode FmBankFormatBase::saveFile(QString, FmBank &)
{
    return FfmtErrCode::ERR_NOT_IMLEMENTED;
}

FfmtErrCode FmBankFormatBase::loadFileInst(QString, FmBank::Instrument &, bool *)
{
    return FfmtErrCode::ERR_NOT_IMLEMENTED;
}

FfmtErrCode FmBankFormatBase::saveFileInst(QString, FmBank::Instrument &, bool)
{
    return FfmtErrCode::ERR_NOT_IMLEMENTED;
}

int FmBankFormatBase::formatCaps()
{
    return (int)FormatCaps::FORMAT_CAPS_NOTHING;
}

int FmBankFormatBase::formatInstCaps()
{
    return (int)FormatCaps::FORMAT_CAPS_NOTHING;
}

QString FmBankFormatBase::formatInstName()
{
    return "Unknown format";
}

QString FmBankFormatBase::formatInstExtensionMask()
{
    return "*.*";
}

QString FmBankFormatBase::formatName()
{
    return "Unknown format";
}

QString FmBankFormatBase::formatExtensionMask()
{
    return "*.*";
}

BankFormats FmBankFormatBase::formatId()
{
    return BankFormats::FORMAT_UNKNOWN;
}

InstFormats FmBankFormatBase::formatInstId()
{
    return InstFormats::FORMAT_INST_UNKNOWN;
}

