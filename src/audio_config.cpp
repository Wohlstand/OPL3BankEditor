/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "audio_config.h"
#include "ui_audio_config.h"
#include "bank_editor.h"
#include <QMenu>

AudioConfigDialog::AudioConfigDialog(AudioOutRt *audioOut, QWidget *parent)
    : QDialog(parent), m_audioOut(audioOut), m_ui(new Ui::AudioConfigDialog)
{
    m_ui->setupUi(this);

#if QT_VERSION >= 0x050000
    m_ui->ctlDeviceNameEdit->setPlaceholderText(tr("Default device"));
#endif

    m_ui->ctlLatency->setRange(
        BankEditor::audioMinimumLatency, BankEditor::audioMaximumLatency);
    m_ui->ctlLatencyEdit->setText(QString::number(m_ui->ctlLatency->value()));

    adjustSize();
    setFixedSize(size());
}

AudioConfigDialog::~AudioConfigDialog()
{
}

double AudioConfigDialog::latency() const
{
    return m_ui->ctlLatency->value();
}

void AudioConfigDialog::setLatency(double lat)
{
    m_ui->ctlLatency->setValue(lat);
}

QString AudioConfigDialog::deviceName() const
{
    return m_ui->ctlDeviceNameEdit->text();
}

void AudioConfigDialog::setDeviceName(const QString &deviceName)
{
    m_ui->ctlDeviceNameEdit->setText(deviceName);
}

void AudioConfigDialog::on_ctlLatency_valueChanged(int value)
{
    m_ui->ctlLatencyEdit->setText(QString::number(value));
}

void AudioConfigDialog::on_ctlLatencyEdit_editingFinished()
{
    m_ui->ctlLatency->setValue(m_ui->ctlLatencyEdit->text().toInt());
    m_ui->ctlLatencyEdit->setText(QString::number(m_ui->ctlLatency->value()));
}

void AudioConfigDialog::on_btnChooseDevice_clicked()
{
    QToolButton *button = m_ui->btnChooseDevice;
    QMenu menu;
    QAction *action;

    std::vector<std::string> devices = m_audioOut->listCompatibleDevices();

    action = menu.addAction(tr("Default device"));

    for (size_t i = 0, n = devices.size(); i < n; ++i) {
        if(i == 0)
            menu.addSeparator();
        QString portName = QString::fromStdString(devices[i]);
        action = menu.addAction(portName);
        action->setData(portName);
    }

    QAction *choice = menu.exec(button->mapToGlobal(button->rect().bottomLeft()));
    if (choice) {
        QString device = choice->data().toString();
        m_ui->ctlDeviceNameEdit->setText(device);
    }
}
