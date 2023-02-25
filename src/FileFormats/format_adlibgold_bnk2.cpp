/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_adlibgold_bnk2.h"
#include "../common.h"
#include <QDebug>
#include <cstring>

static const char *AdLibGoldBnk2_magic = "Accomp. Bank, (C) AdLib Inc";

bool AdLibGoldBnk2_reader::detect(const QString &, char *magic)
{
    return !std::memcmp(magic, AdLibGoldBnk2_magic, 28);
}

static void convertInstrument(
    const uint8_t src[28], FmBank::Instrument &dst, const char *name)
{
    unsigned char C4xxxFFFC = src[24];
    unsigned char xxP24NNN = src[25];
    unsigned char TTTTTTTT = src[26];
    // unsigned char xxxxxxxx = src[27];

    dst.is_blank = false;
    std::strcpy(dst.name, name);

    for(unsigned nth_op = 0; nth_op < 4; ++nth_op)
    {
        static const unsigned opnums[4] =
            { MODULATOR1, CARRIER1, MODULATOR2, CARRIER2 };
        unsigned opnum = opnums[nth_op];

        const uint8_t *op = src + 6 * nth_op;
        dst.setAVEKM(opnum, op[0]);
        dst.setKSLL(opnum, op[1]);
        dst.setAtDec(opnum, op[2]);
        dst.setSusRel(opnum, op[3]);
        dst.setWaveForm(opnum, op[4]);
    }
    dst.setFBConn1(C4xxxFFFC & 0x0F);
    dst.setFBConn2((C4xxxFFFC & 0x0E) | (C4xxxFFFC >> 7));

    dst.fine_tune = (int8_t)TTTTTTTT;
    dst.en_4op = (xxP24NNN & 8) != 0;
    dst.en_pseudo4op = dst.en_4op && (xxP24NNN & 16) != 0;  // pseudo-4op maybe?

    //--- rhythm mode percussion: BD=0, SD=1, TOM=2, CYMB=3, HH=4
    bool is_perc = (src[25] & 32) != 0;
    unsigned perc = src[25] & 7;

    switch(is_perc ? perc : -1)
    {
    case 0:
        dst.rhythm_drum_type = 6;
        break;
    case 1:
        dst.rhythm_drum_type = 7;
        break;
    case 2:
        dst.rhythm_drum_type = 8;
        break;
    case 3:
        dst.rhythm_drum_type = 9;
        break;
    case 4:
        dst.rhythm_drum_type = 10;
        break;
    }
}

