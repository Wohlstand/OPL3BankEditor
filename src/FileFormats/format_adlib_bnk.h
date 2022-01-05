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

#ifndef ADLIBBNK_H
#define ADLIBBNK_H

#include "ffmt_base.h"

class AdLibAndHmiBnk_reader final : public FmBankFormatBase
{
    BankFormats m_recentFormat = BankFormats::FORMAT_UNKNOWN;
public:
    bool detect(const QString &filePath, char* magic) override;
    FfmtErrCode  loadFile(QString filePath, FmBank &bank) override;
    int  formatCaps() const override;
    QString formatName() const override;
    QString formatModuleName() const override;
    QString formatExtensionMask() const override;
    BankFormats formatId() const override;

    bool        detectInst(const QString &filePath, char* magic) override;
    FfmtErrCode loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0) override;
    FfmtErrCode saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum = false) override;
    int         formatInstCaps() const override;
    QString     formatInstName() const override;
    QString     formatInstModuleName() const override;
    QString     formatInstExtensionMask() const override;
    QString     formatInstDefaultExtension() const override;
    InstFormats formatInstId() const override;
};

class AdLibBnk_writer final : public FmBankFormatBase
{
public:
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int  formatCaps() const override;
    QString formatName() const override;
    QString formatModuleName() const override;
    QString formatExtensionMask() const override;
    QString formatDefaultExtension() const override;
    BankFormats formatId() const override;
};

class HmiBnk_writer final : public FmBankFormatBase
{
public:
    FfmtErrCode  saveFile(QString filePath, FmBank &bank) override;
    int  formatCaps() const override;
    QString formatName() const override;
    QString formatModuleName() const override;
    QString formatExtensionMask() const override;
    QString formatDefaultExtension() const override;
    BankFormats formatId() const override;
};

class HmiBnk_Drums_writer final : public FmBankFormatBase
{
public:
    FfmtErrCode  saveFile(QString filePath, FmBank &bank) override;
    int  formatCaps() const override;
    QString formatName() const override;
    QString formatModuleName() const override;
    QString formatExtensionMask() const override;
    QString formatDefaultExtension() const override;
    BankFormats formatId() const override;
};

#endif // ADLIBBNK_H
