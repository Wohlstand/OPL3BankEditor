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

#include "format_adlib_bnk.h"
#include "../common.h"
#include <QMap>
#include <QFileInfo>

//! Enables strict validation of every parameter.
//! Or all parameters are will be filtered even data is an invalid crap
//#define STRICT_BNK

//! Enable skipping unused instruments also included in the file
#define SKIP_UNUSED

#ifdef STRICT_BNK
//#define VERIFY_BYTE(param, byte) if( ((param)|(byte)) != (byte) ) { bank.reset(); return ERR_BADFORMAT; }
inline void VERIFY_BYTE(uint8_t &param, uint8_t mask)
{
    if(((param) | (mask)) != (mask))
        throw("YOUR BYTE SUCK!");
}
#else
#define VERIFY_BYTE(param, byte)
#endif

#define BNK_HEAD_OFFSET 8

#define SIZEOF_NAME     12
#define SIZEOF_INST     30

static const char *bnk_magic = "ADLIB-";
// Note: some banks are having those magics are confusing file format detector
static const char *bnk_magicAM = "AMLIB-";
static const char *bnk_magicAN = "ANLIB-";

/**
 * @brief Reader and Writer of the Apogee Sound System TMB Bank format
 */
class AdLibBnk_impl : public FmBankFormatBase
{
public:
    enum BnkType
    {
        BNK_ADLIB,
        BNK_HMI
    };
    static bool detectBank(char *magic);
    static bool detectInst(QString filePath);
    static FfmtErrCode loadBankFile(QString filePath, FmBank &bank, BankFormats &format);
    static FfmtErrCode saveBankFile(QString filePath, FmBank &bank, BnkType type, bool hmiIsDrum);
};

bool AdLibBnk_impl::detectBank(char *magic)
{
    bool ret = false;
    ret |= (strncmp(magic + 2, bnk_magic, 6) == 0);
    ret |= (strncmp(magic + 2, bnk_magicAM, 6) == 0);
    ret |= (strncmp(magic + 2, bnk_magicAN, 6) == 0);
    return ret;
}

bool AdLibBnk_impl::detectInst(QString filePath)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return false;
    qint64 fileSize = file.bytesAvailable();
    file.close();
    /*
     * Need to check both conditions, because some other files with "ins" extension are been used
     * Unfortunately, AdLib INS files has no magic number
     *
     * File size formula: 2 + (13*2) + (13*2) + 20 + 6
     */
    return hasExt(filePath, ".ins") && ((fileSize == 80) || (fileSize == 54));
}

