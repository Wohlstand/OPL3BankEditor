/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_cmf_importer.h"
#include "../common.h"

static const char *cmf_magic = "CTMF";

bool CMF_Importer::detect(const QString &, char *magic)
{
    return (strncmp(magic, cmf_magic, 4) == 0);
}

FfmtErrCode CMF_Importer::loadFile(QString filePath, FmBank &bank)
{
    char        magic[4];
    uint8_t     version[2];
    uint16_t    insOffset = 0;
    uint16_t    insCount = 0;
    uint8_t     insCount_a[2];
    FmBank::Instrument ins = FmBank::emptyInst();

    QFile file(filePath);
    memset(magic, 0, 4);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();

    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;
    if(strncmp(magic, cmf_magic, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;
    if(file.read(char_p(&version), 2) != 2)
        return FfmtErrCode::ERR_BADFORMAT;

    if((version[1] != 1) && ((version[0] != 1) || (version[0] != 0)))
        return FfmtErrCode::ERR_BADFORMAT;

    bank.Ins_Melodic_box.clear();
    bank.Ins_Percussion_box.clear();

    if(readLE(file, insOffset) != 2)
    {
        bank.reset();
        return FfmtErrCode::ERR_BADFORMAT;
    }

    if(!file.seek(36))//Go for instruments count
    {
        bank.reset();
        return FfmtErrCode::ERR_BADFORMAT;
    }

    if(file.read(char_p(insCount_a), 2) != 2)
    {
        bank.reset();
        return FfmtErrCode::ERR_BADFORMAT;
    }

    if(version[0] == 1)
        insCount = toUint16LE(insCount_a);
    else
        insCount = insCount_a[0];

    if(!file.seek(insOffset))//Go for instruments offset
    {
        bank.reset();
        return FfmtErrCode::ERR_BADFORMAT;
    }

    for(uint16_t i = 0; i < insCount; i++)
    {
        uint8_t idata[16];
        if(file.read(char_p(idata), 16) != 16)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
        ins.setAVEKM(MODULATOR1,    idata[0]);
        ins.setAVEKM(CARRIER1,      idata[1]);
        ins.setKSLL(MODULATOR1,     idata[2]);
        ins.setKSLL(CARRIER1,       idata[3]);
        ins.setAtDec(MODULATOR1,    idata[4]);
        ins.setAtDec(CARRIER1,      idata[5]);
        ins.setSusRel(MODULATOR1,   idata[6]);
        ins.setSusRel(CARRIER1,     idata[7]);
        ins.setWaveForm(MODULATOR1, idata[8]);
        ins.setWaveForm(CARRIER1,   idata[9]);
        ins.setFBConn1(idata[10]);
        snprintf(ins.name, 32, "CMF Instr. %03u", i);
        bank.Ins_Melodic_box.push_back(ins);
    }

    /*
        TODO:   Add analyzis of the MIDI data to recogonize
                which instruments are percussion (AdLib rythm mode)
    */

    file.close();
    bank.Ins_Percussion = bank.Ins_Percussion_box.data();
    bank.Ins_Melodic    = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}

int CMF_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString CMF_Importer::formatName() const
{
    return "Creative Music Format";
}

QString CMF_Importer::formatModuleName() const
{
    return "Creative Music importer";
}

QString CMF_Importer::formatExtensionMask() const
{
    return "*.cmf *.ddt";
}

BankFormats CMF_Importer::formatId() const
{
    return BankFormats::FORMAT_CMF_IMPORTER;
}
