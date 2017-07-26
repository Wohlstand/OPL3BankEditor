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
    Can be built by Borland C, OpenWattcom, or DGJPP
 */

#include <stdio.h>
#include <bios.h>
#include <dos.h>
#include <string.h>

#define BIOStimer   biostime(0, 0l);

#define BIOSTICK    55			/* biostime() increases one tick about
					               every 55 msec */

typedef unsigned char   uint8_t;
typedef char            int8_t;
typedef unsigned short  uint16_t;
typedef short           int16_t;
typedef unsigned long   uint32_t;
typedef long            int32_t;

static const uint16_t   NewTimerFreq = 209;

static const uint16_t   MaxCards = 1;
static const uint16_t   OPLBase = 0x388;

static uint32_t         BIOStimer_begin = 0;

void mch_delay(int32_t msec)
{
	/*
	 * We busy-wait here.  Unfortunately, delay() and usleep() have been
	 * reported to give problems with the original Windows 95.  This is
	 * fixed in service pack 1, but not everybody installed that.
	 */
	long starttime = biostime(0, 0L);
	while(biostime(0, 0L) < starttime + msec / BIOSTICK);
}

void chipInit(void)
{
    unsigned TimerPeriod = 0x1234DDul / NewTimerFreq;
    BIOStimer_begin = BIOStimer;
    /*disable();*/
    outportb(0x43, 0x34);
    outportb(0x40, TimerPeriod & 0xFF);
    outportb(0x40, TimerPeriod >>   8);
    /*enable();*/
}

void chipPoke(uint16_t index, uint16_t value)
{
    uint16_t c;
    uint16_t o = index >> 8;
    uint16_t port = OPLBase + o * 2;
    outportb(port, index);
    for(c = 0; c < 6; ++c)  inportb(port);
    outportb(port + 1, value);
    for(c = 0; c < 35; ++c) inportb(port);
}

/* On Quit */
void chipUnInit(void)
{
    /* disable(); */
    outportb(0x43, 0x34);
    outportb(0x40, 0);
    outportb(0x40, 0);
    /* enable(); */
}

int main()
{
    char cmd[4] = "XXX";
    uint16_t reg = 0, value = 0;

    chipInit(); /* Initialize OPL3 chip */

    while(1)
    {
        scanf("%3s %X %X", cmd, &reg, &value);
        if(strcmp(cmd, "EX") == 0)
            break;
        if(strcmp(cmd, "REG") == 0)
            chipPoke(reg, value);
        if(strcmp(cmd, "OUT") == 0)
            printf("Received command %s with register %02X and value %02X\n", cmd, reg, value);
    }

    chipUnInit(); /* DeInitialize OPL3 chip */

    return 0;
}

