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

#include "format_wohlstand_opl3.h"
#include "../common.h"

#include "wopl/wopl_file.h"

static const char       *wopl3_magic = "WOPL3-BANK\0";
static const char       *wopli_magic = "WOPL3-INST\0";

static const uint16_t   latest_version = 3;

#define WOPL_INST_SIZE_V2 62
#define WOPL_INST_SIZE_V3 66
/*
Version history:
V. 1
    * first release
V. 2
    * Added bank meta-data (title, LSB and MSB MIDI keys)
V. 3
    * Added sounding delay fields into every isntrument for ADLMIDI's channel manager
*/

bool WohlstandOPL3::detect(const QString &, char *magic)
{
    return (strncmp(magic, wopl3_magic, 11) == 0);
}

bool WohlstandOPL3::detectInst(const QString &, char *magic)
{
    return (strncmp(magic, wopli_magic, 11) == 0);
}

static bool readInstrument(QFile &file, FmBank::Instrument &ins, uint16_t &version, bool hasSoundKoefficients = true)
{
    uint8_t idata[WOPL_INST_SIZE_V3];
    memset(idata, 0, WOPL_INST_SIZE_V3);
    if(version >= 3)
    {
        qint64 got = file.read(char_p(idata), WOPL_INST_SIZE_V3);
        if(hasSoundKoefficients && (got != WOPL_INST_SIZE_V3))
            return false;
        if(!hasSoundKoefficients && (got != WOPL_INST_SIZE_V2) && (got != WOPL_INST_SIZE_V3))
            return false;
    }
    else
    {
        if(file.read(char_p(idata), WOPL_INST_SIZE_V2) != WOPL_INST_SIZE_V2)
            return false;
    }
    strncpy(ins.name, char_p(idata), 32);
    ins.note_offset1 = toSint16BE(idata + 32);
    ins.note_offset2 = toSint16BE(idata + 34);
    ins.velocity_offset = int8_t(idata[36]);
    ins.fine_tune       = int8_t(idata[37]);
    ins.percNoteNum     = idata[38];
    uint8_t flags       = idata[39];
    ins.en_4op          = (flags & WOPL_Ins_4op) != 0;
    ins.en_pseudo4op    = (flags & WOPL_Ins_Pseudo4op) != 0;
    ins.is_blank        = (flags & WOPL_Ins_IsBlank) != 0;
    ins.rhythm_drum_type = 0;
    if((flags & WOPL_RhythmModeMask) != 0)
    {
        uint8_t rm = flags & WOPL_RhythmModeMask;
        switch(rm)
        {
        case WOPL_RM_BassDrum:
            ins.rhythm_drum_type = 6;
            break;
        case WOPL_RM_Snare:
            ins.rhythm_drum_type = 7;
            break;
        case WOPL_RM_TomTom:
            ins.rhythm_drum_type = 8;
            break;
        case WOPL_RM_Cymbal:
            ins.rhythm_drum_type = 9;
            break;
        case WOPL_RM_HiHat:
            ins.rhythm_drum_type = 10;
            break;
        }
    }
    ins.setFBConn1(idata[40]);
    ins.setFBConn2(idata[41]);
    for(int op = 0; op < 4; op++)
    {
        size_t off = 42 + size_t(op) * 5;
        ins.setAVEKM(op,    idata[off + 0]);
        ins.setKSLL(op,     idata[off + 1]);
        ins.setAtDec(op,    idata[off + 2]);
        ins.setSusRel(op,   idata[off + 3]);
        ins.setWaveForm(op, idata[off + 4]);
    }
    if(version >= 3 && hasSoundKoefficients)
    {
        ins.ms_sound_kon  = toUint16BE(idata + 62);
        ins.ms_sound_koff = toUint16BE(idata + 64);
    }
    return true;
}

