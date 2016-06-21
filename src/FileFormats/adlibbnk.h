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

#ifndef ADLIBBNK_H
#define ADLIBBNK_H

#include "ffmt_base.h"

/**
 * @brief Reader and Writer of the Apogee Sound System TMB Bank format
 */
class AdLibBnk : public FmBankFormatBase
{
public:
    enum BnkType
    {
        BNK_ADLIB,
        BNK_HMI
    };
    static bool detect(char* magic);
    static int  loadFile(QString filePath, FmBank &bank, Formats &format);
    static int  saveFile(QString filePath, FmBank &bank, BnkType type);
};

#endif // ADLIBBNK_H
