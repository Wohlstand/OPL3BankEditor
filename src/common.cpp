/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

#if defined(IS_QT_4) || defined(QT_GUI_LIB)
#include <QMessageBox>
#endif

qint64 readLE(QFile &file, uint16_t &out)
{
    uint8_t bytes[2] = {0, 0};
    qint64 len = file.read(char_p(bytes), 2);
    out = (quint16(bytes[0]) & 0x00FF)
          | ((quint16(bytes[1]) << 8) & 0xFF00);
    return len;
}


qint64 readLE(QFile &file, uint32_t &out)
{
    uint8_t bytes[4] = {0, 0, 0, 0};
    qint64 len = file.read(char_p(bytes), 4);
    out =    (quint32(bytes[0]  << 0) & 0x000000FF)
          | ((quint32(bytes[1]) << 8)  & 0x0000FF00)
          | ((quint32(bytes[2]) << 16) & 0x00FF0000)
          | ((quint32(bytes[3]) << 24) & 0xFF000000);
    return len;
}

qint64 writeLE(QFile &file, const uint16_t &in)
{
    uint8_t bytes[2] = {uint8_t(in & 0x00FF), uint8_t((in >> 8) & 0x00FF) };
    qint64 len = file.write(char_p(bytes), 2);
    return len;
}


qint64 writeLE(QFile &file, const uint32_t &in)
{
    uint8_t bytes[4] = { uint8_t(in & 0x000000FF),
                         uint8_t((in >> 8) & 0x000000FF),
                         uint8_t((in >> 16) & 0x000000FF),
                         uint8_t((in >> 24) & 0x000000FF)
                       };
    qint64 len = file.write(char_p(bytes), 4);
    return len;
}


qint64 readBE(QFile &file, uint16_t &out)
{
    uint8_t bytes[2] = {0, 0};
    qint64 len = file.read(char_p(bytes), 2);
    out = (quint16(bytes[1]) & 0x00FF)
          | ((quint16(bytes[0]) << 8) & 0xFF00);
    return len;
}

qint64 writeBE(QFile &file, const uint16_t &in)
{
    uint8_t bytes[2] = {uint8_t((in >> 8) & 0x00FF), uint8_t(in & 0x00FF)};
    qint64 len = file.write(char_p(bytes), 2);
    return len;
}

int16_t toSint16LE(const uint8_t *arr)
{
    int16_t num = *reinterpret_cast<const int8_t *>(&arr[1]);
    num *= 1 << 8;
    num |= arr[0];
    return num;
}

uint16_t toUint16LE(const uint8_t *arr)
{
    uint16_t num = arr[0];
    num |= ((arr[1] << 8) & 0xFF00);
    return num;
}

uint16_t toUint16BE(const uint8_t *arr)
{
    uint16_t num = arr[1];
    num |= ((arr[0] << 8) & 0xFF00);
    return num;
}


int16_t toSint16BE(const uint8_t *arr)
{
    int16_t num = *reinterpret_cast<const int8_t *>(&arr[0]);
    num *= 1 << 8;
    num |= arr[1];
    return num;
}

uint32_t toUint32LE(const uint8_t *arr)
{
    uint32_t num = arr[0];
    num |= (static_cast<uint32_t>(arr[1] << 8)  & 0x0000FF00);
    num |= (static_cast<uint32_t>(arr[2] << 16) & 0x00FF0000);
    num |= (static_cast<uint32_t>(arr[3] << 24) & 0xFF000000);
    return num;
}

uint32_t toUint32BE(const uint8_t *arr)
{
    uint32_t num = arr[3];
    num |= (static_cast<uint32_t>(arr[2] << 8)  & 0x0000FF00);
    num |= (static_cast<uint32_t>(arr[1] << 16) & 0x00FF0000);
    num |= (static_cast<uint32_t>(arr[0] << 24) & 0xFF000000);
    return num;
}


void fromSint16LE(int16_t in, uint8_t *arr)
{
    arr[0] =  in & 0x00FF;
    arr[1] = (in >> 8) & 0x00FF;
}

void fromUint16LE(uint16_t in, uint8_t *arr)
{
    arr[0] =  in & 0x00FF;
    arr[1] = (in >> 8) & 0x00FF;
}

void fromUint16BE(uint16_t in, uint8_t *arr)
{
    arr[1] =  in & 0x00FF;
    arr[0] = (in >> 8) & 0x00FF;
}

void fromSint16BE(int16_t in, uint8_t *arr)
{
    arr[1] =  in & 0x00FF;
    arr[0] = (in >> 8) & 0x00FF;
}

void fromUint32LE(uint32_t in, uint8_t *arr)
{
    arr[0] = (in)     & 0x00FF;
    arr[1] = (in >> 8)  & 0x00FF;
    arr[2] = (in >> 16) & 0x00FF;
    arr[3] = (in >> 24) & 0x00FF;
}


void getMagic(QString filePath, char *bytes, int count)
{
    QFile file(filePath);
    memset(bytes, 0, static_cast<size_t>(count));

    if(file.open(QIODevice::ReadOnly))
    {
        file.read(bytes, count);
        file.close();
    }
}

bool hasExt(const QString &file, const char *ext)
{
    return file.endsWith(ext, Qt::CaseInsensitive);
}

uint8_t clip_u8(int i, int min, int max)
{
    if(i < min)
        return min;
    else if(i > max)
        return max;
    else
        return static_cast<uint8_t>(i);
}

#if defined(IS_QT_4) || defined(QT_GUI_LIB)
void ErrMessageO(QWidget *parent, QString errStr, bool isBank)
{
    QString ftype = isBank ? QObject::tr("bank") : QObject::tr("instrument");
    QMessageBox::warning(parent,
                         QObject::tr("Can't open %1 file!").arg(ftype),
                         QObject::tr("Can't open %1 file because %2.").arg(ftype).arg(errStr),
                         QMessageBox::Ok);
}

void ErrMessageS(QWidget *parent, QString errStr, bool isBank)
{
    QString ftype = isBank ? QObject::tr("bank") : QObject::tr("instrument");
    QMessageBox::warning(parent,
                         QObject::tr("Can't save %1 file!").arg(ftype),
                         QObject::tr("Can't save %1 file because %2.").arg(ftype).arg(errStr),
                         QMessageBox::Ok);
}
#endif

