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

#ifndef SBIBK_H
#define SBIBK_H

#include "ffmt_base.h"

class SbIBK_DOS : public FmBankFormatBase
{
public:
    SbIBK_DOS();
    ~SbIBK_DOS() = default;

    bool detect(const QString &filePath, char* magic);
    int  loadFile(QString filePath, FmBank &bank);
    int  saveFile(QString filePath, FmBank &bank);
    int  formatCaps();
    QString formatName();
    QString formatExtensionMask();
    Formats formatId();

    bool detectInst(const QString &filePath, char* magic);
    int  loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0);
    int         formatInstCaps();
    QString     formatInstName();
    QString     formatInstExtensionMask();
    InsFormats  formatInstId();
};

class SbIBK_UNIX_READ : public FmBankFormatBase
{
    Formats m_recentFormat = FORMAT_UNKNOWN;
public:
    SbIBK_UNIX_READ();
    ~SbIBK_UNIX_READ() = default;

    bool detect(const QString &filePath, char* magic);
    int  loadFile(QString filePath, FmBank &bank);
    int  formatCaps();
    QString formatName();
    QString formatExtensionMask();
    Formats formatId();
};


class SbIBK_UNIX2OP_SAVE : public FmBankFormatBase
{
public:
    SbIBK_UNIX2OP_SAVE();
    ~SbIBK_UNIX2OP_SAVE() = default;
    int  saveFile(QString filePath, FmBank &bank);
    int  formatCaps();
    QString formatName();
    QString formatExtensionMask();
    Formats formatId();
};


class SbIBK_UNIX4OP_SAVE : public FmBankFormatBase
{
public:
    SbIBK_UNIX4OP_SAVE();
    ~SbIBK_UNIX4OP_SAVE() = default;
    int  saveFile(QString filePath, FmBank &bank);
    int  formatCaps();
    QString formatName();
    QString formatExtensionMask();
    Formats formatId();
};




#endif // SBIBK_H
