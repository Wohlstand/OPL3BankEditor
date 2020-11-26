/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_adlib_tracker.h"
#include "../common.h"
#include <QFileInfo>

#include "at_misc/adt2pack.hpp"


static const char *A2I_magic = "_A2ins_";

bool AdLibTracker::detectInst(const QString &filePath, char *magic)
{
    Q_UNUSED(filePath);
    return memcmp(magic, A2I_magic, 7) == 0;
}

FfmtErrCode AdLibTracker::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    Q_UNUSED(isDrum);
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    char magic[7];
    if(file.read((char *)magic, 7) != 7 || memcmp(magic, A2I_magic, 7) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    char check[2];
    char version;
    if(file.read(check, 2) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(file.read(&version, 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;

    if(version != 0x0A) // Support Format 10 only yet
        return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;


    return FfmtErrCode::ERR_NOT_IMPLEMENTED;
}

int AdLibTracker::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN |
           (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString AdLibTracker::formatInstName() const
{
    return "A2I instrument";
}

QString AdLibTracker::formatInstExtensionMask() const
{
    return "*.a2i";
}

InstFormats AdLibTracker::formatInstId() const
{
    return InstFormats::FORMAT_INST_A2I;
}
