/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

bool FlatbufferOpl3::detect(const QString &filePath, char *magic)
{
    return filePath.endsWith(".fbop3");
}

FfmtErrCode FlatbufferOpl3::loadFile(QString filePath, FmBank &bank)
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
    for (uint8_t i = 0; i < banks->Length(); i++) {
        auto bnk = banks->Get(i);
        switch (bnk->type()) {
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
    switch (opl3Bank->volumeModel()) {
        case VolumeModel_Auto:
            bank.volume_model = 0;
            break;
        case VolumeModel_Generic:
            bank.volume_model = 1;
            break;
        case VolumeModel_Native:
            bank.volume_model = 2;
            break;
        case VolumeModel_DMX:
            bank.volume_model = 3;
            break;
        case VolumeModel_Apogee:
            bank.volume_model = 4;
            break;
        case VolumeModel_Win9x:
            bank.volume_model = 5;
            break;
        default:
            bank.volume_model = 0;
            break;        
    }
    bank.volume_model = opl3Bank->volumeModel();

    uint16_t current_melodic_bank     = 0;
    uint16_t current_percusive_bank   = 0;
    FmBank::MidiBank* bankMeta = NULL;

    for(uint8_t i = 0; i < banks->Length(); i++) {
        auto bnk = banks->Get(i);
        auto bankType = bnk->type();

        switch (bankType) {
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
        uint8_t instrumentCount = instruments->Length();

        for (uint8_t j = 0; j < instrumentCount; j++) {
            auto instrument = instruments->Get(j);

            FmBank::Instrument* ins = NULL;
            switch (bankType) {
                case BankType_Melodic:
                    ins = &bank.Ins_Melodic[current_melodic_bank * 128 + instrument->program()];
                    break;
                case BankType_Percussion:
                    ins = &bank.Ins_Percussion[current_percusive_bank * 128 + instrument->percussionKey()];
                    break;
            }

            ins->fine_tune = instrument->secondVoiceTuning();
            int mode = instrument->mode();
            ins->en_pseudo4op = (mode == Mode_Pseudo);
            ins->en_4op = (mode == Mode_Pseudo) || (mode == Mode_FourOp);
            ins->percNoteNum = instrument->percussionKey();
            ins->note_offset1 = instrument->keyOffset1();
            ins->note_offset2 = instrument->keyOffset2();
            ins->velocity_offset = instrument->velocityOffset();
            ins->is_blank = instrument->blank();
            ins->ms_sound_kon = instrument->konMs();
            ins->ms_sound_koff = instrument->koffMs();
            strncpy(ins->name, instrument->name()->c_str(), 32);

            ins->setFBConn1(instrument->fb_conn1());
            ins->setFBConn2(instrument->fb_conn2());

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

        switch (bankType) {
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

FfmtErrCode FlatbufferOpl3::saveFile(QString filePath, FmBank &bank)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    return FfmtErrCode::ERR_OK;
}

int FlatbufferOpl3::formatCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_EVERYTHING;
}

QString FlatbufferOpl3::formatName() const
{
    return "FlatBuffer OPL-3 bank";
}

QString FlatbufferOpl3::formatExtensionMask() const
{
    return "*.fbop3";
}

BankFormats FlatbufferOpl3::formatId() const
{
    return BankFormats::FORMAT_FLATBUFFER_OPL3;
}
