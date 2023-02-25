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

#include "format_smaf_importer.h"
#include "../common.h"


static const char *s_mmf_magic = "MMMD";

/* voice parameter table for MA-1 */
static const uint8_t s_table_a[16] =
{
    0, 1, 1, 1, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15
};

static const uint8_t s_table_d[16] =
{
    0, 1, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
};

static bool s_readInstrumentMA1(uint8_t *pBuf, FmBank::Instrument &inst)
{
    uint8_t  i, j, bOperatorNum = 0;
    uint8_t  bBO, bML, bVIB, bEGT, bSUS, bRR, bDR, bAR, bSL, bTL, bWAV, bFL;
    uint8_t  bSR;

    Q_UNUSED(bBO)
    Q_UNUSED(bFL)
    Q_UNUSED(bSR)
    Q_UNUSED(bWAV)

    if(pBuf[0] & 0xF8)
        return false;

//    bankNum = pBuf[1];
//    instNum = (uint8_t)(pBuf[2] & 0x7F);

    bBO = (uint8_t)(pBuf[3] & 0x03);

    inst.feedback1 = 0;
    inst.connection1 = FmBank::Instrument::FM;

    int opMap[] =
    {
        MODULATOR1,
        CARRIER1,
        MODULATOR2,
        CARRIER2
    };

    i = j = 0;
    bOperatorNum = 0;

    while(i < 5)
    {
        bML  = (uint8_t)((pBuf[4 + i] >> 5) & 0x07);
        bVIB = (uint8_t)((pBuf[4 + i] >> 4) & 0x01);
        bEGT = (uint8_t)((pBuf[4 + i] >> 3) & 0x01);
        bSUS = (uint8_t)((pBuf[4 + i] >> 2) & 0x01);
        bRR  = (uint8_t)(((pBuf[4 + i] & 0x03) << 2) | ((pBuf[5 + i] >> 6) & 0x03));
        bDR  = (uint8_t)((pBuf[5 + i] >> 2) & 0x0F);
        bAR  = (uint8_t)(((pBuf[5 + i] & 0x03) << 2) | ((pBuf[6 + i] >> 6) & 0x03));
        bSL  = (uint8_t)((pBuf[6 + i] >> 2) & 0x0F);
        bTL  = (uint8_t)(((pBuf[6 + i] & 0x03) << 4) | ((pBuf[7 + i] >> 4) & 0x0F));
        bWAV = (uint8_t)((pBuf[7 + i] >> 3) & 0x01);
        bFL  = (uint8_t)(pBuf[7 + i] & 0x07);

        if(bEGT == 0)
        {
            bSR = s_table_d[bRR];
            if(bSUS == 0)
                bRR = 8;
            else
                bRR = s_table_d[bRR];
        }
        else
        {
            bSR = 0;
            if(bSUS == 0)
                bRR = s_table_d[bRR];
            else
                bRR = 5;
        }

        if(i != 0)
            bFL = 0;

        bDR = s_table_d[bDR];
        bAR = s_table_a[bAR];

        auto &op = inst.OP[opMap[bOperatorNum]];
        op.fmult = bML;
        op.am = 0;
        op.vib = bVIB;
        op.eg = bEGT;
        op.ksr = 0;
        op.release = bRR;
        op.decay = bDR;
        op.attack = bAR;
        op.sustain = 15 - bSL;
        op.level = 63 - bTL;
        op.ksl = 0;
        op.waveform = 0;

        bOperatorNum++;
        i += 4;
        j += 7;
    }

    return true;
}


static bool s_readInstrumentMA2(uint8_t *pBuf, FmBank::Instrument &inst)
{
    uint8_t  i, j, bOperator, bOperatorNum, bSize;
    uint8_t  bLFO, bFB, bALG, bBO, bML, bVIB, bEGT, bSUS, bKSR, bRR, bDR, bAR;
    uint8_t  bSL, bTL, bKSL, bDVB, bDAM, bAM, bWS;
    uint8_t  bSR;

    Q_UNUSED(bSize)
    Q_UNUSED(bLFO)
    Q_UNUSED(bBO)
    Q_UNUSED(bSR)
    Q_UNUSED(bDVB)
    Q_UNUSED(bDAM)

    if(pBuf[0] & 0xF0)
        return false;

    bLFO = (uint8_t)((pBuf[3] >> 6) & 0x03);
    bFB  = (uint8_t)((pBuf[3] >> 3) & 0x07);
    bALG = (uint8_t)(pBuf[3] & 0x07);
    bBO  = (uint8_t)(pBuf[4] & 0x03);

//    bankNum = pBuf[1];
//    instNum = (uint8_t)(pBuf[2] & 0x7F);

    if(bALG >= 2)
    {
        bOperator = 4;
        bSize = 30;
    }
    else
    {
        bOperator = 2;
        bSize = 16;
    }

    inst.feedback1 = bFB;
    switch(bALG)
    {
    case 0:
        inst.connection1 = FmBank::Instrument::FM;
        break;
    case 1:
        inst.connection1 = FmBank::Instrument::AM;
        break;
    }

    i = j = 0;
    bOperatorNum = 0;

    int opMap[] =
    {
        MODULATOR1,
        CARRIER1,
        MODULATOR2,
        CARRIER2
    };

    while(bOperator != 0)
    {
        bML  = (uint8_t)((pBuf[5 + i] >> 4) & 0x0F);
        bVIB = (uint8_t)((pBuf[5 + i] >> 3) & 0x01);
        bEGT = (uint8_t)((pBuf[5 + i] >> 2) & 0x01);
        bSUS = (uint8_t)((pBuf[5 + i] >> 1) & 0x01);
        bKSR = (uint8_t)(pBuf[5 + i] & 0x01);
        bRR  = (uint8_t)((pBuf[6 + i] >> 4) & 0x0F);
        bDR  = (uint8_t)(pBuf[6 + i] & 0x0F);
        bAR  = (uint8_t)((pBuf[7 + i] >> 4) & 0x0F);
        bSL  = (uint8_t)(pBuf[7 + i] & 0x0F);
        bTL  = (uint8_t)((pBuf[8 + i] >> 2) & 0x3F);
        bKSL = (uint8_t)(pBuf[8 + i] & 0x03);
        bDVB = (uint8_t)((pBuf[9 + i] >> 6) & 0x03);
        bDAM = (uint8_t)((pBuf[9 + i] >> 4) & 0x03);
        bAM  = (uint8_t)((pBuf[9 + i] >> 3) & 0x01);
        bWS  = (uint8_t)(pBuf[9 + i] & 0x07);

        if(bEGT == 0)
            bSR = bRR;
        else
            bSR = 0;

        if(bSUS != 0)
            bRR = 4;

        auto &op = inst.OP[opMap[bOperatorNum]];
        op.fmult = bML;
        op.am = bAM;
        op.vib = bVIB;
        op.eg = bEGT;
        op.ksr = bKSR;
        op.release = bRR;
        op.decay = bDR;
        op.attack = bAR;
        op.sustain = 15 - bSL;
        op.level = 63 - bTL;
        op.ksl = bKSL;
        op.waveform = bWS & 0x7;

        i += 5;
        j += 7;
        bOperator --;
        bOperatorNum++;
        bFB = 0;
    }

    return 1;
}





