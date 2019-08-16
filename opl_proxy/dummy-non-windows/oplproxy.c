/*
 * OPL3 chip interface proxy for OPL Bank Editor by Wohlstand,
 * a part of free tool for music bank editing
 *
 * Copyright (c) 2016-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifdef _WIN32
#   define DLLExport       __declspec(dllexport)
#   define STDCall         __stdcall
#else
#   define DLLExport
#   define STDCall
#endif
typedef unsigned short  uint16_t;

static uint16_t   OPLBase = 0x388;

DLLExport uint16_t STDCall chipType()
{
    /*
    - 0 is OPL3
    - 1 is OPL2
    */
    return 0;
}

DLLExport void STDCall chipInit(void)
{ /* Dummy */ }

DLLExport void STDCall chipUnInit(void)
{ /* Dummy */ }

DLLExport void STDCall chipPoke(uint16_t index, uint16_t value)
{
    /* Dummy */
}

DLLExport void STDCall chipSetPort(uint16_t port)
{
    OPLBase = port;
}

