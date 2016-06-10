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

#include "bank.h"
#include <memory.h>

FmBank::FmBank()
{
    reset();
}

FmBank::FmBank(const FmBank &fb)
{
    int size = sizeof(Instrument)*128;
    memcpy(Ins_Melodic,    fb.Ins_Melodic,    size);
    memcpy(Ins_Percussion, fb.Ins_Percussion, size);
}

bool FmBank::operator==(const FmBank &fb)
{
    int size = sizeof(Instrument)*128;
    bool res = true;
    res &= (memcmp(Ins_Melodic,    fb.Ins_Melodic,    size)==0);
    res &= (memcmp(Ins_Percussion, fb.Ins_Percussion, size)==0);
    return res;
}

bool FmBank::operator!=(const FmBank &fb)
{
    return !this->operator==(fb);
}

void FmBank::reset()
{
    int size=sizeof(Instrument)*128;
    memset(Ins_Melodic,    0, size);
    memset(Ins_Percussion, 0, size);
}