FfmtErrCode AdLibBnk_impl::loadBankFile(QString filePath, FmBank &bank, BankFormats &format)
{
    char magic[8];
    memset(magic, 0, 8);
    format = BankFormats::FORMAT_ADLIB_BKN1;

    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    QByteArray fileData  = file.readAll();
    file.close();

    bool        isHMI = false;

    uint32_t    size  = uint32_t(fileData.size());
    uint8_t     *dataU = (uint8_t *)fileData.data();
    char        *dataS = (char *)fileData.data();

    bank.reset();

    bank.deep_tremolo = false;
    bank.deep_vibrato = false;
    bank.volume_model = FmBank::VOLUME_CMF;

    if(size < 28)   //File too small!
        return FfmtErrCode::ERR_BADFORMAT;

    memcpy(magic, dataS, 8);
    //Verify magic number
    {
        bool isMagicValid = false;
        isMagicValid |= (strncmp(magic + 2, bnk_magic, 6) == 0);
        isMagicValid |= (strncmp(magic + 2, bnk_magicAM, 6) == 0);
        isMagicValid |= (strncmp(magic + 2, bnk_magicAN, 6) == 0);
        if(!isMagicValid)
            return FfmtErrCode::ERR_BADFORMAT;
    }

    char    ver_maj = dataS[0],
            ver_min = dataS[1];

    if((ver_maj == 0) && (ver_min == 0))
    {
        isHMI = true;
        bank.volume_model = FmBank::VOLUME_HMI;
    }

    //uint16_t  totalInsUsed = 0;
    uint16_t    totalIns = 0;
    uint32_t    offsetName = 0;
    uint32_t    offsetData = 0;

    //totalInsUsed = toUint16LE( dataU + BNK_HEAD_OFFSET + 0 );
    totalIns     = toUint16LE(dataU + BNK_HEAD_OFFSET + 2);
    offsetName   = toUint32LE(dataU + BNK_HEAD_OFFSET + 4);
    offsetData   = toUint32LE(dataU + BNK_HEAD_OFFSET + 8);

    bank.Ins_Melodic_box.clear();
    bank.Ins_Percussion_box.clear();

    //offsetInstr = offsetData + (index * sizeof(PackedTimbre))
    for(uint32_t i = 0; i < totalIns; i++)
    {
        uint32_t name_address   = offsetName + SIZEOF_NAME * i;
        if(name_address + SIZEOF_NAME > size)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        //    UINT16LE  index   Index into data section Calculation: offsetInstr = offsetData + (index * sizeof(PackedTimbre))
        //    UINT8     flags   0 if this record is not used, else 1
        //    char[9]   name    Instrument name - must be NULL-terminated

        uint16_t ins_index      = toUint16LE(dataU + name_address);
        uint32_t ins_address    = offsetData + ins_index * SIZEOF_INST;
        uint8_t  ins_used       = dataU[name_address + 2];

        #ifdef SKIP_UNUSED
        if(!isHMI && (ins_used == 0))
            continue;
        #endif

        if(ins_address + SIZEOF_INST > size)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }

        if((ver_maj == 0) && (ver_min == 0))
            format = BankFormats::FORMAT_ADLIB_BKNHMI;
        else
            format = BankFormats::FORMAT_ADLIB_BKN1;

        FmBank::Instrument *ins_m = 0;
        FmBank::Instrument *ins_p = 0;
        //At this point, the current position should be the same as offsetData. The actual instrument
        //data follows, again repeated once for each instrument. The instrument data is in the following
        //format, which is almost identical to the AdLib Instrument Format except with only one byte to
        //store each field instead of two.
        if(!isHMI)
        {
            //0    UINT8    iPercussive     0: Melodic instrument
            //                              1: Percussive instrument
            if(dataU[ins_address + 0] == 0)
            {
                FmBank::Instrument ins = FmBank::emptyInst();
                bank.Ins_Melodic_box.push_back(ins);
                ins_m = &bank.Ins_Melodic_box.last();
                bank.Ins_Melodic = bank.Ins_Melodic_box.data();
            }
            else
            {
                FmBank::Instrument ins = FmBank::emptyInst();
                bank.Ins_Percussion_box.push_back(ins);
                ins_m = &bank.Ins_Percussion_box.last();
                bank.Ins_Percussion = bank.Ins_Percussion_box.data();
            }
        }
        else
        {
            //Store both melodic and percussion while loading HMI bank
            FmBank::Instrument ins = FmBank::emptyInst();
            bank.Ins_Melodic_box.push_back(ins);
            bank.Ins_Percussion_box.push_back(ins);
            bank.Ins_Melodic    = bank.Ins_Melodic_box.data();
            bank.Ins_Percussion = bank.Ins_Percussion_box.data();
            ins_m = &bank.Ins_Melodic_box.last();
            ins_p = &bank.Ins_Percussion_box.last();
        }

        FmBank::Instrument &ins = *ins_m;

        strncpy(ins.name, dataS + name_address + 3, 8);
        try
        {
            ins.rhythm_drum_type        = dataU[ins_address + 1];

            ins.OP[MODULATOR1].ksl      = dataU[ins_address + 2] & 0x03;

            VERIFY_BYTE(dataU[ins_address + 3], 0x0F);
            ins.OP[MODULATOR1].fmult    = dataU[ins_address + 3] & 0x0F;

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
            ins.OP[MODULATOR1].am       = ((dataU[ins_address + 11] & 0x01) != 0);

            VERIFY_BYTE(dataU[ins_address + 12], 0x01);
            ins.OP[MODULATOR1].vib      = ((dataU[ins_address + 12] & 0x01) != 0);

            VERIFY_BYTE(dataU[ins_address + 13], 0x01);
            ins.OP[MODULATOR1].ksr      = (dataU[ins_address + 13] != 0);

            //VERIFY_BYTE(dataU[ins_address + 14], 0x01);
            if(ver_maj == 0)//HMI bank format
                ins.connection1             = (dataU[ins_address + 14] != 0);
            else
                ins.connection1             = (dataU[ins_address + 14] == 0);

            ins.OP[CARRIER1].ksl        = (dataU[ins_address + 15] & 0x03);

            //VERIFY_BYTE(dataU[ins_address + 16], 0x0F);
            ins.OP[CARRIER1].fmult      = dataU[ins_address + 16] & 0x0F;

            //[IGNORE THIS] ins.feedback1 |= (dataU[ins_address + 17]>>1) & 0x07;
            //VERIFY_BYTE(dataU[ins_address + 18], 0x0F);
            ins.OP[CARRIER1].attack     = dataU[ins_address + 18] & 0x0F;

            //VERIFY_BYTE(dataU[ins_address + 19], 0x0F);
            ins.OP[CARRIER1].sustain    = 0x0F - (dataU[ins_address + 19] & 0x0F);

            ins.OP[CARRIER1].eg         = (dataU[ins_address + 20] & 0x01) != 0;

            //VERIFY_BYTE(dataU[ins_address + 21], 0x0F);
            ins.OP[CARRIER1].decay      = dataU[ins_address + 21] & 0x0F;

            //VERIFY_BYTE(dataU[ins_address + 22], 0x0F);
            ins.OP[CARRIER1].release    = dataU[ins_address + 22] & 0x0F;

            //VERIFY_BYTE(dataU[ins_address + 23], 0x3F);
            ins.OP[CARRIER1].level      = 0x3F - (dataU[ins_address + 23] & 0x3F);

            ins.OP[CARRIER1].am         = ((dataU[ins_address + 24] & 0x01) != 0);
            ins.OP[CARRIER1].vib        = ((dataU[ins_address + 25] & 0x01) != 0);
            ins.OP[CARRIER1].ksr        = ((dataU[ins_address + 26] & 0x01) != 0);

            ins.OP[MODULATOR1].waveform = dataU[ins_address + 28] & 0x07;

            ins.OP[CARRIER1].waveform   = dataU[ins_address + 29] & 0x07;

            if(isHMI)
            {
                //in HMI instead of "is Used" flag is the actual percussion note number
                memcpy(ins_p, ins_m, sizeof(FmBank::Instrument));
                ins_p->percNoteNum = ins_used;
            }
        }
        catch(...)
        {
            bank.reset();
            return FfmtErrCode::ERR_BADFORMAT;
        }
    }

    //Automatically create missing banks
    bank.autocreateMissingBanks();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode AdLibBnk_impl::saveBankFile(QString filePath, FmBank &bank, BnkType type, bool hmiIsDrum)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    uint8_t ver[2] = { 1, 0 };

    bool isHMI = false;

    switch(type)
    {
    case BNK_ADLIB:
        ver[0] = 1;
        ver[1] = 0;
        break;
    case BNK_HMI:
        ver[0] = 0;
        ver[1] = 0;
        isHMI = true;
        break;
    }

    //struct BNK_Head
    //{
    //    uint8_t verMajor;
    //    uint8_t verMinor;
    file.write(char_p(ver), 2);         //0..1
    //    char  magic[6];
    file.write(char_p(bnk_magic), 6);   //2..7

    uint16_t instMax = isHMI ? 128 : 65515;
    uint32_t insts   = isHMI ?
                        uint32_t(hmiIsDrum ? bank.Ins_Percussion_box.size() : bank.Ins_Melodic_box.size()) :
                        uint32_t(bank.Ins_Melodic_box.size() + bank.Ins_Percussion_box.size());
    uint16_t instsU = insts > instMax ? instMax : uint16_t(insts);
    uint16_t instsS = instsU;
    uint32_t nameAddress = 28;
    uint32_t dataAddress = 28 + SIZEOF_NAME * instsS;

    //    uint8_t numUsed[2];
    writeLE(file, instsU);          //8,9
    //    uint8_t numInstruments[2];
    writeLE(file, instsS);          //10,11
    //    uint8_t offsetName[2];
    writeLE(file, nameAddress);     //12..15
    //    uint8_t offsetData[2];
    writeLE(file, dataAddress);     //16..19
    //    uint8_t pad[8];
    char pad[8];
    memset(pad, 0, 8);
    file.write(pad, 8);             //20...27
    //} __attribute__((__packed__));

    nameAddress = uint32_t(file.pos());
    file.seek(0x0C);
    writeLE(file, nameAddress);
    file.seek(nameAddress);

    /*
        http://www.shikadi.net/moddingwiki/AdLib_Instrument_Bank_Format

        It's important that it must be sorted by instrument name
        in alphabetical order to be correctly processed by Ad Lib tools.
    */
    struct InstrName
    {
        uint16_t index = 0;
        uint8_t  flags = 0;
        char     name[9] = {0};
    };
    typedef QMap<QString, InstrName> instNameMap;
    instNameMap ins_sorted;

    for(uint16_t ins = 0; ins < instsS; ins++)
    {
        InstrName inst;
        bool isDrum = isHMI ? hmiIsDrum : (bank.Ins_Melodic_box.size() <= ins);
        FmBank::Instrument &Ins = isDrum ?
                                  bank.Ins_Percussion_box[ isHMI ? ins : (ins - bank.Ins_Melodic_box.size()) ] :
                                  bank.Ins_Melodic_box[ ins ];
        strncpy(inst.name, Ins.name, 8);
        if(inst.name[0] == '\0')
        {
            char name_tmp[33] = {0};
            snprintf(name_tmp, 32, "%c-%05u", (isDrum ? 'P' : 'M'), ins);
            name_tmp[8] = '\0';
            strncpy(inst.name, name_tmp, 8);
        }
        inst.name[8] = '\0';
        inst.index = ins;
        /*YES, IT'S "USED"! (I see no reasons to keep junk data in the file)*/ /*(char)isDrum*/
        //NOTE: in HMI it's a "Percussive note number"
        inst.flags = isHMI ? (hmiIsDrum ? clip_u8(Ins.percNoteNum + Ins.note_offset1, 1, 127) : 0) : 0x01;

        QString key_s = QString::fromLatin1(inst.name).toLower();
        QString key = key_s;
        uint64_t counter = 0;
        if(isHMI) // in HMI just store instruments as-is
        {
            writeLE(file, inst.index);
            file.write(char_p(&inst.flags), 1);
            file.write(inst.name, 9);
        }
        else //in AdLib BNK all instruments must be sorted alphabetically
        {
            while(ins_sorted.contains(key))
                key = key_s + "-" + QString::number(counter++);
            ins_sorted.insert(key, inst);
        }
    }

    if(!isHMI)
    {
        for(instNameMap::iterator it = ins_sorted.begin();
            it != ins_sorted.end();
            it++)
        {
            InstrName &inst = it.value();
            //isDrum
            //struct BNK_InsName
            //{
            //    uint8_t index[2];
            writeLE(file, inst.index);
            //    uint8_t flags;
            file.write(char_p(&inst.flags), 1);
            //    char  name[9];
            file.write(inst.name, 9);
            //} __attribute__((__packed__));
        }
    }

    dataAddress = uint32_t(file.pos());
    file.seek(0x10);
    writeLE(file, dataAddress);
    file.seek(dataAddress);

    for(uint16_t ins = 0; ins < instsS; ins++)
    {
        bool isDrum = isHMI ? hmiIsDrum : (bank.Ins_Melodic_box.size() <= ins);
        FmBank::Instrument &Ins = isDrum ?
                                  bank.Ins_Percussion_box[ isHMI ? ins : (ins - bank.Ins_Melodic_box.size()) ] :
                                  bank.Ins_Melodic_box[ ins ];

        //struct BNK_OPLRegs
        //{
        //    uint8_t ksl;
        //    uint8_t fmult;
        //    uint8_t feedback;
        //    uint8_t attack;
        //    uint8_t sustain;
        //    uint8_t eg;
        //    uint8_t decay;
        //    uint8_t release;
        //    uint8_t level;
        //    uint8_t am;
        //    uint8_t vib;
        //    uint8_t ksr;
        //    uint8_t con;
        //} __attribute__((__packed__));

        uint8_t buff;
        //struct BNK_Instrument
        //{
        if(isHMI)
        {
            buff = 0; // Two unused bytes
            file.write(char_p(&buff), 1);
            file.write(char_p(&buff), 1);
        }
        else
        {
            //    uint8_t   is_percusive;
            buff = uint8_t(isDrum);
            file.write(char_p(&buff), 1);
            //    uint8_t   voicenum;
            file.write(char_p(&Ins.rhythm_drum_type), 1);
        }
        //BNK_OPLRegs oplModulator;
        //struct BNK_OPLRegs
        //{
        //    uint8_t ksl;
        file.write(char_p(&Ins.OP[MODULATOR1].ksl), 1);
        //    uint8_t fmult;
        file.write(char_p(&Ins.OP[MODULATOR1].fmult), 1);
        //    uint8_t feedback;
        file.write(char_p(&Ins.feedback1), 1);
        //    uint8_t attack;
        file.write(char_p(&Ins.OP[MODULATOR1].attack), 1);
        //    uint8_t sustain;
        buff = 0x0F - Ins.OP[MODULATOR1].sustain;
        file.write(char_p(&buff), 1);
        //    uint8_t eg;
        buff = Ins.OP[MODULATOR1].eg;
        file.write(char_p(&buff), 1);
        //    uint8_t decay;
        file.write(char_p(&Ins.OP[MODULATOR1].decay), 1);
        //    uint8_t release;
        file.write(char_p(&Ins.OP[MODULATOR1].release), 1);
        //    uint8_t level;
        buff = 0x3F - Ins.OP[MODULATOR1].level;
        file.write(char_p(&buff), 1);
        //    uint8_t am;
        buff = Ins.OP[MODULATOR1].am;
        file.write(char_p(&buff), 1);
        //    uint8_t vib;
        buff = Ins.OP[MODULATOR1].vib;
        file.write(char_p(&buff), 1);
        //    uint8_t ksr;
        buff = Ins.OP[MODULATOR1].ksr;
        file.write(char_p(&buff), 1);
        //    uint8_t con;
        switch(type)
        {
        case BNK_ADLIB:
            buff = !Ins.connection1;
            break;
        case BNK_HMI:
            buff = Ins.connection1;
            break;
        }
        file.write(char_p(&buff), 1);
        //} __attribute__((__packed__));
        //BNK_OPLRegs oplCarrier;
        //struct BNK_OPLRegs
        //{
        //    uint8_t ksl;
        file.write(char_p(&Ins.OP[CARRIER1].ksl), 1);
        //    uint8_t fmult;
        file.write(char_p(&Ins.OP[CARRIER1].fmult), 1);
        //    uint8_t feedback;
        file.write(char_p(&Ins.feedback1), 1);
        //    uint8_t attack;
        file.write(char_p(&Ins.OP[CARRIER1].attack), 1);
        //    uint8_t sustain;
        buff = 0x0F - Ins.OP[CARRIER1].sustain;
        file.write(char_p(&buff), 1);
        //    uint8_t eg;
        buff = Ins.OP[CARRIER1].eg;
        file.write(char_p(&buff), 1);
        //    uint8_t decay;
        file.write(char_p(&Ins.OP[CARRIER1].decay), 1);
        //    uint8_t release;
        file.write(char_p(&Ins.OP[CARRIER1].release), 1);
        //    uint8_t level;
        buff = 0x3F - Ins.OP[CARRIER1].level;
        file.write(char_p(&buff), 1);
        //    uint8_t am;
        buff = Ins.OP[CARRIER1].am;
        file.write(char_p(&buff), 1);
        //    uint8_t vib;
        buff = Ins.OP[CARRIER1].vib;
        file.write(char_p(&buff), 1);
        //    uint8_t ksr;
        buff = Ins.OP[CARRIER1].ksr;
        file.write(char_p(&buff), 1);
        //    uint8_t con;
        switch(type)
        {
        case BNK_ADLIB:
            buff = !Ins.connection1;
            break;
        case BNK_HMI:
            buff = Ins.connection1;
            break;
        }
        file.write(char_p(&buff), 1);
        //} __attribute__((__packed__));
        //    uint8_t   modWaveSel;
        file.write(char_p(&Ins.OP[MODULATOR1].waveform), 1);
        //    uint8_t   carWaveSel;
        file.write(char_p(&Ins.OP[CARRIER1].waveform), 1);
        //} __attribute__((__packed__));
    }

    file.close();

    return FfmtErrCode::ERR_OK;
}



