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

#ifndef LATENCY_H
#define LATENCY_H

#include <QDialog>
class QSlider;
class QLineEdit;

class LatencyDialog : public QDialog
{
    Q_OBJECT

public:
    LatencyDialog(QWidget *parent = nullptr);
    ~LatencyDialog();

    double latency() const;
    void setLatency(double lat);

private:
    void setupUi();
    QSlider *m_ctlLatency;
    QLineEdit *m_ctlLatencyEdit;

private slots:
    void onSliderValueChanged(int value);
    void onTextEditingFinished();
};

#endif // LATENCY_H