FfmtErrCode AdLibGoldBnk2_reader::loadFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    QByteArray fileData  = file.readAll();
    file.close();
    unsigned fileSize = fileData.size();

    if(fileSize < 42 || std::memcmp(fileData.data(), AdLibGoldBnk2_magic, 28))
        return FfmtErrCode::ERR_BADFORMAT;

    unsigned ins_entries = fileData[40] | (fileData[41] << 8);
    const uint8_t *records = (const uint8_t *)&fileData.data()[48];

    // find all the non-blank instruments
    std::vector<bool> ins_bitmap(ins_entries);
    std::vector<std::string> ins_names(ins_entries);
    for(unsigned n = 0; n < ins_entries; ++n)
    {
        const size_t offset1 = n * 28;
        if(&records[offset1 + 15] >= (const uint8_t *)fileData.end())
            return FfmtErrCode::ERR_BADFORMAT;

        unsigned used = records[offset1 + 15];

        if(!used)
            continue;

        std::string name;
        name.reserve(12);
        for(unsigned p = 0; p < 12; ++p)
        {
            if(&records[offset1 + 3 + p] >= (const uint8_t *)fileData.end())
                return FfmtErrCode::ERR_BADFORMAT;
            if(records[offset1 + 3 + p] == '\0') break;
            name.push_back(records[offset1 + 3 + p]);
        }
        ins_names[n] = std::move(name);
        ins_bitmap[n] = true;
    }

    // classify the numbered instruments by prefix on their name
    std::map<std::string, std::map<unsigned, unsigned>> pseudoBanks;  // Prefix -> Entry -> GM number
    std::vector<unsigned> unclassifiedInstrumentsBin;
    for(unsigned n = 0; n < ins_entries; ++n)
    {
        bool used = ins_bitmap[n];

        if(!used)
            continue;

        QString name = QString::fromStdString(ins_names[n]);
        QString prefix;

        unsigned number = 0;
        if(!name.isEmpty() && name[name.size() - 1].isDigit())
        {
            prefix = name.left(prefix.size() - 1);
            unsigned number_length = 1;
            while(!prefix.isEmpty() && prefix[prefix.size() - 1].isDigit())
            {
                ++number_length;
                prefix.resize(prefix.size() - 1);
            }
            number = name.right(name.size() - prefix.size()).toUInt();
        }

        // qDebug() << "Ins" << name << "Prefix" << prefix << "Number" << number;

        if(prefix.isEmpty() || number >= 128)
            unclassifiedInstrumentsBin.push_back(n);
        else
        {
            std::map<unsigned, unsigned> &dst = pseudoBanks[prefix.toStdString()];
            if(dst.find(n) == dst.end())
                dst[n] = number;
            else
                unclassifiedInstrumentsBin.push_back(n);
        }
    }

    // reserve needed instrument space
    unsigned melo_banks =
        pseudoBanks.size() + (unclassifiedInstrumentsBin.size() + 127) / 128;
    bank.reset((melo_banks > 0) ? melo_banks : 1, 1);
    for (unsigned i = 0; i < melo_banks; ++i)
    {
        FmBank::MidiBank &mb = bank.Banks_Melodic[i];
        mb.msb = i / 128;
        mb.lsb = i % 128;
    }
    // mark everything as blank
    for (unsigned b_i = 0; b_i < melo_banks; ++b_i)
    {
        FmBank::Instrument *instruments = &bank.Ins_Melodic[128 * b_i];
        for (unsigned p_i = 0; p_i < 128; ++p_i)
            instruments[p_i].is_blank = true;
    }
    for (unsigned p_i = 0; p_i < 128; ++p_i)
        bank.Ins_Percussion[p_i].is_blank = true;

    // populate the entries of banks
    unsigned nth_bank = 0;
    for(const auto &kvBank : pseudoBanks)
    {
        FmBank::MidiBank &mb = bank.Banks_Melodic[nth_bank];
        const std::string &bank_name = kvBank.first;
        std::strncpy(mb.name, bank_name.c_str(), sizeof(mb.name));
        for(const auto &kvGm : kvBank.second)
        {
            unsigned n = kvGm.first;
            unsigned nth_inst = kvGm.second;

            const std::string &ins_name = ins_names[n];

            // qDebug() << "Bank" << QString::fromStdString(bank_name)
            //          << "Entry" << n
            //          << "Ins" << nth_inst
            //          << "Name" << QString::fromStdString(ins_name);

            const size_t offset1 = n * 28;
            if(&records[offset1 + 23] >= (const uint8_t *)fileData.end())
                return FfmtErrCode::ERR_BADFORMAT;

            unsigned offset2 =
                records[offset1 + 20] | (records[offset1 + 21] << 8) |
                (records[offset1 + 22] << 16) | (records[offset1 + 23] << 24);

            const uint8_t *insdata = (const uint8_t *)&fileData.data()[offset2];
            if(&insdata[28] >= (const uint8_t *)fileData.end())
                return FfmtErrCode::ERR_BADFORMAT;

            FmBank::Instrument &dst = bank.Ins_Melodic[nth_bank * 128 + nth_inst];
            convertInstrument(insdata, dst, ins_name.c_str());
        }
        ++nth_bank;
    }

    // populate the rest of entries
    for(unsigned p_i = 0, p_n = unclassifiedInstrumentsBin.size(); p_i < p_n; ++p_i)
    {
        unsigned n = unclassifiedInstrumentsBin[p_i];
        const std::string &ins_name = ins_names[n];

        nth_bank = pseudoBanks.size() + p_i / 128;
        unsigned nth_inst = p_i % 128;

        const size_t offset1 = n * 28;
        if(&records[offset1 + 23] >= (const uint8_t *)fileData.end())
            return FfmtErrCode::ERR_BADFORMAT;

        unsigned offset2 =
            records[offset1 + 20] | (records[offset1 + 21] << 8) |
            (records[offset1 + 22] << 16) | (records[offset1 + 23] << 24);

        const uint8_t *insdata = (const uint8_t *)&fileData.data()[offset2];
        if(&insdata[28] >= (const uint8_t *)fileData.end())
            return FfmtErrCode::ERR_BADFORMAT;

        FmBank::Instrument &dst = bank.Ins_Melodic[nth_bank * 128 + nth_inst];
        convertInstrument(insdata, dst, ins_name.c_str());
    }

    return FfmtErrCode::ERR_OK;
}

int AdLibGoldBnk2_reader::formatCaps() const
{
    return int(FormatCaps::FORMAT_CAPS_OPEN) | int(FormatCaps::FORMAT_CAPS_IMPORT);
}

QString AdLibGoldBnk2_reader::formatName() const
{
    return "AdLib Gold instrument bank";
}

QString AdLibGoldBnk2_reader::formatModuleName() const
{
    return "AdLib Gold bank format reader";
}

QString AdLibGoldBnk2_reader::formatExtensionMask() const
{
    return "*.bnk";
}

QString AdLibGoldBnk2_reader::formatDefaultExtension() const
{
    return "bnk";
}

BankFormats AdLibGoldBnk2_reader::formatId() const
{
    return BankFormats::FORMAT_ADLIBGOLD_BNK2;
}
