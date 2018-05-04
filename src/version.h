/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef VERSION_H
#define VERSION_H

#define COMPANY "WohlSoft"

#define PGE_URL "wohlsoft.ru"

#define PROGRAM_NAME "OPL3 Bank Editor"

#define VERSION "1.4.0.2"

#ifdef IS_QT_4
#define COPYRIGHT_SIGN "(C)"
#else
#define COPYRIGHT_SIGN "©"
#endif

#define COPYRIGHT COPYRIGHT_SIGN " 2016-2018, Vitaly Novichkov \"Wohlstand\""

#endif // VERSION_H
