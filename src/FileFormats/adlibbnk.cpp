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

//! Enables strict validation of every parameter.
//! Or all parameters are will be filtered even data is an invalid crap
//#define STRICT_BNK

//! Enable skipping unused instruments also included in the file
//#define SKIP_UNUSED

#ifdef STRICT_BNK
//#define VERIFY_BYTE(param, byte) if( ((param)|(byte)) != (byte) ) { bank.reset(); return ERR_BADFORMAT; }
inline void VERIFY_BYTE(unsigned char &param, unsigned char mask)
{
    if( ((param)|(mask)) != (mask) )
        throw("YOUR BYTE SUCK!");
}
#else
#define VERIFY_BYTE(param, byte)
#endif

struct BNK_Head
{
    uchar verMajor;
    uchar verMinor;
    char  magic[6];
    uchar numUsed[2];
    uchar numInstruments[2];
    uchar offsetName[2];
    uchar offsetData[2];
    uchar pad[8];
} __attribute__((__packed__));

struct BNK_InsName
{
    uchar index[2];
    uchar flags;
    char  name[9];
} __attribute__((__packed__));

struct BNK_OPLRegs
{
    uchar ksl;
    uchar fmult;
    uchar feedback;
    uchar attack;
    uchar sustain;
    uchar eg;
    uchar decay;
    uchar release;
    uchar level;
    uchar am;
    uchar vib;
    uchar ksr;
    uchar con;
} __attribute__((__packed__));

struct BNK_Instrument
{
    uchar   is_percusive;
    uchar   voicenum;
BNK_OPLRegs oplModulator;
BNK_OPLRegs oplCarrier;
    uchar   modWaveSel;
    uchar   carWaveSel;
} __attribute__((__packed__));


static const char* bnk_magic = "ADLIB-";

bool AdLibBnk::detect(char *magic)
{
    return (strncmp(magic+2, bnk_magic, 6) == 0);
}

