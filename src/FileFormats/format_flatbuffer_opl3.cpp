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

#include "format_flatbuffer_opl3.h"
#include "../common.h"
#include "Opl3Bank_generated.h"

#define INTERNAL_VERSION 1

bool FlatbufferOpl3::detect(const QString& filePath, char*)
{
    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return false;

    QByteArray blob = file.readAll();
    file.close();

    return Opl3BankBufferHasIdentifier(blob.constData());
}

FfmtErrCode FlatbufferOpl3::loadFile(QString filePath, FmBank& bank)
{
    uint16_t count_melodic_banks     = 0;
    uint16_t count_percusive_banks   = 0;

    QFile file(filePath);

    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    QByteArray blob = file.readAll();
    file.close();

    // Get a pointer to the root object inside the buffer.
    auto opl3Bank = GetOpl3Bank(blob.constData());
    auto banks = opl3Bank->banks();

    // count banks types
    for(uint8_t i = 0; i < banks->Length(); i++)
    {
        auto bnk = banks->Get(i);

        switch(bnk->type())
        {
        case BankType_Melodic:
            count_melodic_banks++;
            break;

        case BankType_Percussion:
            count_percusive_banks++;
            break;

        default:
            return FfmtErrCode::ERR_BADFORMAT;
        }
    }

    bank.reset(count_melodic_banks, count_percusive_banks);

    bank.deep_vibrato = ((opl3Bank->oplTV() >> 0) & 0x01);
    bank.deep_tremolo = ((opl3Bank->oplTV() >> 1) & 0x01);
    bank.volume_model = opl3Bank->volumeModel() <= VolumeModel_MAX ? opl3Bank->volumeModel() : 0;

    uint16_t current_melodic_bank     = 0;
    uint16_t current_percusive_bank   = 0;
    FmBank::MidiBank* bankMeta = nullptr;

    for(uint8_t i = 0; i < banks->Length(); i++)
    {
        auto bnk = banks->Get(i);
        auto bankType = bnk->type();

        switch(bankType)
        {
        case BankType_Melodic:
            bankMeta = &bank.Banks_Melodic[current_melodic_bank];
            break;

        case BankType_Percussion:
            bankMeta = &bank.Banks_Percussion[current_percusive_bank];
            break;
        }

        bankMeta->lsb = bnk->bankLSB();
        bankMeta->msb = bnk->bankMSB();
        strncpy(bankMeta->name, bnk->name()->c_str(), 32);

        auto instruments = bnk->instruments();
        uint8_t instrumentCount = (uint8_t)instruments->Length();

        for(uint8_t j = 0; j < instrumentCount; j++)
        {
            auto instrument = instruments->Get(j);

            FmBank::Instrument* ins = nullptr;

            switch(bankType)
            {
            case BankType_Melodic:
                ins = &bank.Ins_Melodic[current_melodic_bank * 128 + instrument->program()];
                break;

            case BankType_Percussion:
                ins = &bank.Ins_Percussion[current_percusive_bank * 128 + instrument->program()];
                break;
            }

            int mode = instrument->mode();
            ins->en_pseudo4op = (mode == Mode_Pseudo);
            ins->en_4op = (mode == Mode_Pseudo) || (mode == Mode_FourOp);
            ins->percNoteNum = instrument->percussionKey();
            ins->velocity_offset = instrument->velocityOffset();
            ins->is_blank = false;
            ins->ms_sound_kon = instrument->konMs();
            ins->ms_sound_koff = instrument->koffMs();
            strncpy(ins->name, instrument->name()->c_str(), 32);

            ins->note_offset1 = instrument->keyOffset1();
            ins->setFBConn1(instrument->fb_conn1());

            ins->setAVEKM(MODULATOR1,    instrument->modulator1()->AVEKM());
            ins->setAtDec(MODULATOR1,    instrument->modulator1()->AtDec());
            ins->setSusRel(MODULATOR1,   instrument->modulator1()->SusRel());
            ins->setWaveForm(MODULATOR1, instrument->modulator1()->WaveForm());
            ins->setKSLL(MODULATOR1,     instrument->modulator1()->KSLL());

            ins->setAVEKM(CARRIER1,    instrument->carrier1()->AVEKM());
            ins->setAtDec(CARRIER1,    instrument->carrier1()->AtDec());
            ins->setSusRel(CARRIER1,   instrument->carrier1()->SusRel());
            ins->setWaveForm(CARRIER1, instrument->carrier1()->WaveForm());
            ins->setKSLL(CARRIER1,     instrument->carrier1()->KSLL());

            if(mode != Mode_TwoOp)
            {
                ins->fine_tune = instrument->secondVoiceTuning();
                ins->note_offset2 = instrument->keyOffset2();
                ins->setFBConn2(instrument->fb_conn2());

                ins->setAVEKM(MODULATOR2,    instrument->modulator2()->AVEKM());
                ins->setAtDec(MODULATOR2,    instrument->modulator2()->AtDec());
                ins->setSusRel(MODULATOR2,   instrument->modulator2()->SusRel());
                ins->setWaveForm(MODULATOR2, instrument->modulator2()->WaveForm());
                ins->setKSLL(MODULATOR2,     instrument->modulator2()->KSLL());

                ins->setAVEKM(CARRIER2,    instrument->carrier2()->AVEKM());
                ins->setAtDec(CARRIER2,    instrument->carrier2()->AtDec());
                ins->setSusRel(CARRIER2,   instrument->carrier2()->SusRel());
                ins->setWaveForm(CARRIER2, instrument->carrier2()->WaveForm());
                ins->setKSLL(CARRIER2,     instrument->carrier2()->KSLL());
            }
        }

        switch(bankType)
        {
        case BankType_Melodic:
            current_melodic_bank++;
            break;

        case BankType_Percussion:
            current_percusive_bank++;
            break;
        }
    }

    return FfmtErrCode::ERR_OK;
}