bool AdLibAndHmiBnk_reader::detect(const QString &, char *magic)
{
    return AdLibBnk_impl::detectBank(magic);
}

FfmtErrCode AdLibAndHmiBnk_reader::loadFile(QString filePath, FmBank &bank)
{
    m_recentFormat = BankFormats::FORMAT_UNKNOWN;
    return AdLibBnk_impl::loadBankFile(filePath, bank, m_recentFormat);
}

int AdLibAndHmiBnk_reader::formatCaps() const
{
    return int(FormatCaps::FORMAT_CAPS_OPEN) | int(FormatCaps::FORMAT_CAPS_IMPORT);
}

QString AdLibAndHmiBnk_reader::formatName() const
{
    return "AdLib/HMI instrument Bank";
}

QString AdLibAndHmiBnk_reader::formatModuleName() const
{
    return "AdLib and HMI bank formats reader";
}

QString AdLibAndHmiBnk_reader::formatExtensionMask() const
{
    return "*.bnk";
}

BankFormats AdLibAndHmiBnk_reader::formatId() const
{
    return m_recentFormat;
}



bool AdLibAndHmiBnk_reader::detectInst(const QString &filePath, char *)
{
    return AdLibBnk_impl::detectInst(filePath);
}

/**
 * @brief Parse operator data from INS file
 * @param inst Destinition instrument
 * @param opType Operator type (Modulator or carrier)
 * @param idata Input raw data
 * @return true if data valid, false if data contains broken or invalid data
 */
