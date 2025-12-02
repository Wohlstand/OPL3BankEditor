/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2025 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "format_wohlstand_opl3_text.h"
#include "../common.h"
#include <cstring>
#include <stdio.h>
#include <stdint.h>


static const char *woplx_magic      = "WOPLX-BANK\n";
static const char *woplx_magic_r    = "WOPLX-BANK\r\n"; // Allow files with CRLF too

static FILE *qfopen(const QString &file, const QString &mode)
{
#if defined(_WIN32) && !defined(ENABLE_WIN9X_OPL_PROXY)
    std::wstring fileU = file.toStdWString();
    std::wstring modeU = mode.toStdWString();
#else
    std::string fileU = file.toStdString();
    std::string modeU = mode.toStdString();
#endif

#if defined(_WIN32) && !defined(ENABLE_WIN9X_OPL_PROXY)
    return _wfopen(fileU.c_str(), modeU.c_str());
#else
    return fopen(fileU.c_str(), modeU.c_str());
#endif
}


bool WohlstandOPL3TeXt::detect(const QString &, char *magic)
{
    return (strncmp(magic, woplx_magic, 11) == 0 || strncmp(magic, woplx_magic_r, 12) == 0);
}


FfmtErrCode WohlstandOPL3TeXt::loadFile(QString filePath, FmBank &bank)
{
    QByteArray line;
    QFile file(filePath);
    bool parseInfo = false;
    bool isDrumBank = false;
    int level = 0; // 0 - begin of file, 1 - bank entry, 2 - instrument
    int readI;
    unsigned readU, msb, lsb;
    bool msbHas = false, lsbHas = false;
    FmBank::MidiBank *newBank = nullptr;
    FmBank::Instrument *newInst = nullptr;
    FmBank::Instrument *curInst = nullptr;
    char nameBuf[33] = "";

    bank.reset();

    if(!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return FfmtErrCode::ERR_NOFILE;

    line = file.readLine();

    if(line.compare(woplx_magic) != 0 && line.compare(woplx_magic_r) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    while(!file.atEnd())
    {
        line = file.readLine();

        if(parseInfo)
        {
            if(line.trimmed().compare("BANK_INFO_END"))
                parseInfo = false;
            else
                bank.InfoString.append(QString::fromUtf8(line));
        }
        else if(line.trimmed().isEmpty())
            continue; // Skip empty lines

        switch(level)
        {
        case 0: // External level
            if(line.trimmed().compare("BANK_INFO:") == 0)
                parseInfo = true;
            else if(line.trimmed().compare("MELODIC_BANK:") == 0)
            {
                msb = 0;
                lsb = 0;
                msbHas = false;
                lsbHas = false;
                isDrumBank = false;
                ++level;
            }
            else if(line.trimmed().compare("PERCUSSION_BANK:") == 0)
            {
                msb = 0;
                lsb = 0;
                msbHas = false;
                lsbHas = false;
                isDrumBank = true;
                ++level;
            }
            else if(line.startsWith("DEEP_VIBRATO="))
            {
                if(std::sscanf(line.data(), "DEEP_VIBRATO=%u", &readU) == 0)
                {
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                bank.deep_vibrato = readU;
            }
            else if(line.startsWith("DEEP_TREMOLO="))
            {
                if(std::sscanf(line.data(), "DEEP_TREMOLO=%u", &readU) == 0)
                {
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                bank.deep_tremolo = readU;
            }
            else if(line.startsWith("VOLUME_MODEL="))
            {
                if(std::sscanf(line.data(), "VOLUME_MODEL=%u", &readU) == 0)
                {
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                bank.volume_model = readU;
            }
            else
            {
                // Invalid data!
                bank.reset();
                return FfmtErrCode::ERR_BADFORMAT;
            }
            break;

        case 1: // Bank level
            if(line.startsWith("NAME="))
            {
                size_t i, o;

                for(i = 5, o = 0; o < 33 && i < (size_t)line.size() && line.at(i) != '\n' && line.at(i) != '\r'; ++i, ++o)
                    nameBuf[o] = line.at(i);

                nameBuf[o] = 0;
            }
            else if(line.startsWith("MIDI_BANK_MSB="))
            {
                if(std::sscanf(line.data(), "MIDI_BANK_MSB=%u", &msb) == 0)
                {
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                msbHas = true;
            }
            else if(line.startsWith("MIDI_BANK_LSB="))
            {
                if(std::sscanf(line.data(), "MIDI_BANK_LSB=%u", &lsb) == 0)
                {
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                lsbHas = true;
            }
            else if(line.startsWith("INSTRUMENT="))
            {
                if(!msbHas || !lsbHas)
                {
                    // Missing required data!
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                // Actually start instruments data
                ++level;
                bank.createBank(msb, lsb, isDrumBank, &newBank, &newInst);
                std::strncpy(newBank->name, nameBuf, 33);

                for(size_t i = 0; i < 128; ++i)
                    newInst[i].is_blank = true; // Mark as blank by default

                goto instrument;
            }
            else if((!isDrumBank && line.trimmed().compare("MELODIC_BANK_END") == 0) ||
                    (isDrumBank && line.trimmed().compare("PERCUSSION_BANK_END") == 0))
            {
                if(!msbHas || !lsbHas)
                {
                    // Missing required data!
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                // An empty bank created
                level = 0;
                bank.createBank(msb, lsb, isDrumBank, &newBank, &newInst);
                std::strncpy(newBank->name, nameBuf, 33);
                curInst = nullptr;
                newInst = nullptr;
                newBank = nullptr;
                nameBuf[0] = 0;
            }
            else
            {
                // Invalid data!
                bank.reset();
                return FfmtErrCode::ERR_BADFORMAT;
            }

            break;

        case 2: // Instrument level
instrument:
            if(line.startsWith("INSTRUMENT="))
            {
                if(std::sscanf(line.data(), "INSTRUMENT=%u:", &readU) == 0 || readU > 127)
                {
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                curInst = newInst + readU;
                if(isDrumBank)
                    curInst->is_fixed_note = true;

                curInst->is_blank = false; // Not blank!
                break;
            }

            if(!curInst)
            {
                // Instrument is NOT selected!
                bank.reset();
                return FfmtErrCode::ERR_BADFORMAT;
            }

            if(line.startsWith("NAME="))
            {
                size_t i, o;

                for(i = 5, o = 0; o < 33 && i < (size_t)line.size() && line.at(i) != '\n' && line.at(i) != '\r'; ++i, ++o)
                    curInst->name[o] = line.at(i);

                curInst->name[o] = 0;
            }
            else if(line.startsWith("FLAGS:"))
            {
                int opFlags = 0;

                if(std::strstr(line.data(), "FN;"))
                    curInst->is_fixed_note = true;

                if(std::strstr(line.data(), "2OP;"))
                {
                    curInst->en_4op = false;
                    curInst->en_pseudo4op = false;
                    ++opFlags;
                }

                if(std::strstr(line.data(), "DV;"))
                {
                    curInst->en_4op = true;
                    curInst->en_pseudo4op = true;
                    ++opFlags;
                }

                if(std::strstr(line.data(), "4OP;"))
                {
                    curInst->en_4op = true;
                    curInst->en_pseudo4op = false;
                    ++opFlags;
                }

                if(opFlags != 1)
                {
                    // Illegal combo!
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }
            }
            else if(line.startsWith("ATTRS:"))
            {
                const char *key;

                if((key = std::strstr(line.data(), "DRUM_KEY=")) != NULL)
                {
                    if(std::sscanf(key, "DRUM_KEY=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->percNoteNum = readU;
                }

                if((key = std::strstr(line.data(), "NOTE_OFF_1=")) != NULL)
                {
                    if(std::sscanf(key, "NOTE_OFF_1=%d;", &readI) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->note_offset1 = (int16_t)readI;
                }

                if((key = std::strstr(line.data(), "NOTE_OFF_2=")) != NULL)
                {
                    if(std::sscanf(key, "NOTE_OFF_2=%d;", &readI) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->note_offset2 = (int16_t)readI;
                }

                if((key = std::strstr(line.data(), "VEL_OFF=")) != NULL)
                {
                    if(std::sscanf(key, "VEL_OFF=%d;", &readI) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->velocity_offset = (int8_t)readI;
                }

                if((key = std::strstr(line.data(), "FINE_TUNE=")) != NULL)
                {
                    if(std::sscanf(key, "FINE_TUNE=%d;", &readI) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->fine_tune = (int8_t)readI;
                }

                if((key = std::strstr(line.data(), "RHYTHM=")) != NULL)
                {
                    if(std::sscanf(key, "RHYTHM=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->rhythm_drum_type = readU;
                }

                if((key = std::strstr(line.data(), "DUR_K_ON=")) != NULL)
                {
                    if(std::sscanf(key, "DUR_K_ON=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->ms_sound_kon = readU;
                }

                if((key = std::strstr(line.data(), "DUR_K_OFF=")) != NULL)
                {
                    if(std::sscanf(key, "DUR_K_OFF=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->ms_sound_koff = readU;
                }
            }
            else if(line.startsWith("FBCONN:"))
            {
                const char *key;

                if((key = std::strstr(line.data(), "FB1=")) != NULL)
                {
                    if(std::sscanf(key, "FB1=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->feedback1 = readU;
                }

                if((key = std::strstr(line.data(), "FB2=")) != NULL)
                {
                    if(std::sscanf(key, "FB2=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->feedback2 = readU;
                }

                if((key = std::strstr(line.data(), "CONN1=")) != NULL)
                {
                    if(std::sscanf(key, "CONN1=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->connection1 = readU > 0;
                }

                if((key = std::strstr(line.data(), "CONN2=")) != NULL)
                {
                    if(std::sscanf(key, "CONN2=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    curInst->connection2 = readU > 0;
                }
            }
            else if(line.startsWith("OP0:") || line.startsWith("OP1:") || line.startsWith("OP2:") || line.startsWith("OP3:"))
            {
                const char *key;
                size_t op;

                if(std::sscanf(line.data(), "OP%u: ", &readU) == 0)
                {
                    bank.reset();
                    return FfmtErrCode::ERR_BADFORMAT;
                }

                op = readU;

                FmBank::Operator &opr = curInst->OP[op];

                if((key = std::strstr(line.data(), "AT=")) != NULL)
                {
                    if(std::sscanf(key, "AT=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.attack = readU & 0x0F;
                }

                if((key = std::strstr(line.data(), "DC=")) != NULL)
                {
                    if(std::sscanf(key, "DC=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.decay = readU & 0x0F;
                }

                if((key = std::strstr(line.data(), "ST=")) != NULL)
                {
                    if(std::sscanf(key, "ST=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    // OPL3-BE uses inverted sustain value for human convenience
                    opr.sustain = 0x0F - (readU & 0x0F);
                }

                if((key = std::strstr(line.data(), "RL=")) != NULL)
                {
                    if(std::sscanf(key, "RL=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.release = readU & 0x0F;
                }

                if((key = std::strstr(line.data(), "WF=")) != NULL)
                {
                    if(std::sscanf(key, "WF=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.waveform = readU & 0x07;
                }

                if((key = std::strstr(line.data(), "ML=")) != NULL)
                {
                    if(std::sscanf(key, "ML=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.fmult = readU & 0x0F;
                }

                if((key = std::strstr(line.data(), "TL=")) != NULL)
                {
                    if(std::sscanf(key, "TL=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    // OPL3-BE uses inverted sustain value for human convenience
                    opr.level = 0x3F - (readU & 0x3F);
                }

                if((key = std::strstr(line.data(), "KL=")) != NULL)
                {
                    if(std::sscanf(key, "KL=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.ksl = readU & 0x03;
                }

                if((key = std::strstr(line.data(), "VB=")) != NULL)
                {
                    if(std::sscanf(key, "VB=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.vib = readU > 0;
                }

                if((key = std::strstr(line.data(), "AM=")) != NULL)
                {
                    if(std::sscanf(key, "AM=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.am = readU > 0;
                }

                if((key = std::strstr(line.data(), "EG=")) != NULL)
                {
                    if(std::sscanf(key, "EG=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.eg = readU > 0;
                }

                if((key = std::strstr(line.data(), "KR=")) != NULL)
                {
                    if(std::sscanf(key, "KR=%u;", &readU) == 0)
                    {
                        bank.reset();
                        return FfmtErrCode::ERR_BADFORMAT;
                    }

                    opr.ksr = readU > 0;
                }
            }
            else if((!isDrumBank && line.trimmed().compare("MELODIC_BANK_END") == 0) ||
                    (isDrumBank && line.trimmed().compare("PERCUSSION_BANK_END") == 0))
            {
                level = 0;
                curInst = nullptr;
                newInst = nullptr;
                newBank = nullptr;
                nameBuf[0] = 0;
            }
            else
            {
                // Invalid data!
                bank.reset();
                return FfmtErrCode::ERR_BADFORMAT;
            }

            break;
        }
    }

    file.close();

    return FfmtErrCode::ERR_OK;
}


FfmtErrCode WohlstandOPL3TeXt::saveFile(QString filePath, FmBank &bank)
{
    FmBank::Instrument null;
    FILE *out;

    memset(&null, 0, sizeof(FmBank::Instrument));

    out = qfopen(filePath, "wb");

    if(!out)
        return FfmtErrCode::ERR_NOFILE;

    fprintf(out, "%s\n", woplx_magic);

    if(!bank.InfoString.isEmpty())
    {
        std::string bank_info = bank.InfoString.toStdString();
        fprintf(out, "BANK_INFO:\n%s\nBANK_INFO_END\n\n", bank_info.c_str());
    }

    fprintf(out, "DEEP_VIBRATO=%u\n", bank.deep_vibrato ? 1 : 0);
    fprintf(out, "DEEP_TREMOLO=%u\n", bank.deep_tremolo ? 1 : 0);
    fprintf(out, "VOLUME_MODEL=%u\n", bank.volume_model);

    fprintf(out, "\n\n");

    uint16_t count_melodic_banks   = uint16_t(((bank.countMelodic() - 1)/ 128) + 1);
    uint16_t count_percusive_banks = uint16_t(((bank.countDrums() - 1)/ 128) + 1);
    FmBank::Instrument *slots_src_ins[2] = {bank.Ins_Melodic, bank.Ins_Percussion};
    size_t slots_src_ins_counts[2] = {(size_t)bank.countMelodic(), (size_t)bank.countDrums()};
    FmBank::MidiBank *slots_src_banks[2] =  {bank.Banks_Melodic.data(), bank.Banks_Percussion.data()};
    size_t slots_counts[2] = {count_melodic_banks, count_percusive_banks};

    for(size_t ss = 0; ss < 2; ss++)
    {
        bool isDrum = (ss == 1);
        for(size_t b = 0; b < slots_counts[ss]; ++b)
        {
            FmBank::MidiBank &bnk = slots_src_banks[ss][b];

            fprintf(out, isDrum ? "PERCUSSION_BANK:\n" : "MELODIC_BANK:\n");

            if(bnk.name[0] != 0)
                fprintf(out, "NAME=%s\n", bnk.name);

            fprintf(out, "MIDI_BANK_MSB=%u\n", bnk.msb);
            fprintf(out, "MIDI_BANK_LSB=%u\n", bnk.lsb);
            fprintf(out, "\n");

            for(unsigned j = 0; j < 128; ++j)
            {
                size_t ins_index = (size_t(b) * 128) + size_t(j);
                if(slots_src_ins_counts[ss] < ins_index)
                    break;

                FmBank::Instrument &ins = slots_src_ins[ss][ins_index];
                if(ins.is_blank)
                    continue; // Skip blank instruments!

                bool is2op = !ins.en_4op && !ins.en_pseudo4op;
                size_t ops = is2op ? 2 : 4;

                fprintf(out, "INSTRUMENT=%u:\n", j);

                if(ins.name[0] != 0)
                    fprintf(out, "NAME=%s\n", ins.name);

                // Flags line
                fprintf(out, "FLAGS: ");

                if(!isDrum && ins.is_fixed_note)
                    fprintf(out, "FN;");

                if(is2op)
                    fprintf(out, "2OP;");
                else if(ins.en_4op && ins.en_pseudo4op)
                    fprintf(out, "DV;");
                else if(ins.en_4op)
                    fprintf(out, "4OP;");

                fprintf(out, "\n");


                fprintf(out, "ATTRS: ");

                if((isDrum || ins.is_fixed_note) && ins.percNoteNum > 0)
                    fprintf(out, "DRUM_KEY=%u;", ins.percNoteNum);

                if(ins.note_offset1 != 0)
                    fprintf(out, "NOTE_OFF_1=%d;", ins.note_offset1);

                if(!is2op && ins.note_offset2 != 0)
                    fprintf(out, "NOTE_OFF_2=%d;", ins.note_offset2);

                if(ins.velocity_offset != 0)
                    fprintf(out, "VEL_OFF=%d;", ins.velocity_offset);

                if(ins.en_pseudo4op && ins.fine_tune != 0)
                    fprintf(out, "FINE_TUNE=%d;", ins.fine_tune);

                if(isDrum && ins.rhythm_drum_type > 0)
                    fprintf(out, "RHYTHM=%u;", ins.rhythm_drum_type);

                if(ins.ms_sound_kon > 0)
                    fprintf(out, "DUR_K_ON=%u;", ins.ms_sound_kon);

                if(ins.ms_sound_koff > 0)
                    fprintf(out, "DUR_K_OFF=%u;", ins.ms_sound_koff);

                fprintf(out, "\n");

                fprintf(out, "FBCONN: ");
                fprintf(out, "FB1=%u;", ins.feedback1);
                fprintf(out, "CONN1=%u;", ins.connection1 ? 1 : 0);

                if(!is2op)
                {
                    fprintf(out, "FB2=%u;", ins.feedback2);
                    fprintf(out, "CONN2=%u;", ins.connection2 ? 1 : 0);
                }

                fprintf(out, "\n");

                for(unsigned int op = 0; op < ops; ++op)
                {
                    FmBank::Operator &o = ins.OP[op];
                    fprintf(out, "OP%u: ", op);
                    fprintf(out, "AT=%u;", o.attack);
                    fprintf(out, "DC=%u;", o.decay);
                    fprintf(out, "ST=%u;", (0x0F - (o.sustain & 0x0F)));
                    fprintf(out, "RL=%u;", o.release);
                    fprintf(out, "WF=%u;", o.waveform);
                    fprintf(out, "ML=%u;", o.fmult);
                    fprintf(out, "TL=%u;", (0x3F - (o.level & 0x3F)));
                    fprintf(out, "KL=%u;", o.ksl);
                    fprintf(out, "VB=%u;", (int)o.vib);
                    fprintf(out, "AM=%u;", (int)o.am);
                    fprintf(out, "EG=%u;", (int)o.eg);
                    fprintf(out, "KR=%u;", (int)o.ksr);
                    fprintf(out, "\n");
                }

                fprintf(out, "\n");

            }

            fprintf(out, isDrum ? "PERCUSSION_BANK_END\n\n\n" : "MELODIC_BANK_END\n\n\n");
        }
    }

    fclose(out);

    return FfmtErrCode::ERR_OK;
}


int WohlstandOPL3TeXt::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN |
           (int)FormatCaps::FORMAT_CAPS_SAVE |
           (int)FormatCaps::FORMAT_CAPS_IMPORT |
           (int)FormatCaps::FORMAT_CAPS_NEEDS_MEASURE;
}


QString WohlstandOPL3TeXt::formatName() const
{
    return "Text-based OPL3 bank format by Wohlstand";
}


QString WohlstandOPL3TeXt::formatExtensionMask() const
{
    return "*.woplx *.wox";
}


QString WohlstandOPL3TeXt::formatDefaultExtension() const
{
    return "woplx";
}


BankFormats WohlstandOPL3TeXt::formatId() const
{
    return BankFormats::FORMAT_WOHLSTAND_OPL3_TEXT;
}
