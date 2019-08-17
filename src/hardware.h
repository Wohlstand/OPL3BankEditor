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

#ifndef HARDWARE_H
#define HARDWARE_H

#include <QDialog>
#include <memory>

namespace Ui { class HardwareDialog; }

class HardwareDialog : public QDialog
{
    Q_OBJECT

public:
    HardwareDialog(QWidget *parent = nullptr);
    ~HardwareDialog();

    unsigned oplAddress() const;
    void setOplAddress(unsigned address);
    void setCanChangeOplAddress(bool can);

private:
    void setupUi();
    void updateInfoLabel();

    std::unique_ptr<Ui::HardwareDialog> m_ui;
};

#endif // HARDWARE_H