static void cvt_WOPLI_to_FMIns(FmBank::Instrument &out, WOPLInstrument &in)
{
    strncpy(out.name, in.inst_name, 32);
    out.note_offset1 = in.note_offset1;
    out.note_offset2 = in.note_offset2;
    out.velocity_offset = in.midi_velocity_offset;
    out.fine_tune = in.second_voice_detune;
    out.percNoteNum = in.percussion_key_number;
    out.is_fixed_note = (out.percNoteNum > 0);
    out.en_4op          = (in.inst_flags & WOPL_Ins_4op) != 0;
    out.en_pseudo4op    = (in.inst_flags & WOPL_Ins_Pseudo4op) != 0;
    out.is_blank        = (in.inst_flags & WOPL_Ins_IsBlank) != 0;
    out.rhythm_drum_type = 0;
    out.setFBConn1(in.fb_conn1_C0);
    out.setFBConn2(in.fb_conn2_C0);
    out.ms_sound_kon = in.delay_on_ms;
    out.ms_sound_koff = in.delay_off_ms;
    for(int k = 0; k < 4; k++)
    {
        out.setAVEKM(k, in.operators[k].avekf_20);
        out.setKSLL(k, in.operators[k].ksl_l_40);
        out.setAtDec(k, in.operators[k].atdec_60);
        out.setSusRel(k, in.operators[k].susrel_80);
        out.setWaveForm(k, in.operators[k].waveform_E0);
    }

    //Set rythm mode flag
    if((in.inst_flags & WOPL_RhythmModeMask) != 0)
    {
        uint8_t rm = in.inst_flags & WOPL_RhythmModeMask;
        switch(rm)
        {
        case WOPL_RM_BassDrum:
            out.rhythm_drum_type = 6;
            break;
        case WOPL_RM_Snare:
            out.rhythm_drum_type = 7;
            break;
        case WOPL_RM_TomTom:
            out.rhythm_drum_type = 8;
            break;
        case WOPL_RM_Cymbal:
            out.rhythm_drum_type = 9;
            break;
        case WOPL_RM_HiHat:
            out.rhythm_drum_type = 10;
            break;
        }
    }
}

static void cvt_FMIns_to_WOPLI(FmBank::Instrument &in, WOPLInstrument &out, bool isDrum = false)
{
    strncpy(out.inst_name, in.name, 32);
    out.note_offset1 = in.note_offset1;
    out.note_offset2 = in.note_offset2;
    out.midi_velocity_offset = in.velocity_offset;
    out.second_voice_detune = in.fine_tune;
    out.percussion_key_number = (in.is_fixed_note || isDrum) ? in.percNoteNum : 0;
    out.inst_flags = (in.en_4op ? WOPL_Ins_4op : 0) |
                     (in.en_pseudo4op ? WOPL_Ins_Pseudo4op : 0) |
                     (in.is_blank ? WOPL_Ins_IsBlank : 0);
    if(in.rhythm_drum_type != 0)
    {
        switch (in.rhythm_drum_type)
        {
        case 6:
            out.inst_flags |= WOPL_RM_BassDrum;
            break;
        case 7:
            out.inst_flags |= WOPL_RM_Snare;
            break;
        case 8:
            out.inst_flags |= WOPL_RM_TomTom;
            break;
        case 9:
            out.inst_flags |= WOPL_RM_Cymbal;
            break;
        case 10:
            out.inst_flags |= WOPL_RM_HiHat;
            break;
        }
    }

    out.fb_conn1_C0 = in.getFBConn1();
    out.fb_conn2_C0 = in.getFBConn2();
    out.delay_on_ms = in.ms_sound_kon;
    out.delay_off_ms = in.ms_sound_koff;
    for(int k = 0; k < 4; k++)
    {
        out.operators[k].avekf_20 = in.getAVEKM(k);
        out.operators[k].ksl_l_40 = in.getKSLL(k);
        out.operators[k].atdec_60 = in.getAtDec(k);
        out.operators[k].susrel_80 = in.getSusRel(k);
        out.operators[k].waveform_E0 = in.getWaveForm(k);
    }
}

