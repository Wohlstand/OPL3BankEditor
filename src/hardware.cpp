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
#include <QMenu>
#include <QAction>
#include <QDebug>
#ifdef ENABLE_HW_OPL_SERIAL_PORT
#include "opl/chips/opl_serial_port.h"
#include <QSerialPortInfo>
#endif

HardwareDialog::HardwareDialog(QWidget *parent)
    : QDialog(parent), m_ui(new Ui::HardwareDialog)
{
    setupUi();
}

HardwareDialog::~HardwareDialog()
{
}

void HardwareDialog::setSoundCardOptionsVisible(bool visible)
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.soundCardGroup->setVisible(visible);
    adjustSize();
    setFixedSize(size());
}

void HardwareDialog::setSerialPortOptionsVisible(bool visible)
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.serialPortGroup->setVisible(visible);
    adjustSize();
    setFixedSize(size());
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

QString HardwareDialog::serialPortName() const
{
    Ui::HardwareDialog &ui = *m_ui;
    return ui.serialPortEdit->text();
}

void HardwareDialog::setSerialPortName(const QString &name) const
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.serialPortEdit->setText(name);
}

unsigned HardwareDialog::serialBaudRate() const
{
    Ui::HardwareDialog &ui = *m_ui;
    return ui.serialRateChoice->itemData(ui.serialRateChoice->currentIndex()).toUInt();
}

void HardwareDialog::setSerialBaudRate(unsigned rate)
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.serialRateChoice->setCurrentIndex(ui.serialRateChoice->findData(rate));
}

unsigned HardwareDialog::serialProtocol() const
{
    Ui::HardwareDialog &ui = *m_ui;
    return ui.serialProtocolChoice->itemData(ui.serialProtocolChoice->currentIndex()).toUInt();
}

void HardwareDialog::setSerialProtocol(unsigned protocol)
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.serialProtocolChoice->setCurrentIndex(ui.serialProtocolChoice->findData(protocol));
}

void HardwareDialog::on_serialPortButton_triggered(QAction *)
{
    Ui::HardwareDialog &ui = *m_ui;

    QMenu *menu = m_serialPortAction->menu();
    menu->clear();

#ifdef ENABLE_HW_OPL_SERIAL_PORT
    for(const QSerialPortInfo &info : QSerialPortInfo::availablePorts())
    {
        QString name = info.portName();
        QAction *act = new QAction(name, menu);
        menu->addAction(act);
        act->setData(name);
        connect(act, SIGNAL(triggered()),
                this, SLOT(onSerialPortChosen()));
    }
#endif

    menu->exec(ui.serialPortButton->mapToGlobal(QPoint(0, ui.serialPortButton->height())));
}

void HardwareDialog::onSerialPortChosen()
{
    Ui::HardwareDialog &ui = *m_ui;
    QAction *act = static_cast<QAction *>(sender());
    ui.serialPortEdit->setText(act->data().toString());
}

void HardwareDialog::setupUi()
{
    Ui::HardwareDialog &ui = *m_ui;
    ui.setupUi(this);

#ifdef ENABLE_HW_OPL_SERIAL_PORT
    for(unsigned rate : QSerialPortInfo::standardBaudRates())
    {
        if (rate >= 1200 && rate <= 115200)
            ui.serialRateChoice->addItem(QString::number(rate), rate);
    }

    ui.serialProtocolChoice->addItem(
        tr("Arduino OPL2"), (unsigned)OPL_SerialPort::ProtocolArduinoOPL2);
    ui.serialProtocolChoice->addItem(
        tr("Nuke.YKT OPL3"), (unsigned)OPL_SerialPort::ProtocolNukeYktOPL3);

    QAction *serialPortAction = m_serialPortAction = new QAction(
        ui.serialPortButton->icon(), ui.serialPortButton->text(), this);
    ui.serialPortButton->setDefaultAction(serialPortAction);
    QMenu *midiInMenu = new QMenu(this);
    serialPortAction->setMenu(midiInMenu);
#endif

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
