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

#ifndef MILESOPL_K
#define MILESOPL_K

#include "ffmt_base.h"

/**
 * @brief Reader and Writer of the Miles Sound System TMB Bank format
 */
class MilesOPL : public FmBankFormatBase
{
public:
    static bool detect(QString filePath);
    static int  loadFile(QString filePath, FmBank &bank);
    static int  saveFile(QString filePath, FmBank &bank);
};

#endif // MILESOPL_K
