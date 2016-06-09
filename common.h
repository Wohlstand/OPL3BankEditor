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
qint64 readLE(QFile &file, unsigned short &out);

/*!
 * \brief Write little-endian unsigned short into the file
 * \param file File descriptor
 * \param out Source reference
 * \return number of written bytes
 */
qint64 writeLE(QFile &file, unsigned short &out);

/*!
 * \brief Read big-endian unsigned short from a file
 * \param file File descriptor
 * \param out Target reference
 * \return number of readed bytes
 */
qint64 readBE(QFile &file, unsigned short &out);

/*!
 * \brief Write big-endian unsigned short into the file
 * \param file File descriptor
 * \param out Source reference
 * \return number of written bytes
 */
qint64 writeBE(QFile &file, unsigned short &out);

#endif // COMMON_H