static bool writeInstrument(QFile &file, FmBank::Instrument &ins, bool hasSoundKoefficients = true)
{
    uint8_t odata[WOPL_INST_SIZE_V3];
    memset(odata, 0, WOPL_INST_SIZE_V3);
    strncpy(char_p(odata), ins.name, 32);       //32
    fromSint16BE(ins.note_offset1, odata + 32); //2
    fromSint16BE(ins.note_offset2, odata + 34); //2
    odata[36] = uint8_t(ins.velocity_offset); //1
    odata[37] = uint8_t(ins.fine_tune);       //1
    odata[38] = ins.percNoteNum;              //1
    odata[39] = (ins.en_4op ? WOPL_Ins_4op : 0) |
                (ins.en_pseudo4op ? WOPL_Ins_Pseudo4op : 0) |
                (ins.is_blank ? WOPL_Ins_IsBlank : 0);
    if(ins.rhythm_drum_type != 0)
    {
        switch (ins.rhythm_drum_type)
        {
        case 6:
            odata[39] |= WOPL_RM_BassDrum;
            break;
        case 7:
            odata[39] |= WOPL_RM_Snare;
            break;
        case 8:
            odata[39] |= WOPL_RM_TomTom;
            break;
        case 9:
            odata[39] |= WOPL_RM_Cymbal;
            break;
        case 10:
            odata[39] |= WOPL_RM_HiHat;
            break;
        }
    }
    odata[40] = ins.getFBConn1();             //1
    odata[41] = ins.getFBConn2();             //1
    for(int op = 0; op < 4; op++)                  //20
    {
        size_t off = 42 + size_t(op) * 5;
        odata[off + 0] = ins.getAVEKM(op);
        odata[off + 1] = ins.getKSLL(op);
        odata[off + 2] = ins.getAtDec(op);
        odata[off + 3] = ins.getSusRel(op);
        odata[off + 4] = ins.getWaveForm(op);
    }

    if(hasSoundKoefficients)
    {
        fromUint16BE(ins.ms_sound_kon,  odata + 62);
        fromUint16BE(ins.ms_sound_koff, odata + 64);
        return (file.write(char_p(odata), WOPL_INST_SIZE_V3) == WOPL_INST_SIZE_V3);
    }
    else
        return (file.write(char_p(odata), WOPL_INST_SIZE_V2) == WOPL_INST_SIZE_V2);
}

