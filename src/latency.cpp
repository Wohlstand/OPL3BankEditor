/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "latency.h"
#include "bank_editor.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QSlider>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>

LatencyDialog::LatencyDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

LatencyDialog::~LatencyDialog()
{
}

double LatencyDialog::latency() const
{
    return m_ctlLatency->value();
}

void LatencyDialog::setLatency(double lat)
{
    m_ctlLatency->setValue(lat);
}

void LatencyDialog::setupUi()
{
    setWindowTitle(tr("Audio latency"));

    QVBoxLayout *vl = new QVBoxLayout;
    setLayout(vl);

    vl->addWidget(new QLabel(tr("Define the audio latency.")));

    QHBoxLayout *ctlBox = new QHBoxLayout;
    vl->addLayout(ctlBox);

    QSlider *ctlLatency = m_ctlLatency = new QSlider(Qt::Horizontal);
    ctlBox->addWidget(ctlLatency);

    ctlLatency->setMinimumWidth(300);
    ctlLatency->setRange(
        BankEditor::audioMinimumLatency, BankEditor::audioMaximumLatency);
    ctlLatency->setTickInterval(5);
    ctlLatency->setTickPosition(QSlider::TicksBelow);

    QLineEdit *ctlLatencyEdit = m_ctlLatencyEdit = new QLineEdit;
    ctlBox->addWidget(ctlLatencyEdit);

    ctlLatencyEdit->setMaximumWidth(40);
    ctlLatencyEdit->setAlignment(Qt::AlignHCenter);
    ctlLatencyEdit->setText(QString::number(ctlLatency->value()));

    ctlBox->addWidget(new QLabel("ms"));

    connect(ctlLatency, SIGNAL(valueChanged(int)), this, SLOT(onSliderValueChanged(int)));
    connect(ctlLatencyEdit, SIGNAL(editingFinished()), this, SLOT(onTextEditingFinished()));

    vl->addWidget(new QLabel(
                      tr("This setting will only become effective after restarting the software.\n"
                          "If audio glitches occur, you should reset this setting to a higher value.")));

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok);
    vl->addWidget(bbox);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));

    adjustSize();
    setFixedSize(size());
}

void LatencyDialog::onSliderValueChanged(int value)
{
    m_ctlLatencyEdit->setText(QString::number(value));
}

void LatencyDialog::onTextEditingFinished()
{
    m_ctlLatency->setValue(m_ctlLatencyEdit->text().toInt());
    m_ctlLatencyEdit->setText(QString::number(m_ctlLatency->value()));
}