bool adlib_ins_insRawToOp(FmBank::Instrument &inst, const int opType, const uint8_t *idata)
{
    inst.OP[opType].ksl     = idata[0] & 0x03;
    inst.OP[opType].fmult   = idata[2] & 0x0F;
    if(opType == MODULATOR1)
        inst.feedback1 = idata[4] & 0x07;
    inst.OP[opType].attack  = idata[6] & 0x0F;
    inst.OP[opType].sustain = (0x0F - (idata[8] & 0x0F));
    inst.OP[opType].eg      = idata[10] & 0x01;
    inst.OP[opType].decay   = idata[12] & 0x0F;
    inst.OP[opType].release = idata[14] & 0x0F;
    inst.OP[opType].level   = (0x3F - (idata[16] & 0x3F));
    inst.OP[opType].am      = idata[18] & 0x01;
    inst.OP[opType].vib     = idata[20] & 0x01;
    inst.OP[opType].ksr     = idata[22] & 0x01;
    if(opType == MODULATOR1)
        inst.connection1    = !(idata[24] & 0x01);
    return true;
}

/**
 * @brief Generate raw INS data from instrument's operator
 * @param inst Source instrument
 * @param opType Operator type (Modulator or carrier)
 * @param odata Destinition output memory block to write
 */
void adlib_ins_opToRawIns(const FmBank::Instrument &inst, const int opType, uint8_t *odata)
{
    odata[0]  = inst.OP[opType].ksl;
    odata[2]  = inst.OP[opType].fmult;
    if(opType == MODULATOR1)
        odata[4] = inst.feedback1;
    odata[6]  = inst.OP[opType].attack & 0x0F;
    odata[8]  = (0x0F - inst.OP[opType].sustain);
    odata[10] = uint8_t(inst.OP[opType].eg) & 0x01;
    odata[12] = inst.OP[opType].decay;
    odata[14] = inst.OP[opType].release;
    odata[16] = (0x3F - (inst.OP[opType].level & 0x3F));
    odata[18] = uint8_t(inst.OP[opType].am) & 0x01;
    odata[20] = inst.OP[opType].vib;
    odata[22] = inst.OP[opType].ksr;
    if(opType == MODULATOR1)
        odata[24] = (!inst.connection1) & 0x01;
}

