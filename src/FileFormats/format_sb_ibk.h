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

#ifndef SBIBK_H
#define SBIBK_H

#include "ffmt_base.h"

class SbIBK_DOS final : public FmBankFormatBase
{
public:
    SbIBK_DOS();
    ~SbIBK_DOS() = default;

    bool    detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int     formatCaps() override;
    QString formatName() override;
    QString formatExtensionMask() override;
    BankFormats formatId() override;

    bool        detectInst(const QString &filePath, char* magic) override;
    FfmtErrCode     loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0) override;
    int         formatInstCaps() override;
    QString     formatInstName() override;
    QString     formatInstExtensionMask() override;
    InstFormats  formatInstId() override;
};

class SbIBK_UNIX_READ final : public FmBankFormatBase
{
    BankFormats m_recentFormat = BankFormats::FORMAT_UNKNOWN;
public:
    SbIBK_UNIX_READ();
    ~SbIBK_UNIX_READ() = default;

    bool    detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    int     formatCaps() override;
    QString formatName() override;
    QString formatExtensionMask() override;
    BankFormats formatId() override;
};


class SbIBK_UNIX2OP_SAVE final : public FmBankFormatBase
{
public:
    SbIBK_UNIX2OP_SAVE();
    ~SbIBK_UNIX2OP_SAVE() = default;

    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int     formatCaps() override;
    QString formatName() override;
    QString formatExtensionMask() override;
    BankFormats formatId() override;
};


class SbIBK_UNIX4OP_SAVE final : public FmBankFormatBase
{
public:
    SbIBK_UNIX4OP_SAVE();
    ~SbIBK_UNIX4OP_SAVE() = default;
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int     formatCaps() override;
    QString formatName() override;
    QString formatExtensionMask() override;
    BankFormats formatId() override;
};

#endif // SBIBK_H
