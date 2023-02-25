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

#ifndef COMMON_H
#define COMMON_H

#include <QFile>

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
 * \param in Source reference
 * \return number of written bytes
 */
qint64 writeLE(QFile &file, const uint16_t &in);

/*!
 * \brief Write little-endian unsigned int into the file
 * \param file File descriptor
 * \param in Source reference
 * \return number of written bytes
 */
qint64 writeLE(QFile &file, const uint32_t &in);

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
 * \param in Source reference
 * \return number of written bytes
 */
qint64 writeBE(QFile &file, const uint16_t &in);

/*!
 * \brief Convers array of little endian bytes into short
 * \param arr Source array of bytes
 * \return Resulted number
 */
int16_t toSint16LE(const uint8_t *arr);

/*!
 * \brief Convers array of little endian bytes into short
 * \param arr Source array of bytes
 * \return Resulted number
 */
uint16_t toUint16LE(const uint8_t *arr);

/*!
 * \brief Convers array of big endian bytes into short
 * \param arr Source array of bytes
 * \return Resulted number
 */
uint16_t toUint16BE(const uint8_t *arr);

/*!
 * \brief Convers array of big endian bytes into short
 * \param arr Source array of bytes
 * \return Resulted number
 */
int16_t toSint16BE(const uint8_t *arr);

/*!
 * \brief Convers array of little endian bytes into int
 * \param arr Source array of bytes
 * \return Resulted number
 */
uint32_t toUint32LE(const uint8_t *arr);

/*!
 * \brief Convers array of big endian bytes into int
 * \param arr Source array of bytes
 * \return Resulted number
 */
uint32_t toUint32BE(const uint8_t *arr);


/*!
 * \brief Converts signed short into little endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromSint16LE(int16_t in, uint8_t *arr);

/*!
 * \brief Converts unsigned short into little endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromUint16LE(uint16_t in, uint8_t *arr);

/*!
 * \brief Converts unsigned short into big endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromUint16BE(uint16_t in, uint8_t *arr);

/*!
 * \brief Converts signed short into big endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromSint16BE(int16_t in, uint8_t *arr);

/*!
 * \brief Converts unsigned int into little endian byte array
 * \param [in] in Source number
 * \param [out] arr Target byte array
 */
void fromUint32LE(uint32_t in, uint8_t *arr);

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

/*!
 * \brief Clips the input value to minimum and maximum
 * \param i Input value
 * \param min Minimum value
 * \param max Maximum value
 * \return Final result
 */
uint8_t clip_u8(int i, int min, int max);

#if defined(IS_QT_4) || defined(QT_GUI_LIB)
void ErrMessageO(QWidget *parent, QString errStr, bool isBank = true);

void ErrMessageS(QWidget *parent, QString errStr, bool isBank = true);
#endif //QT_WIDGETS_LIB

#endif // COMMON_H
