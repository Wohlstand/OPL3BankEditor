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

#ifndef COMMON_H
#define COMMON_H

#include <QFile>

//! Typedef to unsigned char
typedef unsigned char uchar;

//! Typedef to signed character pointer
typedef char*         char_p;

/*!
 * \brief Read little-endian unsigned short from a file
 * \param file File descriptor
 * \param out Target reference
 * \return number of readed bytes
 */
qint64 readLE(QFile &file, uint16_t &out);

/*!
 * \brief Read little-endian unsigned int from a file
 * \param file File descriptor
 * \param out Target reference
 * \return number of readed bytes
 */
qint64 readLE(QFile &file, uint32_t &out);

/*!
 * \brief Write little-endian unsigned short into the file
 * \param file File descriptor
 * \param out Source reference
 * \return number of written bytes
 */
qint64 writeLE(QFile &file, uint16_t &out);

/*!
 * \brief Write little-endian unsigned int into the file
 * \param file File descriptor
 * \param out Source reference
 * \return number of written bytes
 */
qint64 writeLE(QFile &file, uint32_t &out);

/*!
 * \brief Read big-endian unsigned short from a file
 * \param file File descriptor
 * \param out Target reference
 * \return number of readed bytes
 */
qint64 readBE(QFile &file, uint16_t &out);

/*!
 * \brief Write big-endian unsigned short into the file
 * \param file File descriptor
 * \param out Source reference
 * \return number of written bytes
 */
qint64 writeBE(QFile &file, uint16_t &out);

/*!
 * \brief Convers array of little endian bytes into short
 * \param arr Source array of bytes
 * \return Resulted number
 */
int16_t toSint16LE(uchar *arr);

/*!
 * \brief Convers array of little endian bytes into short
 * \param arr Source array of bytes
 * \return Resulted number
 */
uint16_t toUint16LE(uchar *arr);

/*!
 * \brief Convers array of big endian bytes into short
 * \param arr Source array of bytes
 * \return Resulted number
 */
int16_t toSint16BE(uchar *arr);

/*!
 * \brief Convers array of big endian bytes into int
 * \param arr Source array of bytes
 * \return Resulted number
 */
uint32_t toUint32LE(uchar *arr);


/*!
 * \brief Converts signed short into little endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromSint16LE(int16_t in, uchar *arr);

/*!
 * \brief Converts unsigned short into little endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromUint16LE(uint16_t in, uchar *arr);

/*!
 * \brief Converts signed short into big endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromSint16BE(int16_t in, uchar *arr);

/*!
 * \brief Converts unsigned int into little endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromUint32LE(int32_t in, uchar *arr);

/*!
 * \brief Reads some bytes from begin of the file
 * \param [in] filePath Path to the file
 * \param [out] bytes target array pointer
 * \param [in] count How much needed bytes to be readed
 */
void getMagic(QString filePath, char* bytes, int count);

/*!
 * \brief Checks, has filename an extension
 * \param file File path
 * \param ext File extension
 * \return true if extension has been detected
 */
bool hasExt(const QString &file, const char*ext);

void ErrMessageO(QWidget *parent, QString errStr);

void ErrMessageS(QWidget *parent, QString errStr);

#endif // COMMON_H