FfmtErrCode FlatbufferOpl3::saveFile(QString filePath, FmBank& bank)
{
    QFile file(filePath);

    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    flatbuffers::FlatBufferBuilder builder(1024);

    std::vector<flatbuffers::Offset<Bank>> banks_vector;

    // save melodics banks
    for(int i = 0; i < bank.Banks_Melodic.size(); i++)
    {
        const FmBank::MidiBank& bankMeta = bank.Banks_Melodic[i];

        std::vector<flatbuffers::Offset<Instrument>> instruments_vector;
        FmBank::Instrument* insts = &bank.Ins_Melodic[i * 128];

        for(int j = 0; j < 128; j++)
        {
            FmBank::Instrument& ins = insts[j];

            if(!ins.is_blank)
            {
                auto instrName = builder.CreateString((const char*)ins.name);
                Mode mode = ins.en_pseudo4op ? Mode_Pseudo : ins.en_4op ? Mode_FourOp : Mode_TwoOp;

                Operator modulator1 = Operator(
                                          ins.getAVEKM(MODULATOR1), ins.getKSLL(MODULATOR1),
                                          ins.getAtDec(MODULATOR1), ins.getSusRel(MODULATOR1), ins.getWaveForm(MODULATOR1));

                Operator carrier1 = Operator(
                                        ins.getAVEKM(CARRIER1), ins.getKSLL(CARRIER1),
                                        ins.getAtDec(CARRIER1), ins.getSusRel(CARRIER1), ins.getWaveForm(CARRIER1));

                if(mode == Mode_TwoOp)
                {
                    auto instrument = CreateInstrument(builder, (uint8_t)j, instrName, ins.note_offset1, 0, ins.velocity_offset, 0,
                                                       ins.percNoteNum, mode, ins.getFBConn1(), 0, &modulator1, &carrier1, nullptr, nullptr, ins.ms_sound_kon, ins.ms_sound_koff);
                    instruments_vector.push_back(instrument);
                }
                else
                {
                    Operator modulator2 = Operator(
                                              ins.getAVEKM(MODULATOR2), ins.getKSLL(MODULATOR2),
                                              ins.getAtDec(MODULATOR2), ins.getSusRel(MODULATOR2), ins.getWaveForm(MODULATOR2));

                    Operator carrier2 = Operator(
                                            ins.getAVEKM(CARRIER2), ins.getKSLL(CARRIER2),
                                            ins.getAtDec(CARRIER2), ins.getSusRel(CARRIER2), ins.getWaveForm(CARRIER2));

                    auto instrument = CreateInstrument(builder, (uint8_t)j, instrName, ins.note_offset1, ins.note_offset2,
                                                       ins.velocity_offset, ins.fine_tune,
                                                       ins.percNoteNum, mode, ins.getFBConn1(), ins.getFBConn2(),
                                                       &modulator1, &carrier1, &modulator2, &carrier2, ins.ms_sound_kon, ins.ms_sound_koff);
                    instruments_vector.push_back(instrument);
                }
            }
        }

        auto instruments = builder.CreateVector(instruments_vector);
        auto bankName = builder.CreateString((const char*)bankMeta.name);
        auto bnk = CreateBank(builder, bankName, BankType_Melodic, bankMeta.lsb, bankMeta.msb, instruments);
        banks_vector.push_back(bnk);
    }

    // save percussions banks
    for(int i = 0; i < bank.Banks_Percussion.size(); i++)
    {
        const FmBank::MidiBank& bankMeta = bank.Banks_Percussion[i];

        std::vector<flatbuffers::Offset<Instrument>> instruments_vector;
        FmBank::Instrument* insts = &bank.Ins_Percussion[i * 128];

        for(int j = 0; j < 128; j++)
        {
            FmBank::Instrument& ins = insts[j];

            if(!ins.is_blank)
            {
                auto instrName = builder.CreateString((const char*)ins.name);
                Mode mode = ins.en_pseudo4op ? Mode_Pseudo : ins.en_4op ? Mode_FourOp : Mode_TwoOp;

                Operator modulator1 = Operator(
                                          ins.getAVEKM(MODULATOR1), ins.getKSLL(MODULATOR1),
                                          ins.getAtDec(MODULATOR1), ins.getSusRel(MODULATOR1), ins.getWaveForm(MODULATOR1));

                Operator carrier1 = Operator(
                                        ins.getAVEKM(CARRIER1), ins.getKSLL(CARRIER1),
                                        ins.getAtDec(CARRIER1), ins.getSusRel(CARRIER1), ins.getWaveForm(CARRIER1));

                if(mode == Mode_TwoOp)
                {
                    auto instrument = CreateInstrument(builder, (uint8_t)j, instrName, ins.note_offset1, 0, ins.velocity_offset, 0,
                                                       ins.percNoteNum, mode, ins.getFBConn1(), 0, &modulator1, &carrier1, nullptr, nullptr, ins.ms_sound_kon, ins.ms_sound_koff);
                    instruments_vector.push_back(instrument);
                }
                else
                {
                    Operator modulator2 = Operator(
                                              ins.getAVEKM(MODULATOR2), ins.getKSLL(MODULATOR2),
                                              ins.getAtDec(MODULATOR2), ins.getSusRel(MODULATOR2), ins.getWaveForm(MODULATOR2));

                    Operator carrier2 = Operator(
                                            ins.getAVEKM(CARRIER2), ins.getKSLL(CARRIER2),
                                            ins.getAtDec(CARRIER2), ins.getSusRel(CARRIER2), ins.getWaveForm(CARRIER2));

                    auto instrument = CreateInstrument(builder, (uint8_t)j, instrName, ins.note_offset1, ins.note_offset2,
                                                       ins.velocity_offset, ins.fine_tune,
                                                       ins.percNoteNum, mode, ins.getFBConn1(), ins.getFBConn2(),
                                                       &modulator1, &carrier1, &modulator2, &carrier2, ins.ms_sound_kon, ins.ms_sound_koff);
                    instruments_vector.push_back(instrument);
                }
            }
        }

        auto instruments = builder.CreateVector(instruments_vector);
        auto bankName = builder.CreateString((const char*)bankMeta.name);
        auto bnk = CreateBank(builder, bankName, BankType_Percussion, bankMeta.lsb, bankMeta.msb, instruments);
        banks_vector.push_back(bnk);
    }

    auto banks = builder.CreateVector(banks_vector);
    uint8_t oplTV = ((uint8_t(bank.deep_vibrato) << 0) & 0x01) | ((uint8_t(bank.deep_tremolo) << 1) & 0x02);
    auto opl3Bank = CreateOpl3Bank(builder, INTERNAL_VERSION, oplTV, static_cast<VolumeModel>(bank.volume_model), banks);
    FinishOpl3BankBuffer(builder, opl3Bank);

    uint8_t* buf = builder.GetBufferPointer();
    int size = (int)builder.GetSize();

    file.write(char_p(buf), size);
    file.close();

    return FfmtErrCode::ERR_OK;
}

int FlatbufferOpl3::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING | (int)FormatCaps::FORMAT_CAPS_NEEDS_MEASURE;
}

QString FlatbufferOpl3::formatName() const
{
    return "FlatBuffer OPL-3 bank";
}

QString FlatbufferOpl3::formatExtensionMask() const
{
    return "*.fbop3";
}

QString FlatbufferOpl3::formatDefaultExtension() const
{
    return "fbop3";
}

BankFormats FlatbufferOpl3::formatId() const
{
    return BankFormats::FORMAT_FLATBUFFER_OPL3;
}