FfmtErrCode AdLibAndHmiBnk_reader::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *)
{
    memset(&inst, 0, sizeof(FmBank::Instrument));
    QFile file(filePath);
    uint8_t idata[80];
    memset(&idata, 0, 80);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    qint64 fileSize = file.bytesAvailable();
    if((fileSize == 80) && file.read(char_p(idata), 80) != 80)
        return FfmtErrCode::ERR_BADFORMAT;
    if((fileSize == 54) && file.read(char_p(idata), 54) != 54)
        return FfmtErrCode::ERR_BADFORMAT;

    if((idata[0] != 0) || (idata[1] != 0))
        return FfmtErrCode::ERR_BADFORMAT;

    if(!adlib_ins_insRawToOp(inst, MODULATOR1, idata + 2))
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return FfmtErrCode::ERR_BADFORMAT;
    }

    if(!adlib_ins_insRawToOp(inst, CARRIER1, idata + 28))
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return FfmtErrCode::ERR_BADFORMAT;
    }

    //By this specification: http://www.shikadi.net/moddingwiki/AdLib_Instrument_Format
    #if 0
    strncpy(inst.name, char_p(idata + 54), 19);
    if((idata[75] != 0) && (idata[77] != 0))
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return FfmtErrCode::ERR_BADFORMAT;
    }
    inst.OP[MODULATOR1].waveform = idata[74];
    inst.OP[CARRIER1].waveform = idata[76];
    #else
    //However, SBANK by Jammie O'Connel produces different:
    if((idata[55] != 0) && (idata[57] != 0))
    {
        memset(&inst, 0, sizeof(FmBank::Instrument));
        return FfmtErrCode::ERR_BADFORMAT;
    }
    inst.setWaveForm(MODULATOR1, idata[54]);
    inst.setWaveForm(CARRIER1, idata[56]);
    strncpy(inst.name, char_p(idata + 58), 19);
    #endif

    if(inst.name[0] == '\0')
    {
        QFileInfo i(filePath);
        strncpy(inst.name, i.baseName().toUtf8().data(), 32);
    }

    //bytes 78 and 79 can be ignored
    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode AdLibAndHmiBnk_reader::saveFileInst(QString filePath, FmBank::Instrument &inst, bool)
{
    QFile file(filePath);

    uint8_t odata[80];
    memset(&odata, 0, 80);

    adlib_ins_opToRawIns(inst, MODULATOR1, odata + 2);
    adlib_ins_opToRawIns(inst, CARRIER1, odata + 28);

    //By this specification: http://www.shikadi.net/moddingwiki/AdLib_Instrument_Format
    #if 0
    strncpy(char_p(odata + 54), inst.name, 19);
    odata[75] = inst.OP[MODULATOR1].waveform;
    odata[77] = inst.OP[CARRIER1].waveform;
    #else
    //By SBANK by Jammie O'Connel
    odata[54] = inst.getWaveForm(MODULATOR1);
    odata[56] = inst.getWaveForm(CARRIER1);
    strncpy(char_p(odata + 58), inst.name, 19);
    #endif

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;
    if(file.write(char_p(&odata), 80) != 80)
        return FfmtErrCode::ERR_BADFORMAT;
    file.close();

    return FfmtErrCode::ERR_OK;
}

