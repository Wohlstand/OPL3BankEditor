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

#include "adlibbnk.h"
#include "../common.h"

static const char* bnk_magic = "ADLIB-";

bool AdLibBnk::detect(char *magic)
{
    return (strncmp(magic+2, bnk_magic, 6) == 0);
}

int AdLibBnk::loadFile(QString filePath, FmBank &bank)
{
    char magic[8]; memset(magic, 0, 8);

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    bank.reset();

    if(file.read(magic, 8) != 8)
        return ERR_BADFORMAT;

    if( strncmp(magic+2, bnk_magic, 6) != 0 )
        return ERR_BADFORMAT;

    char    ver_maj = magic[0],
            ver_min = magic[1];
    unsigned short totalInsUsed = 0;
    unsigned short totalIns = 0;
    unsigned int offsetName;
    unsigned int offsetData;
    if( readLE(file, totalInsUsed) != 2 )
        return ERR_BADFORMAT;
    if( readLE(file, totalIns) != 2 )
        return ERR_BADFORMAT;
    if( readLE(file, offsetName) != 4 )
        return ERR_BADFORMAT;
    if( readLE(file, offsetData) != 4 )
        return ERR_BADFORMAT;
    if( file.read(magic, 8) !=8 )//Just read a 8 bytes padding
        return ERR_BADFORMAT;

    //Fetch data first (then fill instrument names to their places!)
    if(!file.seek( offsetData ))
    {
        bank.reset();
        return ERR_BADFORMAT;
    }

    //offsetInstr = offsetData + (index * sizeof(PackedTimbre))
    for(unsigned int i=0; i<totalIns; i++ )
    {
        unsigned char idata[31];
        if( file.read(char_p(idata), 31) != 31 )
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        FmBank::Instrument *ins_p = 0;
        if( idata[0] == 0 )
        {
            if( i >= bank.Ins_Melodic_box.size() )
            {
                FmBank::Instrument ins;
                memset(&ins, 0, sizeof(FmBank::Instrument));
                bank.Ins_Melodic_box.push_back(ins);
            }
            ins_p = &bank.Ins_Melodic[i];
        }
        else
        {
            if( i >= bank.Ins_Percussion_box.size() )
            {
                FmBank::Instrument ins;
                memset(&ins, 0, sizeof(FmBank::Instrument));
                bank.Ins_Percussion_box.push_back(ins);
            }
            ins_p = &bank.Ins_Percussion[i];
        }

        FmBank::Instrument ins = *ins_p;
        //At this point, the current position should be the same as offsetData. The actual instrument
        //data follows, again repeated once for each instrument. The instrument data is in the following
        //format, which is almost identical to the AdLib Instrument Format except with only one byte to
        //store each field instead of two.

        //0    UINT8 	iPercussive 	0: Melodic instrument
        //                              1: Percussive instrument
        //1    UINT8 	iVoiceNum       Voice number (percussive only)
        ins.percNoteNum = idata[1];
        //    OPLREGS 	oplModulator 	Register values for the Modulator operator (op 0)
            //2    UINT8 	ksl         Key scaling level                       0x40 (bits 6-7)
            //3    UINT8 	multiple 	Frequency multiplier                    0x20 (bits 0-3) 	iMultiple & 0x0F is sent to OPL register [verify this]
            //4    UINT8 	feedback 	Feedback [op 0 only, op 1 ignored]      0xC0 (bits 1-3)
            //5    UINT8 	attack      Attack rate                             0x60 (upper four bits) 	[verify this]
            //6    UINT8 	sustain 	Sustain level                           0x80 (upper four bits) 	[verify this]
            //7    UINT8 	eg          Envelope gain (nonzero value is on) 	0x20 (bit 5) 	[verify this]
            //8    UINT8 	decay       Decay rate                              0x60 (lower four bits) 	[verify this]
            //9    UINT8 	releaseRate Release rate                            0x80 (lower four bits) 	[verify this]
            //10    UINT8 	totalLevel 	Total output level                      0x40 (bit 0-5) 	[verify this]
            //11    UINT8 	am          Amplitude modulation (Tremolo)          0x20 (bit 7) 	[verify this]
            //12    UINT8 	vib         Frequency Vibrato                       0x20 (bit 6) 	[verify this]
            //13    UINT8 	ksr         Key scaling/envelope rate               0x20 (bit 4) 	[verify this]
            //14    UINT8 	con         Connector [op 0 only, op 1 ignored] 	0xC0 (bit 0, inverted) 	0: OPL bit set to 1
            //                                                                                  other: OPL bit set to 0
        //    OPLREGS 	oplCarrier      Register values for the Carrier operator (op 1)
            //15    UINT8 	ksl         Key scaling level                       0x40 (bits 6-7)
            //16    UINT8 	multiple 	Frequency multiplier                    0x20 (bits 0-3) 	iMultiple & 0x0F is sent to OPL register [verify this]
            //17    UINT8 	feedback 	Feedback [op 0 only, op 1 ignored]      0xC0 (bits 1-3)
            //18    UINT8 	attack      Attack rate                             0x60 (upper four bits) 	[verify this]
            //19    UINT8 	sustain 	Sustain level                           0x80 (upper four bits) 	[verify this]
            //20    UINT8 	eg          Envelope gain (nonzero value is on) 	0x20 (bit 5) 	[verify this]
            //21    UINT8 	decay       Decay rate                              0x60 (lower four bits) 	[verify this]
            //22    UINT8 	releaseRate Release rate                            0x80 (lower four bits) 	[verify this]
            //23    UINT8 	totalLevel 	Total output level                      0x40 (bit 0-5) 	[verify this]
            //24    UINT8 	am          Amplitude modulation (Tremolo)          0x20 (bit 7) 	[verify this]
            //25    UINT8 	vib         Frequency Vibrato                       0x20 (bit 6) 	[verify this]
            //26    UINT8 	ksr         Key scaling/envelope rate               0x20 (bit 4) 	[verify this]
            //27    UINT8 	con         Connector [op 0 only, op 1 ignored] 	0xC0 (bit 0, inverted) 	0: OPL bit set to 1
            //28                                                                                  other: OPL bit set to 0
        //29    UINT8 	iModWaveSel 	Modulator wave select (OPL base register 0xE0)
        //30    UINT8 	iCarWaveSel 	Carrier wave select (OPL base register 0xE0)


        //The OPLREGS structure is defined as:
        //    UINT8 	ksl         Key scaling level                       0x40 (bits 6-7)
        //    UINT8 	multiple 	Frequency multiplier                    0x20 (bits 0-3) 	iMultiple & 0x0F is sent to OPL register [verify this]
        //    UINT8 	feedback 	Feedback [op 0 only, op 1 ignored]      0xC0 (bits 1-3)
        //    UINT8 	attack      Attack rate                             0x60 (upper four bits) 	[verify this]
        //    UINT8 	sustain 	Sustain level                           0x80 (upper four bits) 	[verify this]
        //    UINT8 	eg          Envelope gain (nonzero value is on) 	0x20 (bit 5) 	[verify this]
        //    UINT8 	decay       Decay rate                              0x60 (lower four bits) 	[verify this]
        //    UINT8 	releaseRate Release rate                            0x80 (lower four bits) 	[verify this]
        //    UINT8 	totalLevel 	Total output level                      0x40 (bit 0-5) 	[verify this]
        //    UINT8 	am          Amplitude modulation (Tremolo)          0x20 (bit 7) 	[verify this]
        //    UINT8 	vib         Frequency Vibrato                       0x20 (bit 6) 	[verify this]
        //    UINT8 	ksr         Key scaling/envelope rate               0x20 (bit 4) 	[verify this]
        //    UINT8 	con         Connector [op 0 only, op 1 ignored] 	0xC0 (bit 0, inverted) 	0: OPL bit set to 1
        //                                                                                  other: OPL bit set to 0
        //This structure is almost identical to an individual instrument in AdLib Instrument Format,
        //but with UINT8 fields instead of UINT16LE.
    }

    //This is followed by a list of instrument names, repeated numInstruments times:

    //    UINT16LE 	index 	Index into data section Calculation: offsetInstr = offsetData + (index * sizeof(PackedTimbre))
    //    UINT8 	flags 	0 if this record is not used, else 1
    //    char[9] 	name 	Instrument name - must be NULL-terminated



    if(ver_maj == 1)//Standard bank format
    {

    }
    else
    if(ver_maj == 0)//HMI bank format
    {
        /*
        Human Machine Interfaces version ("Version 0.0")

        ! Find out more about what is altered and how this version could be worked with.

        The version of this format associated with HMP/HMI files is altered and incompatible with
        most tools that work with BNK files. Known differences include:

            The major and minor version numbers in the header are both zero.
            The flags byte in the instrument names list may have values other than 0 or 1,
        and a null flags byte might not be indicative of an unused sample.

        The header and names list otherwise appear to follow the format spec. The instrument data
        itself has not yet been inspected for differences.

        Known examples of games that include files of this version include two possibly standardized
        file names: DRUMS.BNK and MELODIC.BNK. Dark Legions also includes a BNKDRUM.BNK.
        All of the known "version 0.0" files are 5,404 bytes in length, with 128 instrument records
        reported in the header, but have differing contents.
        */

    }

    return ERR_OK;
}

int AdLibBnk::saveFile(QString filePath, FmBank &bank)
{
    return ERR_NOT_IMLEMENTED;
}
