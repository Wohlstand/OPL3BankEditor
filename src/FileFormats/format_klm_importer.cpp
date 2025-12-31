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

#include <set>
#include <QFileInfo>

#include "format_klm_importer.h"
#include "../common.h"

// https://moddingwiki.shikadi.net/wiki/KLM_Format

bool KLM_Importer::detect(const QString &filePath, char *magic)
{
    uint16_t song_offset = 0;

    if(magic[2] != 0x01)
        return false;

    QFileInfo file(filePath);

    song_offset = toUint16LE(reinterpret_cast<uint8_t*>(magic) + 3);

    if(song_offset > file.size())
        return false;

    if((song_offset - 5) % 11 != 0) // Invalid offset!
        return false;

    return filePath.endsWith(".klm", Qt::CaseInsensitive);
}

FfmtErrCode KLM_Importer::loadFile(QString filePath, FmBank &bank)
{
    uint16_t song_offset;
    uint8_t insIn[11];
    FmBank::Instrument ins = FmBank::emptyInst();

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset();

    file.seek(3); // Skip tempo (2 bytes) and unknown byte (1 byte)

    if(readLE(file, song_offset) != 2)
    {
        // Can't read offset
        file.close();
        return FfmtErrCode::ERR_BADFORMAT;
    }

    if(song_offset <= 4)
    {
        // Song offset is too small
        file.close();
        return FfmtErrCode::ERR_BADFORMAT;
    }

    if((qint64)song_offset >= file.size())
    {
        // Offset is too large
        file.close();
        return FfmtErrCode::ERR_BADFORMAT;
    }

    bank.Ins_Melodic_box.clear();
    bank.Ins_Percussion_box.clear();

    // Reading instruments while they available
    unsigned i = 0;
    while(file.pos() < (qint64)song_offset && file.read((char*)insIn, 11) == 11)
    {
        if(file.pos() > (qint64)song_offset)
        {
            // Offset mismatches!
            file.close();
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        ins.setKSLL(MODULATOR1,     insIn[0]);
        ins.setKSLL(CARRIER1,       insIn[1]);
        ins.setAtDec(MODULATOR1,    insIn[2]);
        ins.setAtDec(CARRIER1,      insIn[3]);
        ins.setSusRel(MODULATOR1,   insIn[4]);
        ins.setSusRel(CARRIER1,     insIn[5]);
        ins.setAVEKM(MODULATOR1,    insIn[6]);
        ins.setAVEKM(CARRIER1,      insIn[7]);
        ins.setWaveForm(MODULATOR1, insIn[8]);
        ins.setWaveForm(CARRIER1,   insIn[9]);
        ins.setFBConn1(insIn[10]);

        snprintf(ins.name, 32, "KLM Instr. %03u", i++);
        bank.Ins_Melodic_box.push_back(ins);
    }


    // Find which instruments are percussive around the song data
    unsigned drum_i = 0;
    unsigned cur_inst[16];
    unsigned tone[16];
    unsigned octave[16];
    std::set<unsigned> rhythm_stored;

    memset(cur_inst, 0, sizeof(cur_inst));
    memset(tone, 0, sizeof(tone));
    memset(octave, 0, sizeof(octave));

    while(!file.atEnd())
    {
        uint8_t cmd;
        uint8_t chan;
        uint8_t data[2];

        if(file.read((char*)&cmd, 1) != 1)
        {
            // Can't read command
            file.close();
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        chan = cmd & 0x0F;

        switch(cmd & 0xF0)
        {
        case 0x00: // Note Off;
            break; // 0
        case 0x10: // Note on with frequency
            if(chan <= 5) // Melodic instrument and bass drum, two bytes should be skipped
            {
                file.read((char*)data, 2);
                tone[chan] = data[0] && ((unsigned)(data[1] & 0x03) << 8);
                octave[chan] = (data[1] >> 2) & 0x07;
            }
            else // Caught a rhythm-mode instrument
            {
                if(chan == 6) // Bass drum also has frequency
                {
                    file.read((char*)data, 2);
                    tone[chan] = data[0] && ((unsigned)(data[1] & 0x03) << 8);
                    octave[chan] = (data[1] >> 2) & 0x07;
                }

                if(rhythm_stored.find(cur_inst[chan]) == rhythm_stored.end())
                {
                    ins = bank.Ins_Melodic_box[cur_inst[chan]];
                    switch(chan)
                    {
                    case 0x06:
                        ins.rhythm_drum_type = 6;
                        ins.percNoteNum = 50;
                        break;
                    case 0x07:
                        ins.rhythm_drum_type = 7;
                        ins.percNoteNum = 52;
                        break;
                    case 0x08:
                        ins.rhythm_drum_type = 8;
                        ins.percNoteNum = 64;
                        break;
                    case 0x09:
                        ins.rhythm_drum_type = 9;
                        ins.percNoteNum = 64;
                        break;
                    case 0x0A:
                        ins.rhythm_drum_type = 10;
                        ins.percNoteNum = 64;
                        break;

                    default:
                        ins.rhythm_drum_type = 0;
                        ins.percNoteNum = 42;
                        break;
                    }

                    snprintf(ins.name, 32, "KLM Drum. %03u (idx=%03u)", drum_i++, cur_inst[chan]);
                    bank.Ins_Percussion_box.push_back(ins);
                    rhythm_stored.insert(cur_inst[chan]);
                }
            }

            break;
        case 0x20: // Volume
            file.read((char*)data, 1);
            break;

        case 0x30: // Set Instrument
            file.read((char*)data, 1);
            cur_inst[chan] = data[0];
            if(cur_inst[chan] >= (unsigned)bank.Ins_Melodic_box.size())
            {
                // Invalid number of instrument!
                file.close();
                bank.reset();
                return FfmtErrCode::ERR_BADFORMAT;
            }
            break;

        case 0x40: // Note On without frequency
            break;

        case 0xF0: // Special command
            switch(cmd)
            {
            case 0xFD: // Delay
                file.read((char*)data, 1);
                break;
            case 0xFE: // Long delay
                file.read((char*)data, 2);
                break;
            case 0xFF: // End of song
                break;
            default:
                // Invalid command
                file.close();
                bank.reset();
                return FfmtErrCode::ERR_BADFORMAT;
            }
            break;

        default:
            // Invalid command
            file.close();
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
    }

    file.close();

    // Remove instruments captured as rhythm mode
    for(std::set<unsigned>::reverse_iterator d = rhythm_stored.rbegin(); d != rhythm_stored.rend(); ++d)
        bank.Ins_Melodic_box.remove(*d);

    bank.Ins_Percussion = bank.Ins_Percussion_box.data();
    bank.Ins_Melodic    = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}

int KLM_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString KLM_Importer::formatName() const
{
    return "Wacky Wheeles KLM Music Format";
}

QString KLM_Importer::formatModuleName() const
{
    return "Wacky Wheeles KLM Music Importer";
}

QString KLM_Importer::formatExtensionMask() const
{
    return "*.klm";
}

BankFormats KLM_Importer::formatId() const
{
    return BankFormats::FORMAT_KLM_IMPORTER;
}
