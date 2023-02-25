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

#include "bank_editor.h"
#include "ui_bank_editor.h"
#include "operator_editor.h"
#include <cmath>

void BankEditor::on_insName_textChanged(const QString &arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    strncpy(m_curInst->name, arg1.toUtf8().data(), 32);
}

void BankEditor::on_insName_editingFinished()
{
    if(m_lock) return;
    if(!m_curInst) return;
    QString arg1 = ui->insName->text();
    strncpy(m_curInst->name, arg1.toUtf8().data(), 32);
    reloadInstrumentNames();
}

void BankEditor::on_deepTremolo_clicked(bool checked)
{
    if(m_lock) return;
    m_bank.deep_tremolo = checked;
}

void BankEditor::on_deepVibrato_clicked(bool checked)
{
    if(m_lock) return;
    m_bank.deep_vibrato = checked;
}

void BankEditor::on_volumeModel_currentIndexChanged(int index)
{
    if(m_lock) return;
    m_bank.volume_model = (uint8_t)index;
}

void BankEditor::on_volumeSlider_valueChanged(int value)
{
    m_generator->ctl_changeVolume((unsigned)value);
}

void BankEditor::on_feedback1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->feedback1 = uint8_t(arg1);
    afterChangeControlValue();
}

void BankEditor::on_am1_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection1 = FmBank::Instrument::AM;
    afterChangeControlValue();
}

void BankEditor::on_fm1_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection1 = FmBank::Instrument::FM;
    afterChangeControlValue();
}

void BankEditor::on_percMode_currentIndexChanged(int index)
{
    ui->carrier1->setEnabled(ui->melodic->isChecked() || index <= 1 || index == 2 || index == 4);
    ui->modulator1->setEnabled(ui->melodic->isChecked() || index <= 1 || index == 3 || index == 5);

    ui->op4mode->setEnabled(ui->melodic->isChecked() || index == 0);

    if(m_lock || !m_curInst)
        return;

    // Remove 4op flag when switching the rhythm mode!
    m_lock = true;
    if(ui->op4mode->isChecked() && ui->percussion->isChecked() && index > 0)
    {
        ui->op4mode->setChecked(false);
        ui->doubleVoice->setChecked(false);
        m_curInst->en_4op = false;
        m_curInst->en_pseudo4op = false;
    }
    m_lock = false;

    switch(index)
    {
    case 0:
        m_curInst->rhythm_drum_type = 0;
        break;
    default:
        m_curInst->rhythm_drum_type = uint8_t(5 + index);
        break;
    }
    afterChangeControlValue();
}

void BankEditor::on_perc_noteNum_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->percNoteNum = uint8_t(arg1);
    if(ui->percussion->isChecked())
        ui->noteToTest->setValue(arg1);
    afterChangeControlValue();
}

void BankEditor::on_fixedNote_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->is_fixed_note = checked;
    afterChangeControlValue();
}

void BankEditor::on_feedback2_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->feedback2 = uint8_t(arg1);
    afterChangeControlValue();
}

void BankEditor::on_am2_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection2 = FmBank::Instrument::AM;
    afterChangeControlValue();
}

void BankEditor::on_fm2_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection2 = FmBank::Instrument::FM;
    afterChangeControlValue();
}

void BankEditor::on_op4mode_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->en_4op = checked;
    if(!checked)
        ui->doubleVoice->setChecked(false);
    afterChangeControlValue();
}

void BankEditor::on_doubleVoice_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->en_pseudo4op = checked;
    afterChangeControlValue();
}


void BankEditor::on_secVoiceFineTune_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->fine_tune = int8_t(arg1);
    afterChangeControlValue();
}

void BankEditor::on_noteOffset1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->note_offset1 = int16_t(arg1);
    afterChangeControlValue();
}


void BankEditor::on_noteOffset2_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->note_offset2 = int16_t(arg1);
    afterChangeControlValue();
}

void BankEditor::on_velocityOffset_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->velocity_offset = int8_t(arg1);
    afterChangeControlValue();
}

void BankEditor::on_pitchBendSlider_valueChanged(int value)
{
    int bend = (int)std::lround(value * (8192.0 / 100.0));
    m_generator->ctl_pitchBend(bend);
}

void BankEditor::on_pitchBendSlider_sliderReleased()
{
    ui->pitchBendSlider->setValue(0);  // spring back to middle position
}

void BankEditor::on_holdButton_toggled(bool checked)
{
    m_generator->ctl_hold(checked);
}

void BankEditor::onOperatorChanged()
{
    if(m_lock) return;
    if(!m_curInst) return;
    static_cast<OperatorEditor *>(sender())->writeDataToInst(*m_curInst);
    afterChangeControlValue();
}

/* ***************** All ***************** */

void BankEditor::afterChangeControlValue()
{
    Q_ASSERT(m_curInst);
    if(m_curInst->is_blank)
    {
        m_curInst->is_blank = false;
        syncInstrumentBlankness();
    }
    sendPatch();
}
