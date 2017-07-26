/*
 * OPL3 chip interface proxy for OPL Bank Editor by Wohlstand,
 * a part of free tool for music bank editing
 *
 * Copyright (c) 2016-2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <windows.h>
#include <conio.h>
#include <stdio.h>
#include <bios.h>
#include <dos.h>
#include <string.h>

#define DLLExport __declspec( dllexport )
#define STDCall _stdcall
/*#define STDCall*/

typedef unsigned char   uint8_t;
typedef char            int8_t;
typedef unsigned short  uint16_t;
typedef short           int16_t;
typedef unsigned long   uint32_t;
typedef long            int32_t;

static const uint16_t   NewTimerFreq = 209;
static const uint16_t   OPLBase = 0x388;

DLLExport void STDCall chipInit(void)
{
    unsigned TimerPeriod = 0x1234DDul / NewTimerFreq;
    #ifndef _WIN32
    BIOStimer_begin = BIOStimer;
    #endif
    outp(0x43, 0x34);
    outp(0x40, TimerPeriod & 0xFF);
    outp(0x40, TimerPeriod >>   8);
}

DLLExport void STDCall chipPoke(uint16_t index, uint16_t value)
{
    uint16_t c;
    uint16_t o = index >> 8;
    uint16_t port = OPLBase + o * 2;
    outp(port, index);
    for(c = 0; c < 6; ++c)  inp(port);
    outp(port + 1, value);
    for(c = 0; c < 35; ++c) inp(port);
}

/* On Quit */
DLLExport void STDCall chipUnInit(void)
{
    outp(0x43, 0x34);
    outp(0x40, 0);
    outp(0x40, 0);
}
