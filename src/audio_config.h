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

#ifndef AUDIO_CONFIG_H
#define AUDIO_CONFIG_H

#include <QDialog>
#include <memory>
namespace Ui { class AudioConfigDialog; }
class AudioOutRt;

class AudioConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AudioConfigDialog(AudioOutRt *audioOut, QWidget *parent = nullptr);
    ~AudioConfigDialog();

    double latency() const;
    void setLatency(double lat);

    QString deviceName() const;
    void setDeviceName(const QString &deviceName);

private:
    AudioOutRt *m_audioOut = nullptr;
    std::unique_ptr<Ui::AudioConfigDialog> m_ui;

private slots:
    void on_ctlLatency_valueChanged(int value);
    void on_ctlLatencyEdit_editingFinished();
    void on_btnChooseDevice_clicked();
};

#endif // LATENCY_H