int AdLibAndHmiBnk_reader::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString AdLibAndHmiBnk_reader::formatInstName() const
{
    return "AdLib Instrument";
}

QString AdLibAndHmiBnk_reader::formatInstModuleName() const
{
    return "AdLib Instrument (1987)";
}

QString AdLibAndHmiBnk_reader::formatInstExtensionMask() const
{
    return "*.ins";
}

QString AdLibAndHmiBnk_reader::formatInstDefaultExtension() const
{
    return "ins";
}

InstFormats AdLibAndHmiBnk_reader::formatInstId() const
{
    return InstFormats::FORMAT_INST_ADLIB_INS;
}





FfmtErrCode AdLibBnk_writer::saveFile(QString filePath, FmBank &bank)
{
    return AdLibBnk_impl::saveBankFile(filePath, bank, AdLibBnk_impl::BNK_ADLIB, false);
}

int AdLibBnk_writer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE;
}

QString AdLibBnk_writer::formatName() const
{
    return "AdLib instrument bank";
}

QString AdLibBnk_writer::formatModuleName() const
{
    return "AdLib instrument bank writer";
}

QString AdLibBnk_writer::formatExtensionMask() const
{
    return "*.bnk";
}

QString AdLibBnk_writer::formatDefaultExtension() const
{
    return "bnk";
}

