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

#include "bank_editor.h"
#include "ui_bank_editor.h"


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


void BankEditor::on_feedback1_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->feedback1 = uchar(arg1);
    sendPatch();
}

void BankEditor::on_am1_clicked(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    if(checked)
        m_curInst->connection1 = FmBank::Instrument::AM;

    sendPatch();
}

void BankEditor::on_fm1_clicked(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    if(checked)
        m_curInst->connection1 = FmBank::Instrument::FM;

    sendPatch();
}


void BankEditor::on_percMode_currentIndexChanged(int index)
{
    if(m_lock) return;

    if(!m_curInst) return;

    switch(index)
    {
    case 0:
        m_curInst->adlib_drum_number = 0;
        break;

    default:
        m_curInst->adlib_drum_number = (5 + index);
        break;
    }

    sendPatch();
}

void BankEditor::on_perc_noteNum_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->percNoteNum = arg1;

    if(ui->percussion->isChecked())
        ui->noteToTest->setValue(arg1);

    sendPatch();
}

void BankEditor::on_feedback2_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->feedback2 = uchar(arg1);
    sendPatch();
}

void BankEditor::on_am2_clicked(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    if(checked)
        m_curInst->connection2 = FmBank::Instrument::AM;

    sendPatch();
}

void BankEditor::on_fm2_clicked(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    if(checked)
        m_curInst->connection2 = FmBank::Instrument::FM;

    sendPatch();
}

void BankEditor::on_op4mode_clicked(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->en_4op = checked;

    if(!checked)
        ui->doubleVoice->setChecked(false);

    sendPatch();
}

void BankEditor::on_doubleVoice_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->en_pseudo4op = checked;
    sendPatch();
}


void BankEditor::on_secVoiceFineTune_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->fine_tune = char(arg1);
    sendPatch();
}




void BankEditor::on_noteOffset1_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->note_offset1 = short(arg1);
    sendPatch();
}


void BankEditor::on_noteOffset2_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->note_offset2 = short(arg1);
    sendPatch();
}

void BankEditor::on_velocityOffset_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->velocity_offset = char(arg1);
    sendPatch();
}



/* ***************** Modulator 1 ***************** */

void BankEditor::on_op1_attack_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].attack = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op1_sustain_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].sustain = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op1_decay_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].decay = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op1_release_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].release = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op1_level_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].level = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op1_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].fmult = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op1_ksl_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].ksl = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op1_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].waveform = uchar(index);
    sendPatch();
}

void BankEditor::on_op1_am_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].am = checked;
    sendPatch();
}

void BankEditor::on_op1_vib_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].vib = checked;
    sendPatch();
}

void BankEditor::on_op1_eg_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].eg = checked;
    sendPatch();
}

void BankEditor::on_op1_ksr_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR1].ksr = checked;
    sendPatch();
}



/* ***************** Carrier 1 ***************** */

void BankEditor::on_op2_attack_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].attack = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op2_sustain_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].sustain = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op2_decay_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].decay = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op2_release_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].release = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op2_level_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].level = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op2_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].fmult = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op2_ksl_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].ksl = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op2_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].waveform = uchar(index);
    sendPatch();
}

void BankEditor::on_op2_am_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].am = checked;
    sendPatch();
}

void BankEditor::on_op2_vib_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].vib = checked;
    sendPatch();
}

void BankEditor::on_op2_eg_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].eg = checked;
    sendPatch();
}

void BankEditor::on_op2_ksr_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER1].ksr = checked;
    sendPatch();
}



/* ***************** Modulator 2 ***************** */

void BankEditor::on_op3_attack_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].attack = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op3_sustain_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].sustain = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op3_decay_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].decay = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op3_release_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].release = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op3_level_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].level = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op3_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].fmult = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op3_ksl_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].ksl = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op3_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].waveform = uchar(index);
    sendPatch();
}

void BankEditor::on_op3_am_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].am = checked;
    sendPatch();
}

void BankEditor::on_op3_vib_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].vib = checked;
    sendPatch();
}

void BankEditor::on_op3_eg_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].eg = checked;
    sendPatch();
}

void BankEditor::on_op3_ksr_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[MODULATOR2].ksr = checked;
    sendPatch();
}



/* ***************** Carrier 2 ***************** */

void BankEditor::on_op4_attack_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].attack = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op4_sustain_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].sustain = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op4_decay_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].decay = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op4_release_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].release = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op4_level_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].level = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op4_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].fmult = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op4_ksl_valueChanged(int arg1)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].ksl = uchar(arg1);
    sendPatch();
}

void BankEditor::on_op4_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].waveform = uchar(index);
    sendPatch();
}

void BankEditor::on_op4_am_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].am = checked;
    sendPatch();
}

void BankEditor::on_op4_vib_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].vib = checked;
    sendPatch();
}

void BankEditor::on_op4_eg_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].eg = checked;
    sendPatch();
}

void BankEditor::on_op4_ksr_toggled(bool checked)
{
    if(m_lock) return;

    if(!m_curInst) return;

    m_curInst->OP[CARRIER2].ksr = checked;
    sendPatch();
}


