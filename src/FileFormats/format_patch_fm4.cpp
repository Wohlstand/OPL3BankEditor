/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2026 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_patch_fm4.h"
#include "../common.h"
#include <cstring>
#include <QtDebug>

#define RIFF_MAGIC        "RIFF"
#define RIFF_PATCH_MAGIC  "Ptch"
#define RIFF_FM4_MAGIC    "fm4 "

// 4op instrument
#define FLAG_1_4OP      0
// Pseudo-4op instrument
#define FLAG_2_2OP      1
// 2op instrument
#define FLAG_1_2OP      2

// How many instruments in the bank
#define TOTAL_INSTRUMENTS       256
#define INSTRUMENT_SIZE         28

struct Fm4Op
{
    uint8_t reg20;
    uint8_t reg40;
    uint8_t reg60;
    uint8_t reg80;
    uint8_t regE0;
};

struct Fm4Inst
{
    Fm4Op op[4]; // 5 x 4 = 20
    uint8_t regA0[2];
    uint8_t regB0[2];
    uint8_t regC0[2];
    uint8_t opType;
    uint8_t reserved;
}; // 28 bytes


bool PatchFm4::detect(const QString& filePath, char* magic)
{
    uint32_t size = toUint32LE(reinterpret_cast<uint8_t*>(magic) + 4);

    if(std::memcmp(magic, RIFF_MAGIC, 4) != 0)
        return false;

    if(std::memcmp(magic + 8, RIFF_PATCH_MAGIC, 4) != 0)
        return false;

    if(std::memcmp(magic + 12, RIFF_FM4_MAGIC, 4) != 0)
        return false;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return false;

    qint64 fileSize = file.bytesAvailable();
    file.close();

    return (fileSize == (qint64)(size + 8));
}