int AdLibBnk::loadFile(QString filePath, FmBank &bank, Formats &format)
{
    char magic[8]; memset(magic, 0, 8);
    format = FORMAT_ADLIB_BKN1;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return ERR_NOFILE;

    QByteArray fileData  = file.readAll();
    file.close();

    unsigned int    size  = fileData.size();
    unsigned char*  dataU = (unsigned char*)fileData.data();
    char*           dataS = (char*)fileData.data();

    bank.reset();

    if( size < 28 ) //File too small!
        return ERR_BADFORMAT;

    memcpy(magic, dataS, 8);

    if( strncmp(magic + 2, bnk_magic, 6) != 0 )
        return ERR_BADFORMAT;

#define BNK_HEAD_OFFSET 8

#define SIZEOF_NAME     12
#define SIZEOF_INST     30

    char    ver_maj = dataS[0],
            ver_min = dataS[1];

    //unsigned short  totalInsUsed = 0;
    unsigned short  totalIns = 0;
    unsigned int    offsetName = 0;
    unsigned int    offsetData = 0;

    //totalInsUsed = toUint16LE( dataU + BNK_HEAD_OFFSET + 0 );
    totalIns     = toUint16LE( dataU + BNK_HEAD_OFFSET + 2 );
    offsetName   = toUint32LE( dataU + BNK_HEAD_OFFSET + 4 );
    offsetData   = toUint32LE( dataU + BNK_HEAD_OFFSET + 8 );

    bank.Ins_Melodic_box.clear();
    bank.Ins_Percussion_box.clear();

    //offsetInstr = offsetData + (index * sizeof(PackedTimbre))
    for(unsigned int i=0; i<totalIns; i++ )
    {
        unsigned int name_address = offsetName + SIZEOF_NAME*i;
        if( name_address+SIZEOF_NAME > size )
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        //    UINT16LE 	index 	Index into data section Calculation: offsetInstr = offsetData + (index * sizeof(PackedTimbre))
        //    UINT8 	flags 	0 if this record is not used, else 1
        //    char[9] 	name 	Instrument name - must be NULL-terminated

        unsigned short ins_index   = toUint16LE( dataU + name_address );
        unsigned int   ins_address = offsetData + ins_index*SIZEOF_INST;

        #ifdef SKIP_UNUSED
        if( dataU[name_address + 2] == 0 )
            continue;
        #endif

        if( ins_address+SIZEOF_INST > size )
        {
            bank.reset();
            return ERR_BADFORMAT;
        }

        if( (ver_maj==0) && (ver_min==0))
        {
            format = FORMAT_ADLIB_BKNHMI;
        } else {
            format = FORMAT_ADLIB_BKN1;
        }

        FmBank::Instrument *ins_p = 0;
        //At this point, the current position should be the same as offsetData. The actual instrument
        //data follows, again repeated once for each instrument. The instrument data is in the following
        //format, which is almost identical to the AdLib Instrument Format except with only one byte to
        //store each field instead of two.

        //0    UINT8 	iPercussive 	0: Melodic instrument
        //                              1: Percussive instrument
        if( dataU[ins_address + 0] == 0 )
        {
            FmBank::Instrument ins = FmBank::emptyInst();
            bank.Ins_Melodic_box.push_back(ins);
            ins_p = &bank.Ins_Melodic_box.last();
            bank.Ins_Melodic = bank.Ins_Melodic_box.data();
        }
        else
        {
            FmBank::Instrument ins = FmBank::emptyInst();
            bank.Ins_Percussion_box.push_back(ins);
            ins_p = &bank.Ins_Percussion_box.last();
            bank.Ins_Percussion = bank.Ins_Percussion_box.data();
        }

        FmBank::Instrument &ins = *ins_p;

        strncpy(ins.name, dataS+name_address+3, 8);

        try
        {
            //This is followed by a list of instrument names, repeated numInstruments times:

//            if(ver_maj == 1)//Standard bank format
//            {
                ins.adlib_drum_number       = dataU[ins_address + 1];

                ins.OP[MODULATOR1].ksl      = dataU[ins_address + 2]&0x03;

                VERIFY_BYTE( dataU[ins_address+3], 0x0F );
                ins.OP[MODULATOR1].fmult    = dataU[ins_address + 3]&0x0F;

                ins.feedback1 = dataU[ins_address + 4] & 0x07;

                VERIFY_BYTE(dataU[ins_address + 5], 0x0F);
                ins.OP[MODULATOR1].attack   = dataU[ins_address + 5] & 0x0F;

                VERIFY_BYTE(dataU[ins_address + 6], 0x0F);
                ins.OP[MODULATOR1].sustain  = 0x0F - (dataU[ins_address + 6] & 0x0F);

                VERIFY_BYTE(dataU[ins_address + 7], 0x01);
                ins.OP[MODULATOR1].eg = (dataU[ins_address + 7] != 0);

                VERIFY_BYTE(dataU[ins_address + 8], 0x0F);
                ins.OP[MODULATOR1].decay    = dataU[ins_address + 8] & 0x0F;

                VERIFY_BYTE(dataU[ins_address + 9], 0x0F);
                ins.OP[MODULATOR1].release  = dataU[ins_address + 9] & 0x0F;

                VERIFY_BYTE(dataU[ins_address + 10], 0x3F);
                ins.OP[MODULATOR1].level    = 0x3F - (dataU[ins_address + 10] & 0x3F);

                VERIFY_BYTE(dataU[ins_address + 11], 0x01);
                ins.OP[MODULATOR1].am       = ((dataU[ins_address + 11]&0x01) != 0);

                VERIFY_BYTE(dataU[ins_address + 12], 0x01);
                ins.OP[MODULATOR1].vib      = ((dataU[ins_address + 12]&0x01) != 0);

                VERIFY_BYTE(dataU[ins_address + 13], 0x01);
                ins.OP[MODULATOR1].ksr      = ( dataU[ins_address + 13] != 0 );

                //VERIFY_BYTE(dataU[ins_address + 14], 0x01);
                if(ver_maj == 0)//HMI bank format
                {
                    ins.connection1             = (dataU[ins_address + 14] != 0);
                } else {
                    ins.connection1             = (dataU[ins_address + 14] == 0);
                }

                ins.OP[CARRIER1].ksl        = (dataU[ins_address + 15]&0x03);

                //VERIFY_BYTE(dataU[ins_address + 16], 0x0F);
                ins.OP[CARRIER1].fmult      = dataU[ins_address + 16]&0x0F;

                //[IGNORE THIS] ins.feedback1 |= (dataU[ins_address + 17]>>1) & 0x07;
                //VERIFY_BYTE(dataU[ins_address + 18], 0x0F);
                ins.OP[CARRIER1].attack     = dataU[ins_address + 18] & 0x0F;

                //VERIFY_BYTE(dataU[ins_address + 19], 0x0F);
                ins.OP[CARRIER1].sustain    = 0x0F - ( dataU[ins_address + 19] & 0x0F );

                ins.OP[CARRIER1].eg         = (dataU[ins_address + 20]&0x01) != 0;

                //VERIFY_BYTE(dataU[ins_address + 21], 0x0F);
                ins.OP[CARRIER1].decay      = dataU[ins_address + 21] & 0x0F;

                //VERIFY_BYTE(dataU[ins_address + 22], 0x0F);
                ins.OP[CARRIER1].release    = dataU[ins_address + 22] & 0x0F;

                //VERIFY_BYTE(dataU[ins_address + 23], 0x3F);
                ins.OP[CARRIER1].level      = 0x3F - (dataU[ins_address + 23] & 0x3F);

                ins.OP[CARRIER1].am         = ((dataU[ins_address + 24]&0x01) != 0);
                ins.OP[CARRIER1].vib        = ((dataU[ins_address + 25]&0x01) != 0);
                ins.OP[CARRIER1].ksr        = ((dataU[ins_address + 26]&0x01) != 0);

                ins.OP[MODULATOR1].waveform = dataU[ins_address + 28]&0x07;

                ins.OP[CARRIER1].waveform   = dataU[ins_address + 29]&0x07;
//            }
//            else
//            if(ver_maj == 0)//HMI bank format
//            {
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

//                unsigned char* op1 = &dataU[ins_address + 1];
//                unsigned char* op2 = &dataU[ins_address + 15];

//                ins.percNoteNum = dataU[ins_address + 1];
//                ins.OP[MODULATOR1].ksl      = (dataU[ins_address + 2]>>6)&0x03;

//                VERIFY_BYTE( op1[1], 0x0F );
//                ins.OP[MODULATOR1].fmult    = op1[1] & 0x0F;

//                ins.feedback1 |= op1[2] & 0x07;

//                VERIFY_BYTE(dataU[ins_address + 5], 0xF0);
//                ins.OP[MODULATOR1].attack   = (dataU[ins_address + 5]>>4) & 0x0F;

//                VERIFY_BYTE(dataU[ins_address + 6], 0xF0);
//                ins.OP[MODULATOR1].sustain  = 0x0F - ( (dataU[ins_address + 6]>>4) & 0x0F );
//                VERIFY_BYTE(dataU[ins_address + 7], 0x01);
//                ins.OP[MODULATOR1].eg = (dataU[ins_address + 7] != 0);

//                VERIFY_BYTE(dataU[ins_address + 8], 0x0F);
//                ins.OP[MODULATOR1].decay    = dataU[ins_address + 8] & 0x0F;

//                VERIFY_BYTE(dataU[ins_address + 9], 0x0F);
//                ins.OP[MODULATOR1].release  = dataU[ins_address + 9] & 0x0F;

//                VERIFY_BYTE(dataU[ins_address + 10], 0x3F);
//                ins.OP[MODULATOR1].level    = 0x3F - (dataU[ins_address + 10] & 0x3F);

//                //VERIFY_BYTE(dataU[ins_address + 11], 0x80);
//                ins.OP[MODULATOR1].am       = ((dataU[ins_address + 11]&0x80) != 0);
//                //VERIFY_BYTE(dataU[ins_address + 12], 0x40);
//                ins.OP[MODULATOR1].vib      = ((dataU[ins_address + 13]&0x40) != 0);
//                //VERIFY_BYTE(dataU[ins_address + 13], 0x10);
//                ins.OP[MODULATOR1].ksr      = ((dataU[ins_address + 13]&0x10) != 0);

//                //VERIFY_BYTE(dataU[ins_address + 14], 0x10);
//                ins.connection1             = ((dataU[ins_address + 14]&0x10) != 0);

//                ins.OP[CARRIER1].ksl        = (dataU[ins_address + 15]>>6)&0x03;

//                VERIFY_BYTE(dataU[ins_address + 16], 0x0F);
//                ins.OP[CARRIER1].fmult      = dataU[ins_address + 16]&0x0F;

//                //[IGNORE THIS] ins.feedback1 |= (dataU[ins_address + 17]>>1) & 0x07;
//                VERIFY_BYTE(dataU[ins_address + 18], 0xF0);
//                ins.OP[CARRIER1].attack     = (dataU[ins_address + 18]>>4) & 0x0F;

//                VERIFY_BYTE(dataU[ins_address + 19], 0xF0);
//                ins.OP[CARRIER1].sustain    = 0x0F - ( (dataU[ins_address + 19]>>4) & 0x0F );

//                ins.OP[CARRIER1].eg = ((dataU[ins_address + 20]&0x20) != 0);

//                VERIFY_BYTE(dataU[ins_address + 21], 0x0F);
//                ins.OP[CARRIER1].decay      = dataU[ins_address + 21] & 0x0F;

//                VERIFY_BYTE(dataU[ins_address + 22], 0x0F);
//                ins.OP[CARRIER1].release    = dataU[ins_address + 22] & 0x0F;

//                VERIFY_BYTE(dataU[ins_address + 23], 0x3F);
//                ins.OP[CARRIER1].level      = 0x3F - (dataU[ins_address + 23] & 0x3F);

//                ins.OP[CARRIER1].am         = ((dataU[ins_address + 24]&0x80) != 0);
//                ins.OP[CARRIER1].vib        = ((dataU[ins_address + 25]&0x40) != 0);
//                ins.OP[CARRIER1].ksr        = ((dataU[ins_address + 26]&0x10) != 0);

//                ins.OP[MODULATOR1].waveform = dataU[ins_address + 28]&0x07;
//                ins.OP[CARRIER1].waveform   = dataU[ins_address + 28]&0x07;
//            }
        }
        catch(...)
        {
            bank.reset();
            return ERR_BADFORMAT;
        }
    }
    return ERR_OK;
}

int AdLibBnk::saveFile(QString filePath, FmBank &bank, BnkType type)
{
    return ERR_NOT_IMLEMENTED;
}
