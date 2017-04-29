/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

class AdLibBnk_read : public FmBankFormatBase
{
    Formats m_recentFormat = FORMAT_UNKNOWN;
public:
    AdLibBnk_read();
    ~AdLibBnk_read() = default;

    bool detect(const QString &filePath, char* magic);
    int  loadFile(QString filePath, FmBank &bank);
    int  formatCaps();
    QString formatName();
    QString formatExtensionMask();
    Formats formatId();
};

class AdLibBnk_save : public FmBankFormatBase
{
public:
    AdLibBnk_save();
    ~AdLibBnk_save() = default;
    int  saveFile(QString filePath, FmBank &bank);
    int  formatCaps();
    QString formatName();
    QString formatExtensionMask();
    Formats formatId();
};

class HmiBnk_save : public FmBankFormatBase
{
public:
    HmiBnk_save();
    ~HmiBnk_save() = default;
    int  saveFile(QString filePath, FmBank &bank);
    int  formatCaps();
    QString formatName();
    QString formatExtensionMask();
    Formats formatId();
};

#endif // ADLIBBNK_H
