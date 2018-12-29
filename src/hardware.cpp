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

#include "hardware.h"
#include "bank_editor.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QLabel>
#include <QDialogButtonBox>

HardwareDialog::HardwareDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUi();
}

HardwareDialog::~HardwareDialog()
{
}

unsigned HardwareDialog::oplAddress() const
{
    return m_ctlAddressEdit->text().toUInt(nullptr, 16);
}

void HardwareDialog::setOplAddress(unsigned address)
{
    m_ctlAddressEdit->setText(QString::number(address, 16));
}

void HardwareDialog::setCanChangeOplAddress(bool can)
{
    m_ctlAddressEdit->setEnabled(can);
    updateInfoLabel();
}

void HardwareDialog::setupUi()
{
    setWindowTitle(tr("Hardware OPL"));

    QVBoxLayout *vl = new QVBoxLayout;
    setLayout(vl);

    vl->addWidget(new QLabel(tr("Define the hardware address.")));

    QLabel *infoLabel = m_infoLabel = new QLabel;
    vl->addWidget(infoLabel);
    updateInfoLabel();

    QHBoxLayout *ctlBox = new QHBoxLayout;
    vl->addLayout(ctlBox);

    ctlBox->addWidget(new QLabel(tr("Hexadecimal address: ")));

    QLineEdit *ctlAddressEdit = m_ctlAddressEdit = new QLineEdit;
    ctlBox->addWidget(ctlAddressEdit);
    ctlAddressEdit->setInputMask("hhh");

    QDialogButtonBox *bbox = new QDialogButtonBox(QDialogButtonBox::Ok);
    vl->addWidget(bbox);
    connect(bbox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(bbox, SIGNAL(rejected()), this, SLOT(reject()));

    adjustSize();
    setFixedSize(size());
}

void HardwareDialog::updateInfoLabel()
{
    if(m_ctlAddressEdit && m_ctlAddressEdit->isEnabled())
        m_infoLabel->setText(tr("Usually $388, varies depending on card."));
    else
        m_infoLabel->setText(tr("Impossible to set the hardware address.\n"
                                "Make sure you installed the latest OPL proxy."));
}
