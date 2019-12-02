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

#include "ffmt_enums.h"
#include <QCoreApplication>
#include <QString>

QString FileFormats::getErrorText(FfmtErrCode err)
{
    QString errText;
    switch(err)
    {
    case FfmtErrCode::ERR_BADFORMAT:
        errText = QCoreApplication::translate(
            "FileFormats", "bad file format");
        break;
    case FfmtErrCode::ERR_NOFILE:
        errText = QCoreApplication::translate(
            "FileFormats", "can't open file");
        break;
    case FfmtErrCode::ERR_NOT_IMPLEMENTED:
        errText = QCoreApplication::translate(
            "FileFormats", "reading of this format is not implemented yet");
        break;
    case FfmtErrCode::ERR_UNSUPPORTED_FORMAT:
        errText = QCoreApplication::translate(
            "FileFormats", "unsupported file format");
        break;
    case FfmtErrCode::ERR_UNKNOWN:
        errText = QCoreApplication::translate(
            "FileFormats", "unknown error occurred");
        break;
    case FfmtErrCode::ERR_OK:
        break;
    }
    return errText;
}
