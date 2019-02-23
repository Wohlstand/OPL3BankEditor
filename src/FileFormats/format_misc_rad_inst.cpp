/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_misc_rad_inst.h"
#include "../common.h"

static const char *rad_inst_magic = "RAD INST V10";

bool Misc_RealityAdLib::detectInst(const QString &, char *magic)
{
    return (memcmp(magic, rad_inst_magic, 12) == 0);
}

FfmtErrCode Misc_RealityAdLib::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    memset(&inst, 0, sizeof(FmBank::Instrument));
    QFile file(filePath);
    uint8_t idata[300];
    qint64  idata_len = 0;
    size_t  idata_offset = 0;
    char    title[257] = {0};
    uint8_t title_length = 0;
    memset(&idata, 0, 300);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    idata_len = file.read(char_p(idata), 300);
    if(idata_len < 0)
        return FfmtErrCode::ERR_NOFILE;
    if(memcmp(idata, rad_inst_magic, 12) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    // Skip magic
    idata_offset += 12;

    // Get the title size
    title_length = idata[idata_offset++];
    // Copy titile data
    memcpy(title, &idata[idata_offset], title_length);
    // Put the title string into bank data
    title[title_length] = '\0';
    strncpy(inst.name, title, 32);

    // Jump to instrument data
    idata_offset += title_length;




    return FfmtErrCode::ERR_NOT_IMLEMENTED;
}

FfmtErrCode Misc_RealityAdLib::saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum)
{
    return FfmtErrCode::ERR_NOT_IMLEMENTED;
}

int Misc_RealityAdLib::formatInstCaps() const
{
    return int(FormatCaps::FORMAT_CAPS_EVERYTHING);
}

QString Misc_RealityAdLib::formatInstName() const
{
    return "Reality AdLib Tracker instrument";
}

QString Misc_RealityAdLib::formatInstModuleName() const
{
    return "Reality AdLib Tracker instrument";
}

QString Misc_RealityAdLib::formatInstExtensionMask() const
{
    return "*.ins";
}

InstFormats Misc_RealityAdLib::formatInstId() const
{
    return InstFormats::FORMAT_INST_RAD;
}
