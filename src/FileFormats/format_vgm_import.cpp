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

#include "format_vgm_import.h"
#include "ymf262_to_wopi.h"
#include "../common.h"

#include <QSet>
#include <QByteArray>
#include <QBuffer>
#include <zlib.h>
#include <algorithm>
#include <cstring>

static void make_size_table(uint8_t *table, unsigned version);
static gzFile gzopen_q(const QString &path, const char *mode);

const char magic_vgm[4] = {0x56, 0x67, 0x6D, 0x20};
const unsigned char magic_gzip[2] = {0x1F, 0x8B};

bool VGM_Importer::detect(const QString &filePath, char *magic)
{
    if(memcmp(magic_vgm, magic, 4) == 0)
        return true;

    //Try as compressed VGM file
    if(memcmp(magic_gzip, magic, 2) == 0)
    {
        gzFile vgz = gzopen_q(filePath, "rb");
        if(!vgz)
            return false;

        char compMagic[4];
        int count = (int)gzread(vgz, compMagic, 4);
        gzclose(vgz);
        if(count == 4 && memcmp(compMagic, magic_vgm, 4) == 0)
            return true;
    }

    return false;
}

FfmtErrCode VGM_Importer::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    char magic[4];
    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(memcmp(magic_vgm, magic, 4) == 0)
    {
        file.seek(0);
        return load(file, bank);
    }

    file.close();

    //Try as compressed VGM file
    if(memcmp(magic_gzip, magic, 2) == 0)
    {
        gzFile vgz = gzopen_q(filePath, "rb");
        if(!vgz)
            return FfmtErrCode::ERR_NOFILE;

        QBuffer buffer;
        buffer.open(QBuffer::ReadWrite);

        int n;
        char buf[1024];
        while((n = (int)gzread(vgz, buf, sizeof(buf))) > 0)
            buffer.write(buf, n);
        if(n == -1)
            return FfmtErrCode::ERR_BADFORMAT;
        gzclose(vgz);

        buffer.open(QIODevice::ReadOnly);
        return load(buffer, bank);
    }

    return FfmtErrCode::ERR_BADFORMAT;
}

