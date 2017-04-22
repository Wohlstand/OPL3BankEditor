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
    static bool detectIBK(char* magic);
    static bool detectSBI(char* magic);
    static bool detectUNIXO2(QString filePath);
    static bool detectUNIXO3(QString filePath);
    // IBK/SBI for DOS
    static int  loadFile(QString filePath, FmBank &bank);
    static int  loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0);
    static int  saveFile(QString filePath, FmBank &bank);
    // SB/O3 for UNIX
    static int  loadFileSBOP(QString filePath, FmBank &bank);
    static int  saveFileSBOP(QString filePath, FmBank &bank);
};

#endif // SBIBK_H
