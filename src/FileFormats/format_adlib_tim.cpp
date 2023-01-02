/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_adlib_tim.h"
#include "../common.h"


bool AdLibTimbre::detect(const QString &filePath, char *magic)
{
    if(hasExt(filePath, ".tim") || hasExt(filePath, ".snd"))
        return true;

    // Attempt to detect by file size and offset validation
    uint16_t instruments_count = 0;
    uint16_t instruments_offset = 0;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return false;

    uint64_t fileSize = uint64_t(file.bytesAvailable());
    file.close();

    uint8_t *head = reinterpret_cast<uint8_t*>(magic);
    if((head[0] != 1) || (head[1] != 0))
        return false;
    instruments_count  = toUint16LE(head + 2);
    instruments_offset = toUint16LE(head + 4);

    uint16_t minFileSize = (instruments_count * 9) + (instruments_count * 56) + 6;
    //Validate offste to instruments
    if(instruments_offset != (instruments_count * 9) + 6)
        return false;
    //Validate file size
    if(minFileSize < fileSize)
        return false;

    return true;
}

/**
 * @brief Parse operator data from INS file
 * @param inst Destinition instrument
 * @param opType Operator type (Modulator or carrier)
 * @param idata Input raw data
 * @return true if data valid, false if data contains broken or invalid data
 */
extern bool adlib_ins_insRawToOp(FmBank::Instrument &inst, const int opType, const uint8_t *idata);
//In the format_adlib_bnk

/**
 * @brief Generate raw INS data from instrument's operator
 * @param inst Source instrument
 * @param opType Operator type (Modulator or carrier)
 * @param odata Destinition output memory block to write
 */
extern void adlib_ins_opToRawIns(const FmBank::Instrument &inst, const int opType, uint8_t *odata);
//In the format_adlib_bnk

FfmtErrCode AdLibTimbre::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint64_t fileSize = uint64_t(file.bytesAvailable());

    bank.reset();

    bank.deep_tremolo = false;
    bank.deep_vibrato = false;
    bank.volume_model = FmBank::VOLUME_CMF;

    uint8_t head[6];
    uint16_t instruments_count = 0;
    uint16_t instruments_offset = 0;

    if(file.read(char_p(head), 6) != 6)
        return FfmtErrCode::ERR_BADFORMAT;

    if((head[0] != 1) || (head[1] != 0))
        return FfmtErrCode::ERR_BADFORMAT;

    instruments_count  = toUint16LE(head + 2);
    instruments_offset = toUint16LE(head + 4);

    uint16_t minFileSize = (instruments_count * 9) + (instruments_count * 56) + 6;
    //Validate offste to instruments
    if(instruments_offset != (instruments_count * 9) + 6)
        return FfmtErrCode::ERR_BADFORMAT;
    //Validate file size
    if(minFileSize < fileSize)
        return FfmtErrCode::ERR_BADFORMAT;

    std::string instrument_names_a;
    instrument_names_a.resize(instruments_count * 9);
    char *instrument_names = &instrument_names_a[0];
    char ins_name[9];
    for(uint16_t i = 0; i < instruments_count; i++)
    {
        if(file.read(ins_name, 9) != 9)
            return FfmtErrCode::ERR_BADFORMAT;
        strncpy(instrument_names + (i * 9), ins_name, 9);
    }

    if(!file.seek(instruments_offset))
        return FfmtErrCode::ERR_BADFORMAT;

    bank.Ins_Percussion_box.clear();
    bank.Ins_Percussion = bank.Ins_Percussion_box.data();

    bank.Ins_Melodic_box.resize(instruments_count);
    bank.Ins_Melodic = bank.Ins_Melodic_box.data();

    uint8_t idata[56];
    for(uint16_t i = 0; i < instruments_count; i++)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];
        if(file.read(char_p(idata), 56) != 56)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
        if(!adlib_ins_insRawToOp(ins, MODULATOR1, idata + 0))
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
        if(!adlib_ins_insRawToOp(ins, CARRIER1, idata + 26))
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
        ins.setWaveForm(MODULATOR1,    idata[52]);
        ins.setWaveForm(CARRIER1,      idata[54]);
        strncpy(ins.name, instrument_names + (9 * i), 8);
    }
    file.close();

    //Automatically create missing banks
    bank.autocreateMissingBanks();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode AdLibTimbre::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t head[6];
    memset(head, 0, 6);
    head[0] = 1;
    head[1] = 0;
    uint16_t ins_count = bank.Ins_Melodic_box.count() <= 65535 ? uint16_t(bank.Ins_Melodic_box.count()) : 65535;
    uint16_t ins_offset = (ins_count * 9 + 6);
    fromUint16LE(ins_count, head + 2);
    fromUint16LE(ins_offset, head + 4);

    if(file.write(char_p(head), 6) != 6)
        return FfmtErrCode::ERR_BADFORMAT;

    char ins_name[9];
    for(uint16_t i = 0; i < ins_count; i++)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];
        memset(ins_name, 0, 9);
        strncpy(ins_name, ins.name, 8);
        if(file.write(ins_name, 9) != 9)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    uint8_t odata[56];
    for(uint16_t i = 0; i < ins_count; i++)
    {
        FmBank::Instrument &ins = bank.Ins_Melodic[i];
        memset(odata, 0, 56);
        adlib_ins_opToRawIns(ins, MODULATOR1, odata + 0);
        adlib_ins_opToRawIns(ins, CARRIER1, odata + 26);
        odata[26 + 24] = (!ins.connection1) & 0x01;
        odata[52] = ins.getWaveForm(MODULATOR1);
        odata[54] = ins.getWaveForm(CARRIER1);
        if(file.write(char_p(odata), 56) != 56)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    file.close();

    return FfmtErrCode::ERR_OK;
}

int AdLibTimbre::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString AdLibTimbre::formatName() const
{
    return "Legacy AdLib Timbre Bank Format";
}

QString AdLibTimbre::formatModuleName() const
{
    return "AdLib Timbre Bank (1987-1988)";
}

QString AdLibTimbre::formatExtensionMask() const
{
    return "*.snd *.tim";
}

QString AdLibTimbre::formatDefaultExtension() const
{
    return "tim";
}

BankFormats AdLibTimbre::formatId() const
{
    return BankFormats::FORMAT_ADLIB_TIM;
}
