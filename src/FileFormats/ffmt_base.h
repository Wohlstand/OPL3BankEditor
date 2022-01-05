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

#ifndef FMBANKFORMATBASE_H
#define FMBANKFORMATBASE_H

#include <QString>
#include "../bank.h"
#include "ffmt_enums.h"

/*!
 * \brief Base class provides errors enum and commonly used headers
 */
class FmBankFormatBase
{
public:
    FmBankFormatBase();
    virtual ~FmBankFormatBase();

    virtual bool detect(const QString &filePath, char* magic);
    virtual bool detectInst(const QString &filePath, char* magic);

    virtual FfmtErrCode loadFile(QString filePath, FmBank &bank);
    virtual FfmtErrCode saveFile(QString filePath, FmBank &bank);

    virtual FfmtErrCode loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0);
    virtual FfmtErrCode saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum = false);

    virtual int         formatCaps() const;
    virtual QString     formatName() const;
    virtual QString     formatModuleName() const;
    virtual QString     formatExtensionMask() const;
    virtual QString     formatDefaultExtension() const;
    virtual BankFormats formatId() const;

    virtual int         formatInstCaps() const;
    virtual QString     formatInstName() const;
    virtual QString     formatInstModuleName() const;
    virtual QString     formatInstExtensionMask() const;
    virtual QString     formatInstDefaultExtension() const;
    virtual InstFormats formatInstId() const;
};

#endif // FMBANKFORMATBASE_H
