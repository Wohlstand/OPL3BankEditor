/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2019 Vitaly Novichkov <admin@wohlnet.ru>
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
#include "ui_hardware.h"

HardwareDialog::HardwareDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::HardwareDialog)
{
    setupUi();
}

HardwareDialog::~HardwareDialog()
{
}

unsigned HardwareDialog::oplAddress() const
{
    Ui::HardwareDialog &ui = *m_ui;
    return ui.ctlAddressEdit->text().toUInt(nullptr, 16);
}

void HardwareDialog::setOplAddress(unsigned address)
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.ctlAddressEdit->setText(QString::number(address, 16));
}

void HardwareDialog::setCanChangeOplAddress(bool can)
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.ctlAddressEdit->setEnabled(can);
    updateInfoLabel();
}

void HardwareDialog::setupUi()
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.setupUi(this);
    updateInfoLabel();
    adjustSize();
    setFixedSize(size());
}

void HardwareDialog::updateInfoLabel()
{
    Ui::HardwareDialog &ui = *m_ui;
    if(ui.ctlAddressEdit && ui.ctlAddressEdit->isEnabled())
        ui.infoLabel->setText(tr("Usually $388, varies depending on card."));
    else
        ui.infoLabel->setText(tr("Impossible to set the hardware address.\n"
                                 "Make sure you installed the latest OPL proxy."));
}