bool SMAF_Importer::detect(const QString &, char *magic)
{
    return (strncmp(magic, s_mmf_magic, 4) == 0);
}

FfmtErrCode SMAF_Importer::loadFile(QString filePath, FmBank &bank)
{
    char        magic[4];
    uint8_t     read_buffer[4096];

    QFile file(filePath);
    memset(magic, 0, 4);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    bank.reset(1, 1);

    if(file.read(magic, 4) != 4)
        return FfmtErrCode::ERR_BADFORMAT;
    if(strncmp(magic, s_mmf_magic, 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    bank.Ins_Melodic_box.clear();
    bank.Ins_Percussion_box.clear();

    if(file.read((char *)read_buffer, 4096) <= 0)
        return FfmtErrCode::ERR_BADFORMAT;

    uint8_t *voiceOffset = nullptr;
    size_t  bufSize = 0;

    for(int i = 0 ; i < 4096; ++i)
    {
        if(memcmp(read_buffer + i, "Mtsu", 4) == 0)
        {
            bufSize = toUint32BE(read_buffer + i + 4);
            voiceOffset = read_buffer + i + 8;
            if(i + bufSize > 4096)
            {
                // Too big buffer size
                return FfmtErrCode::ERR_BADFORMAT;
            }

            break;
        }
    }

    uint8_t  bVoiceCount;
    uint8_t  *pBuf;
    bool    ret;
    size_t   index;
    int bankNum;
    int instNum;

    bVoiceCount = 0;

    index  = 0;
    pBuf   = voiceOffset;

    FmBank::Instrument ins = FmBank::emptyInst();

    while(bufSize >= (index + 21))
    {
        if((pBuf[index]   != 0xFF) || (pBuf[index + 1] != 0xF0) ||
           (pBuf[index + 2] <= 0x11) || (pBuf[index + 3] != 0x43) ||
           (pBuf[index + 4] <= 0x01) || (pBuf[index + 4] >= 0x04))
        {
            index += (pBuf[index + 2] + 3);
            continue;
        }

        ins = FmBank::emptyInst();

        uint8_t *voiceData = pBuf + index + 5;

        bankNum = 0;
        instNum = 0;

        if(!(voiceData[0] & 0xF0))
        {
            bankNum = voiceData[1];
            instNum = (uint8_t)(voiceData[2] & 0x7F);
        }

        bool isDrum = bankNum >= 128;

        if(pBuf[index + 4] == 0x02)
            ret = s_readInstrumentMA1(voiceData, ins);
        else
            ret = s_readInstrumentMA2(voiceData, ins);

        if(ret)
        {
            snprintf(ins.name, 32, "SMAF Instr. %02u:%02u", bankNum & 127, instNum & 127);
            if(isDrum)
                bank.Ins_Percussion_box.push_back(ins);
            else
                bank.Ins_Melodic_box.push_back(ins);
            bVoiceCount++;
        }

        if(bVoiceCount >= 16)
            break;;
        index += (pBuf[index + 2] + 3);
    }

    file.close();
    bank.Ins_Percussion = bank.Ins_Percussion_box.data();
    bank.Ins_Melodic    = bank.Ins_Melodic_box.data();

    return FfmtErrCode::ERR_OK;
}

int SMAF_Importer::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString SMAF_Importer::formatName() const
{
    return "Synthetic Music Mobile Application File";
}

QString SMAF_Importer::formatModuleName() const
{
    return "SMAF Importer";
}

QString SMAF_Importer::formatExtensionMask() const
{
    return "*.mmf";
}

BankFormats SMAF_Importer::formatId() const
{
    return BankFormats::FORMAT_SMAF_IMPORTER;
}
