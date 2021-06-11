/*
 * OPL3 chip interface proxy for OPL Bank Editor by Wohlstand,
 * a part of free tool for music bank editing
 *
 * Copyright (c) 2016-2021 Vitaly Novichkov <admin@wohlnet.ru>
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

/*
    Can be built by OpenWattcom
 */

#include <conio.h>

#define DLLExport       __declspec(dllexport)
#define STDCall         __stdcall
typedef unsigned short  uint16_t;

static const uint16_t   OPLBase = 0x388;

DLLExport void STDCall chipInit(void)
{ /* Dummy */ }

DLLExport void STDCall chipUnInit(void)
{ /* Dummy */ }

DLLExport void STDCall chipPoke(uint16_t index, uint16_t value)
{
    uint16_t c, o = (index >> 8), port = (OPLBase + o * 2);
    outp(port, index);
    for(c = 0; c < 6; ++c)  inp(port);
    outp(port + 1, value);
    for(c = 0; c < 35; ++c) inp(port);
}