FfmtErrCode PatchFm4::loadFile(QString filePath, FmBank& bank)
{
    QFile file(filePath);
    uint8_t magic[20];
    Fm4Inst inst;
    uint8_t block;

    static_assert(sizeof(Fm4Inst) == INSTRUMENT_SIZE, "Size of Fm4Inst must be 28 bytes !");

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    qint64 fileSize = file.bytesAvailable();

    bank.reset();

    bank.deep_tremolo = true;
    bank.deep_vibrato = true;
    bank.volume_model = FmBank::VOLUME_9X_GENERIC_FM;

    if(file.read(char_p(magic), 20) != 20)
        return FfmtErrCode::ERR_BADFORMAT;

    uint32_t sizeRiff = toUint32LE(reinterpret_cast<uint8_t*>(magic) + 4);
    uint32_t sizeData = toUint32LE(reinterpret_cast<uint8_t*>(magic) + 16);

    if(std::memcmp(magic, RIFF_MAGIC, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    if(std::memcmp(magic + 8, RIFF_PATCH_MAGIC, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    if(std::memcmp(magic + 12, RIFF_FM4_MAGIC, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    if((qint64)sizeRiff + 8 != fileSize)
        return FfmtErrCode::ERR_BADFORMAT;

    if((qint64)sizeData + 20 != fileSize)
        return FfmtErrCode::ERR_BADFORMAT;

    if(sizeData != (INSTRUMENT_SIZE * TOTAL_INSTRUMENTS))
        return FfmtErrCode::ERR_BADFORMAT;

    for(uint16_t i = 0; i < 256; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                    bank.Ins_Melodic[i] :
                                    bank.Ins_Percussion[(i - 128)];

        if(file.read(char_p(&inst), INSTRUMENT_SIZE) != INSTRUMENT_SIZE)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        const int opsMap[] =
        {
            MODULATOR1,
            CARRIER1,
            MODULATOR2,
            CARRIER2,
        };

        for(int op = 0; op < 4; ++op)
        {
            ins.setAVEKM(op, inst.op[opsMap[op]].reg20);
            ins.setKSLL(op, inst.op[opsMap[op]].reg40);
            ins.setAtDec(op, inst.op[opsMap[op]].reg60);
            ins.setSusRel(op, inst.op[opsMap[op]].reg80);
            ins.setWaveForm(op, inst.op[opsMap[op]].regE0);
        }

        ins.setFBConn1(inst.regC0[0]);
        ins.setFBConn2(inst.regC0[1]);
        ins.en_4op = (inst.opType == FLAG_1_4OP || inst.opType == FLAG_2_2OP);
        ins.en_pseudo4op = (inst.opType == FLAG_2_2OP);

        block = (inst.regB0[0] >> 2) & 0x07;
        ins.note_offset1 = 12 * (static_cast<int16_t>(block) - 4);
        block = (inst.regB0[1] >> 2) & 0x07;
        ins.note_offset2 = 12 * (static_cast<int16_t>(block) - 4);

        if(i >= 128)
            ins.percNoteNum = i - 128;
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode PatchFm4::saveFile(QString filePath, FmBank& bank)
{
    QFile file(filePath);
    Fm4Inst inst;
    int16_t noteOff1, noteOff2;
    uint32_t sizeRiff = 12;
    uint32_t sizeData = 0;

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    /* Temporary bank to prevent crash if current bank has less than 128 instruments
     * (for example, imported from some small BNK file) */
    TmpBank tmp(bank, 128, 128);

    if(file.write(RIFF_MAGIC, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(writeLE(file, sizeRiff) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.write(RIFF_PATCH_MAGIC, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(file.write(RIFF_FM4_MAGIC, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(writeLE(file, sizeData) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    for(uint16_t i = 0; i < 256; i++)
    {
        FmBank::Instrument &ins = (i < 128) ?
                                    tmp.insMelodic[i] :
                                    tmp.insPercussion[(i - 128)];

        std::memset(&inst, 0, sizeof(Fm4Inst));

        const int opsMap[] =
        {
            MODULATOR1,
            CARRIER1,
            MODULATOR2,
            CARRIER2,
        };

        for(int op = 0; op < 4; ++op)
        {
            inst.op[opsMap[op]].reg20 = ins.getAVEKM(op);
            inst.op[opsMap[op]].reg40 = ins.getKSLL(op);
            inst.op[opsMap[op]].reg60 = ins.getAtDec(op);
            inst.op[opsMap[op]].reg80 = ins.getSusRel(op);
            inst.op[opsMap[op]].regE0 = ins.getWaveForm(op);
        }

        inst.regC0[0] = ins.getFBConn1();
        inst.regC0[1] = ins.getFBConn2();

        if(ins.en_4op && ins.en_pseudo4op)
            inst.opType = FLAG_2_2OP;
        else if(ins.en_4op && !ins.en_pseudo4op)
            inst.opType = FLAG_1_4OP;
        else
            inst.opType = FLAG_1_2OP;

        noteOff1 = ins.note_offset1;
        noteOff2 = ins.note_offset2;

        if(i >= 128) // Align values to ensure the note number equal to the note itself
        {
            int16_t diff = (static_cast<int16_t>(i) - 128) - static_cast<int16_t>(ins.percNoteNum);
            noteOff1 -= diff;
            noteOff2 -= diff;
        }

        noteOff1 /= 12;
        noteOff2 /= 12;

        noteOff1 += 4;
        noteOff2 += 4;

        if(noteOff1 > 7)
            noteOff1 = 7;
        else if(noteOff1 < 0)
            noteOff1 = 0;

        if(noteOff2 > 7)
            noteOff2 = 7;
        else if(noteOff2 < 0)
            noteOff2 = 0;

        inst.regB0[0] = (noteOff1 & 0x07) << 2;
        inst.regB0[1] = (noteOff2 & 0x07) << 2;

        if(file.write(char_p(&inst), INSTRUMENT_SIZE) != INSTRUMENT_SIZE)
            return FfmtErrCode::ERR_BADFORMAT;

        sizeRiff += INSTRUMENT_SIZE;
        sizeData += INSTRUMENT_SIZE;
    }

    // Update RIFF and Data sizes
    file.seek(4);
    if(writeLE(file, sizeRiff) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    file.seek(16);
    if(writeLE(file, sizeData) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    file.close();
    return FfmtErrCode::ERR_OK;
}

int PatchFm4::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING_GM;
}

QString PatchFm4::formatName() const
{
    return "RIFF Patch FM4 (WinNT4 bank)";
}

QString PatchFm4::formatExtensionMask() const
{
    return "*.pat";
}

QString PatchFm4::formatDefaultExtension() const
{
    return "pat";
}

BankFormats PatchFm4::formatId() const
{
    return BankFormats::FORMAT_PATCH_FM4;
}