FfmtErrCode WohlstandOPL3::loadFile(QString filePath, FmBank &bank)
{
    int err = 0;
    WOPLFile *wopl = nullptr;
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;
    QByteArray fileData = file.readAll();
    file.close();

    wopl = WOPL_LoadBankFromMem((void*)fileData.data(), (size_t)fileData.size(), &err);
    if(!wopl)
    {
        switch(err)
        {
        case WOPL_ERR_BAD_MAGIC:
        case WOPL_ERR_UNEXPECTED_ENDING:
        case WOPL_ERR_INVALID_BANKS_COUNT:
            return FfmtErrCode::ERR_BADFORMAT;
        case WOPL_ERR_NEWER_VERSION:
            return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
        case WOPL_ERR_OUT_OF_MEMORY:
            return FfmtErrCode::ERR_UNKNOWN;
        default:
            return FfmtErrCode::ERR_UNKNOWN;
        }
    }

    bank.reset(wopl->banks_count_melodic, wopl->banks_count_percussion);
    bank.deep_tremolo = (wopl->opl_flags & WOPL_FLAG_DEEP_TREMOLO) != 0;
    bank.deep_vibrato = (wopl->opl_flags & WOPL_FLAG_DEEP_VIBRATO) != 0;
    bank.volume_model = wopl->volume_model;

    FmBank::Instrument *slots_ins[2] = {bank.Ins_Melodic, bank.Ins_Percussion};
    FmBank::MidiBank * slots_banks[2] =  {bank.Banks_Melodic.data(), bank.Banks_Percussion.data()};
    uint16_t slots_counts[2] = {wopl->banks_count_melodic, wopl->banks_count_percussion};
    WOPLBank *slots_src_ins[2] = { wopl->banks_melodic, wopl->banks_percussive };

    for(int ss = 0; ss < 2; ss++)
    {
        bool isDrum = (ss == 1);
        for(int i = 0; i < slots_counts[ss]; i++)
        {
            strncpy(slots_banks[ss][i].name, slots_src_ins[ss][i].bank_name, 32);
            slots_banks[ss][i].lsb = slots_src_ins[ss][i].bank_midi_lsb;
            slots_banks[ss][i].msb = slots_src_ins[ss][i].bank_midi_msb;
            for(int j = 0; j < 128; j++)
            {
                FmBank::Instrument &ins = slots_ins[ss][(size_t(i) * 128) + size_t(j)];
                WOPLInstrument &inIns = slots_src_ins[ss][i].ins[j];
                cvt_WOPLI_to_FMIns(ins, inIns);
                ins.is_fixed_note |= isDrum;
            }
        }
    }
    WOPL_Free(wopl);

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode WohlstandOPL3::saveFile(QString filePath, FmBank &bank)
{
    FmBank::Instrument null;
    memset(&null, 0, sizeof(FmBank::Instrument));

    uint16_t count_melodic_banks   = uint16_t(((bank.countMelodic() - 1)/ 128) + 1);
    uint16_t count_percusive_banks = uint16_t(((bank.countDrums() - 1)/ 128) + 1);

    WOPLFile *wopl = WOPL_Init(count_melodic_banks, count_percusive_banks);
    if(!wopl)
        return FfmtErrCode::ERR_BADFORMAT;

    wopl->opl_flags = ((uint8_t(bank.deep_tremolo) << 0) & WOPL_FLAG_DEEP_TREMOLO) |
                      ((uint8_t(bank.deep_vibrato) << 1) & WOPL_FLAG_DEEP_VIBRATO);
    wopl->volume_model = bank.volume_model;

    FmBank::Instrument *slots_src_ins[2] = {bank.Ins_Melodic, bank.Ins_Percussion};
    size_t slots_src_ins_counts[2] = {(size_t)bank.countMelodic(), (size_t)bank.countDrums()};
    FmBank::MidiBank * slots_src_banks[2] =  {bank.Banks_Melodic.data(), bank.Banks_Percussion.data()};
    uint16_t slots_counts[2] = {wopl->banks_count_melodic, wopl->banks_count_percussion};
    WOPLBank *slots_dst_ins[2] = { wopl->banks_melodic, wopl->banks_percussive };

    for(int ss = 0; ss < 2; ss++)
    {
        bool isDrum = (ss == 1);
        for(int i = 0; i < slots_counts[ss]; i++)
        {
            strncpy(slots_dst_ins[ss][i].bank_name, slots_src_banks[ss][i].name, 32);
            slots_dst_ins[ss][i].bank_midi_lsb = slots_src_banks[ss][i].lsb;
            slots_dst_ins[ss][i].bank_midi_msb = slots_src_banks[ss][i].msb;
            for(int j = 0; j < 128; j++)
            {
                size_t ins_index = (size_t(i) * 128) + size_t(j);
                if(slots_src_ins_counts[ss] < ins_index)
                    break;
                FmBank::Instrument &ins = slots_src_ins[ss][ins_index];
                WOPLInstrument &inIns = slots_dst_ins[ss][i].ins[j];
                cvt_FMIns_to_WOPLI(ins, inIns, isDrum);
            }
        }
    }
    size_t fileSize = WOPL_CalculateBankFileSize(wopl, 0);
    QByteArray outFile;
    outFile.resize((int)fileSize);
    if(WOPL_SaveBankToMem(wopl, outFile.data(), fileSize, 0, 0) != WOPL_ERR_OK)
    {
        WOPL_Free(wopl);
        return FfmtErrCode::ERR_BADFORMAT;
    }
    WOPL_Free(wopl);

    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    file.write(outFile);
    file.close();

    return FfmtErrCode::ERR_OK;
}

int WohlstandOPL3::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING | (int)FormatCaps::FORMAT_CAPS_NEEDS_MEASURE;
}

