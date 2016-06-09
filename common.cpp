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

#include "common.h"

qint64 readLE(QFile &file, unsigned short &out)
{
    uchar bytes[2] = {0, 0};
    qint64 len = file.read(char_p(bytes), 2);
    out =  (quint16(bytes[0])&0x00FF)
          |((quint16(bytes[1])<<8)&0xFF00);
    return len;
}

qint64 writeLE(QFile &file, unsigned short &out)
{
    uchar bytes[2] = {uchar(out&0x00FF), uchar((out>>8)&0x00FF) };
    qint64 len = file.write(char_p(bytes), 2);
    return len;
}

qint64 readBE(QFile &file, unsigned short &out)
{
    uchar bytes[2] = {0, 0};
    qint64 len = file.read(char_p(bytes), 2);
    out =  (quint16(bytes[1])&0x00FF)
         |((quint16(bytes[0])<<8)&0xFF00);
    return len;
}

qint64 writeBE(QFile &file, unsigned short &out)
{
    uchar bytes[2] = {uchar((out>>8)&0x00FF), uchar(out&0x00FF)};
    qint64 len = file.write(char_p(bytes), 2);
    return len;
}
