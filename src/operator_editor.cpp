/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2025 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "operator_editor.h"
#include "ui_operator_editor.h"

OperatorEditor::OperatorEditor(QWidget *parent)
    : QWidget(parent), m_ui(new Ui::OperatorEditor)
{
    Ui::OperatorEditor &ui = *m_ui;
    ui.setupUi(this);

    connect(ui.op_attack, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_decay, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_sustain, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_release, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_waveform, SIGNAL(currentIndexChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_freqmult, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_level, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_ksl, SIGNAL(valueChanged(int)), this, SIGNAL(operatorChanged()));
    connect(ui.op_vib, SIGNAL(toggled(bool)), this, SIGNAL(operatorChanged()));
    connect(ui.op_am, SIGNAL(toggled(bool)), this, SIGNAL(operatorChanged()));
    connect(ui.op_eg, SIGNAL(toggled(bool)), this, SIGNAL(operatorChanged()));
    connect(ui.op_ksr, SIGNAL(toggled(bool)), this, SIGNAL(operatorChanged()));
}

OperatorEditor::~OperatorEditor()
{
}

void OperatorEditor::onLanguageChanged()
{
    Ui::OperatorEditor &ui = *m_ui;
    ui.retranslateUi(this);
}

unsigned OperatorEditor::operatorNumber() const
{
    return m_operatorNumber;
}

void OperatorEditor::setOperatorNumber(unsigned n)
{
    m_operatorNumber = n;
}

void OperatorEditor::loadDataFromInst(const FmBank::Instrument &inst)
{
    const FmBank::Operator &op = inst.OP[m_operatorNumber];

    Ui::OperatorEditor &ui = *m_ui;
    ui.op_attack->setValue(op.attack);
    ui.op_decay->setValue(op.decay);
    ui.op_sustain->setValue(op.sustain);
    ui.op_release->setValue(op.release);
    ui.op_waveform->setCurrentIndex(op.waveform);
    ui.op_freqmult->setValue(op.fmult);
    ui.op_level->setValue(op.level);
    ui.op_ksl->setValue(op.ksl);
    ui.op_vib->setChecked(op.vib);
    ui.op_am->setChecked(op.am);
    ui.op_eg->setChecked(op.eg);
    ui.op_ksr->setChecked(op.ksr);
}

void OperatorEditor::writeDataToInst(FmBank::Instrument &inst) const
{
    FmBank::Operator &op = inst.OP[m_operatorNumber];

    const Ui::OperatorEditor &ui = *m_ui;
    op.attack = uint8_t(ui.op_attack->value());
    op.decay = uint8_t(ui.op_decay->value());
    op.sustain = uint8_t(ui.op_sustain->value());
    op.release = uint8_t(ui.op_release->value());
    op.waveform = ui.op_waveform->currentIndex();
    op.fmult = uint8_t(ui.op_freqmult->value());
    op.level = uint8_t(ui.op_level->value());
    op.ksl = ui.op_ksl->value();
    op.vib = ui.op_vib->isChecked();
    op.am = ui.op_am->isChecked();
    op.eg = ui.op_eg->isChecked();
    op.ksr = ui.op_ksr->isChecked();
}
