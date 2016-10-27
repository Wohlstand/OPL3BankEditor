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

#ifndef SBIBK_H
#define SBIBK_H

#include "ffmt_base.h"

/**
 * @brief Reader and Writer of the Sound Blaster IBK Bank format
 */
class SbIBK : public FmBankFormatBase
{
public:
    static bool detect(char* magic);
    static bool detectInst(char* magic);
    static bool detectSB2OP(QString filePath);
    static bool detectSB4OP(QString filePath);
    static int  loadFile(QString filePath, FmBank &bank);
    static int  loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0);
    static int  saveFile(QString filePath, FmBank &bank);
};

#endif // SBIBK_H
