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

#include "../common.h"

#include "adlibbnk.h"
#include "apogeetmb.h"
#include "dmxopl2.h"
#include "junlevizion.h"
#include "milesopl.h"
#include "sb_ibk.h"

const char *openFilters[]
{
    "JunleVision bank (*.op3)",
    "DMX OPL-2 bank (*.op2 *.htc *.hxn)",
    "Apogee Sound System timbre bank (*.tmb)",
    "Sound Blaster IBK file (*.ibk)",
    "AdLib/HMI instrument Bank (*.bnk)",
    "",
    "Audio Interface Library (Miles) bank (*.opl *.ad)"
};

const char *saveFilters[]
{
    openFilters[0],
    openFilters[1],
    openFilters[2],
    openFilters[3],
    "AdLib instrument bank (*.bnk)",
    "HMI instrument bank (*.bnk)",
    "Audio Interface Library (Miles) bank (*.opl *.ad)",
};

#include "ffmt_base.h"


QString FmBankFormatBase::getSaveFiltersList()
{
    return  QString()
            +  saveFilters[FORMAT_JUNGLEVIZION] + ";;" +
            +  saveFilters[FORMAT_DMX_OP2]     + ";;" +
            +  saveFilters[FORMAT_APOGEE]      + ";;" +
            +  saveFilters[FORMAT_IBK]         + ";;" +
            +  saveFilters[FORMAT_ADLIB_BKN1]  + ";;" +
            +  saveFilters[FORMAT_ADLIB_BKNHMI] + ";;" +
            +  saveFilters[FORMAT_MILES];
}

QString FmBankFormatBase::getOpenFiltersList()
{
    return  QString("Supported bank files (*.op3 *.op2  *.htc *.hxn *.tmb *.ibk *.bnk *.opl *.ad);;") +
            +  openFilters[FORMAT_JUNGLEVIZION] + ";;" +
            +  openFilters[FORMAT_DMX_OP2]      + ";;" +
            +  openFilters[FORMAT_APOGEE]       + ";;" +
            +  openFilters[FORMAT_IBK]          + ";;" +
            +  openFilters[FORMAT_ADLIB_BKN1]   + ";;" +
            +  openFilters[FORMAT_MILES] + ";;" +
            +  "All files (*.*)";
}

FmBankFormatBase::Formats FmBankFormatBase::getFormatFromFilter(QString filter)
{
    for(int i = (int)FORMATS_BEGIN; i < (int)FORMATS_END; i++)
    {
        if(filter == saveFilters[i])
            return (Formats)i;
    }
    return FORMAT_UNKNOWN;
}

QString FmBankFormatBase::getFilterFromFormat(FmBankFormatBase::Formats format)
{
    if(format >= FORMATS_END)
        return "UNKNOWN";
    if(format < FORMATS_BEGIN)
        return "UNKNOWN";

    return saveFilters[format];
}

int FmBankFormatBase::OpenBankFile(QString filePath, FmBank &bank, Formats *recent)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    Formats fmt = FORMAT_UNKNOWN;

    //Check out for Junglevision file format
    if(JunleVizion::detect(magic))
    {
        err = JunleVizion::loadFile(filePath, bank);
        fmt = FORMAT_JUNGLEVIZION;
    }

    //Check for DMX OPL2 file format
    else if(DmxOPL2::detect(magic))
    {
        err = DmxOPL2::loadFile(filePath, bank);
        fmt = FORMAT_DMX_OP2;
    }

    //Check for Sound Blaster IBK file format
    else if(SbIBK::detect(magic))
    {
        err = SbIBK::loadFile(filePath, bank);
        fmt = FORMAT_IBK;
    }

    //Check for AdLib BNK file format
    else if(AdLibBnk::detect(magic))
        err = AdLibBnk::loadFile(filePath, bank, fmt);

    //Check for Apogee Sound System TMB file format
    else if(ApogeeTMB::detect(filePath))
    {
        err = ApogeeTMB::loadFile(filePath, bank);
        fmt = FORMAT_APOGEE;
    }

    //Check for Miles Sound System TMB file format
    else if(MilesOPL::detect(filePath))
    {
        err = MilesOPL::loadFile(filePath, bank);
        fmt = FORMAT_MILES;
    }

    if(recent)
        *recent = fmt;

    return err;
}

int FmBankFormatBase::OpenInstrumentFile(QString filePath, FmBank::Instrument &ins, FmBankFormatBase::InsFormats *recent, bool *isDrum)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    InsFormats fmt = FORMAT_INST_UNKNOWN;

    if(SbIBK::detectInst(magic))
    {
        err = SbIBK::loadFileInst(filePath, ins, isDrum);
        fmt = FORMAT_INST_SBI;
    }

    if(recent)
        *recent = fmt;

    return err;
}