FfmtErrCode VGM_Importer::load(QIODevice &file, FmBank &bank)
{
    RawYmf262ToWopi pseudoOpl2;
    RawYmf262ToWopi pseudoOpl3;
    pseudoOpl3.shareInstruments(pseudoOpl2);

    char    magic[4];
    uint8_t numb[4];

    bank.reset();
    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    if(memcmp(magic, magic_vgm, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    file.seek(0x8);
    if(file.read(char_p(numb), 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;

    uint32_t vgm_version = toUint32LE(numb);
    uint8_t vgm_sizetable[0x100];
    make_size_table(vgm_sizetable, vgm_version);

    uint32_t data_offset = 0xC;
    if(vgm_version >= 0x150)
    {
        file.seek(0x34);
        file.read(char_p(numb), 4);
        data_offset = toUint32LE(numb);
    }
    file.seek(0x34 + data_offset);

    bank.Ins_Melodic_box.clear();

    uint32_t pcm_offset = 0;
    bool end = false;
    while(!end && !file.atEnd())
    {
        uint8_t cmd = 0x00;
        uint8_t reg = 0x00;
        uint8_t val = 0x00;
        file.read(char_p(&cmd), 1);
        switch(cmd)
        {
        default: {
            uint8_t toSkip = vgm_sizetable[cmd];
            if(toSkip != 0xFF)
                file.seek(file.pos() + toSkip);
            else
            {
                //Unrecognized command
                end = true;
            }
            break;
        }

        case 0x5a: // YM3812, write value dd to register aa
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            pseudoOpl2.passReg(reg, val);
            break;

        case 0x5e:   // YMF262 port 0, write value dd to register aa
        case 0x5f: { // YMF262 port 1, write value dd to register aa
            file.read(char_p(&reg), 1);
            file.read(char_p(&val), 1);
            uint16_t regopl3 = reg;
            if(cmd == 0x5f) regopl3 |= 0x100u;
            pseudoOpl3.passReg(regopl3, val);
            break;
        }

        case 0x61://Wait samples
        case 0x62://Wait samples
        case 0x63://Wait samples
        case 0x70:
        case 0x71:
        case 0x72:
        case 0x73:
        case 0x74:
        case 0x75:
        case 0x76:
        case 0x77:
        case 0x78:
        case 0x79:
        case 0x7A:
        case 0x7B:
        case 0x7C:
        case 0x7D:
        case 0x7E:
        case 0x7F:
            if(cmd == 0x61)
                file.seek(file.pos() + 2);
            pseudoOpl2.doAnalyzeState();
            pseudoOpl3.doAnalyzeState();
            break;

        case 0x66://End of sound data
            end = 1;
            break;

        case 0x67://Data block to skip
            file.seek(file.pos() + 2);
            file.read(char_p(numb), 4);
            pcm_offset = toUint32LE(numb);

            // from ValleyBell's vgmtest.c: offset MSB is chip ID
            pcm_offset &= 0x7fffffff;

            file.seek(file.pos() + pcm_offset);
            break;
        }
    }

    const QList<FmBank::Instrument> &insts = pseudoOpl2.caughtInstruments();
    bank.Ins_Melodic_box.reserve(insts.size());
    for(const FmBank::Instrument &inst : insts)
        bank.Ins_Melodic_box.push_back(inst);
    bank.Ins_Melodic = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}

int VGM_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString VGM_Importer::formatName() const
{
    return "Video Game Music";
}

QString VGM_Importer::formatModuleName() const
{
    return "Video Game Music importer";
}

QString VGM_Importer::formatExtensionMask() const
{
    return "*.vgm *.vgz";
}

BankFormats VGM_Importer::formatId() const
{
    return BankFormats::FORMAT_VGM_IMPORTER;
}

static void make_size_table(uint8_t *table, unsigned version)
{
    std::memset(table, 0xFF, 0x100);
    table[0x4F] = 1;  // Game Gear PSG stereo, write dd to port 0x06
    table[0x50] = 1;  // PSG (SN76489/SN76496) write value dd
    table[0x51] = 2;  // YM2413, write value dd to register aa
    table[0x52] = 2;  // YM2612 port 0, write value dd to register aa
    table[0x53] = 2;  // YM2612 port 1, write value dd to register aa
    table[0x54] = 2;  // YM2151, write value dd to register aa
    table[0x55] = 2;  // YM2203, write value dd to register aa
    table[0x56] = 2;  // YM2608 port 0, write value dd to register aa
    table[0x57] = 2;  // YM2608 port 1, write value dd to register aa
    table[0x58] = 2;  // YM2610 port 0, write value dd to register aa
    table[0x59] = 2;  // YM2610 port 1, write value dd to register aa
    table[0x5A] = 2;  // YM3812, write value dd to register aa
    table[0x5B] = 2;  // YM3526, write value dd to register aa
    table[0x5C] = 2;  // Y8950, write value dd to register aa
    table[0x5D] = 2;  // YMZ280B, write value dd to register aa
    table[0x5E] = 2;  // YMF262 port 0, write value dd to register aa
    table[0x5F] = 2;  // YMF262 port 1, write value dd to register aa
    table[0x61] = 2;  // Wait n samples, n can range from 0 to 65535 (approx 1.49 seconds). Longer pauses than this are represented by multiple wait commands.
    table[0x62] = 0;  // wait 735 samples (60th of a second), a shortcut for 0x61 0xdf 0x02
    table[0x63] = 0;  // wait 882 samples (50th of a second), a shortcut for 0x61 0x72 0x03
    table[0x66] = 0;  // end of sound data
    table[0x67] = /* variable size */0xFF;  // data block: see below
    table[0x68] = 11;  // PCM RAM write: see below
    for(unsigned a = 0x70; a <= 0x7F; ++a)
        table[a] = 0;  // wait n+1 samples, n can range from 0 to 15.
    for(unsigned a = 0x80; a <= 0x8F; ++a)
        table[a] = 0;  // YM2612 port 0 address 2A write from the data bank, then wait n samples; n can range from 0 to 15. Note that the wait is n, NOT n+1. See also command 0xE0.
    table[0x90] = 4;  // Setup Stream Control
    table[0x91] = 4;  // Set Stream Data
    table[0x92] = 5;  // Set Stream Frequency
    table[0x93] = 10;  // Start Stream
    table[0x94] = 1;  // Stop Stream
    table[0x95] = 4;  // Start Stream (fast call)
    table[0xA0] = 2;  // AY8910, write value dd to register aa
    table[0xB0] = 2;  // RF5C68, write value dd to register aa
    table[0xB1] = 2;  // RF5C164, write value dd to register aa
    table[0xB2] = 2;  // PWM, write value ddd to register a (d is MSB, dd is LSB)
    table[0xB3] = 2;  // GameBoy DMG, write value dd to register aa
    table[0xB4] = 2;  // NES APU, write value dd to register aa
    table[0xB5] = 2;  // MultiPCM, write value dd to register aa
    table[0xB6] = 2;  // uPD7759, write value dd to register aa
    table[0xB7] = 2;  // OKIM6258, write value dd to register aa
    table[0xB8] = 2;  // OKIM6295, write value dd to register aa
    table[0xB9] = 2;  // HuC6280, write value dd to register aa
    table[0xBA] = 2;  // K053260, write value dd to register aa
    table[0xBB] = 2;  // Pokey, write value dd to register aa
    table[0xBC] = 2;  // WonderSwan, write value dd to register aa
    table[0xBD] = 2;  // SAA1099, write value dd to register aa
    table[0xBE] = 2;  // ES5506, write value dd to register aa
    table[0xBF] = 2;  // GA20, write value dd to register aa
    table[0xC0] = 3;  // Sega PCM, write value dd to memory offset aabb
    table[0xC1] = 3;  // RF5C68, write value dd to memory offset aabb
    table[0xC2] = 3;  // RF5C164, write value dd to memory offset aabb
    table[0xC3] = 3;  // MultiPCM, write set bank offset aabb to channel cc
    table[0xC4] = 3;  // QSound, write value mmll to register rr (mm - data MSB, ll - data LSB)
    table[0xC5] = 3;  // SCSP, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
    table[0xC6] = 3;  // WonderSwan, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
    table[0xC7] = 3;  // VSU, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
    table[0xC8] = 3;  // X1-010, write value dd to memory offset mmll (mm - offset MSB, ll - offset LSB)
    table[0xD0] = 3;  // YMF278B, port pp, write value dd to register aa
    table[0xD1] = 3;  // YMF271, port pp, write value dd to register aa
    table[0xD2] = 3;  // SCC1, port pp, write value dd to register aa
    table[0xD3] = 3;  // K054539, write value dd to register ppaa
    table[0xD4] = 3;  // C140, write value dd to register ppaa
    table[0xD5] = 3;  // ES5503, write value dd to register ppaa
    table[0xD6] = 3;  // ES5506, write value aadd to register pp
    table[0xE0] = 4;  // Seek to offset dddddddd (Intel byte order) in PCM data bank of data block type 0 (YM2612).
    table[0xE1] = 4;  // C352, write value aadd to register mmll

    for(unsigned a = 0x30; a <= 0x3F; ++a)
        table[a] = 1;  // one operand, reserved for future use

    for(unsigned a = 0x40; a <= 0x4E; ++a)
    {
        if(version >= 0x160)
            table[a] = 2;  // two operands, reserved for future use
        else
            table[a] = 1;  // was one operand only til v1.60
    }

    for(unsigned a = 0xA1; a <= 0xAF; ++a)
        table[a] = 2;  // two operands, reserved for future use

    for(unsigned a = 0xC9; a <= 0xCF; ++a)
        table[a] = 3;  // three operands, reserved for future use

    for(unsigned a = 0xD7; a <= 0xDF; ++a)
        table[a] = 3;  // three operands, reserved for future use

    for(unsigned a = 0xE2; a <= 0xFF; ++a)
        table[a] = 4;  // three operands, reserved for future use
}

static gzFile gzopen_q(const QString &path, const char *mode)
{
#ifndef _WIN32
    return gzopen(path.toStdString().c_str(), mode);
#else
    return gzopen_w(path.toStdWString().c_str(), mode);
#endif
}