QString WohlstandOPL3::formatName() const
{
    return "Standard OPL3 bank by Wohlstand";
}

QString WohlstandOPL3::formatExtensionMask() const
{
    return "*.wopl";
}

QString WohlstandOPL3::formatDefaultExtension() const
{
    return "wopl";
}

BankFormats WohlstandOPL3::formatId() const
{
    return BankFormats::FORMAT_WOHLSTAND_OPL3;
}

FfmtErrCode WohlstandOPL3::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    char magic[32];
    memset(magic, 0, 32);
    uint16_t version = 0;
    uint8_t isDrumFlag = 0;
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;
    if(file.read(magic, 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;
    if(strncmp(magic, wopli_magic, 11) != 0)
        return FfmtErrCode::ERR_BADFORMAT;
    if(readLE(file, version) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(version > latest_version)
        return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;
    if(file.read(char_p(&isDrumFlag), 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;
    if(isDrum)
        *isDrum = bool(isDrumFlag);
    if(!readInstrument(file, inst, version, false))
        return FfmtErrCode::ERR_BADFORMAT;
    file.close();

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode WohlstandOPL3::saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum)
{
    uint8_t isDrumFlag = uint8_t(isDrum);
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;
    if(file.write(char_p(wopli_magic), 11) != 11)
        return FfmtErrCode::ERR_BADFORMAT;
    if(writeLE(file, latest_version) != 2)
        return FfmtErrCode::ERR_BADFORMAT;
    if(file.write(char_p(&isDrumFlag), 1) != 1)
        return FfmtErrCode::ERR_BADFORMAT;
    if(!writeInstrument(file, inst, false))
        return FfmtErrCode::ERR_BADFORMAT;
    file.close();
    return FfmtErrCode::ERR_OK;
}

int WohlstandOPL3::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString WohlstandOPL3::formatInstName() const
{
    return "Standard OPL3 instrument by Wohlstand";
}

QString WohlstandOPL3::formatInstExtensionMask() const
{
    return "*.opli";
}

QString WohlstandOPL3::formatInstDefaultExtension() const
{
    return "opli";
}

InstFormats WohlstandOPL3::formatInstId() const
{
    return InstFormats::FORMAT_INST_WOPL3;
}




FfmtErrCode WohlstandOPL3_GM::saveFile(QString filePath, FmBank &bank)
{
    FmBank gm_bank = bank;
    gm_bank.Ins_Melodic_box.erase(gm_bank.Ins_Melodic_box.begin() + 128,
                                  gm_bank.Ins_Melodic_box.end());
    gm_bank.Ins_Melodic = gm_bank.Ins_Melodic_box.data();
    gm_bank.Ins_Percussion_box.erase(gm_bank.Ins_Percussion_box.begin() + 128,
                                     gm_bank.Ins_Percussion_box.end());
    gm_bank.Ins_Percussion = gm_bank.Ins_Percussion_box.data();
    gm_bank.Banks_Melodic.resize(1);
    gm_bank.Banks_Percussion.resize(1);
    WohlstandOPL3 writer;
    return writer.saveFile(filePath, gm_bank);
}

int WohlstandOPL3_GM::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_SAVE |
           (int)FormatCaps::FORMAT_CAPS_NEEDS_MEASURE |
           (int)FormatCaps::FORMAT_CAPS_GM_BANK;
}

QString WohlstandOPL3_GM::formatName() const
{
    return "Standard OPL3 GM bank by Wohlstand";
}

QString WohlstandOPL3_GM::formatModuleName() const
{
    return "WOPL GeneralMidi Writer";
}

QString WohlstandOPL3_GM::formatExtensionMask() const
{
    return "*.wopl";
}

QString WohlstandOPL3_GM::formatDefaultExtension() const
{
    return "wopl";
}

BankFormats WohlstandOPL3_GM::formatId() const
{
    return BankFormats::FORMAT_WOHLSTAND_OPL3_GM;
}