BankFormats AdLibBnk_writer::formatId() const
{
    return BankFormats::FORMAT_ADLIB_BKN1;
}





FfmtErrCode HmiBnk_writer::saveFile(QString filePath, FmBank &bank)
{
    return AdLibBnk_impl::saveBankFile(filePath, bank, AdLibBnk_impl::BNK_HMI, false);
}

int HmiBnk_writer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE|
           (int)FormatCaps::FORMAT_CAPS_GM_BANK|
           (int)FormatCaps::FORMAT_CAPS_MELODIC_ONLY;
}

QString HmiBnk_writer::formatName() const
{
    return "HMI instrument bank [Melodic]";
}

QString HmiBnk_writer::formatModuleName() const
{
    return "HMI melodic bank writer";
}

QString HmiBnk_writer::formatExtensionMask() const
{
    return "*.bnk";
}

QString HmiBnk_writer::formatDefaultExtension() const
{
    return "bnk";
}

BankFormats HmiBnk_writer::formatId() const
{
    return BankFormats::FORMAT_ADLIB_BKNHMI;
}



FfmtErrCode HmiBnk_Drums_writer::saveFile(QString filePath, FmBank &bank)
{
    return AdLibBnk_impl::saveBankFile(filePath, bank, AdLibBnk_impl::BNK_HMI, true);
}

int HmiBnk_Drums_writer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE|
           (int)FormatCaps::FORMAT_CAPS_GM_BANK|
           (int)FormatCaps::FORMAT_CAPS_PERCUSSION_ONLY;
}

QString HmiBnk_Drums_writer::formatName() const
{
    return "HMI instrument bank [Percussion]";
}

QString HmiBnk_Drums_writer::formatModuleName() const
{
    return "HMI percussion bank writer";
}

QString HmiBnk_Drums_writer::formatExtensionMask() const
{
    return "*.bnk";
}

QString HmiBnk_Drums_writer::formatDefaultExtension() const
{
    return "bnk";
}

BankFormats HmiBnk_Drums_writer::formatId() const
{
    return BankFormats::FORMAT_ADLIB_BKNHMI_DRUMS;
}
