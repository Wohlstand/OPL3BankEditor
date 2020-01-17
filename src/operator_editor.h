/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef OPERATOR_EDITOR_H
#define OPERATOR_EDITOR_H

#include <QWidget>
#include <memory>
#include "bank.h"
namespace Ui { class OperatorEditor; }

class OperatorEditor : public QWidget
{
    Q_OBJECT

public slots:
    void onLanguageChanged();

public:
    explicit OperatorEditor(QWidget *parent = nullptr);
    ~OperatorEditor();

    unsigned operatorNumber() const;
    void setOperatorNumber(unsigned n);

    void loadDataFromInst(const FmBank::Instrument &inst);
    void writeDataToInst(FmBank::Instrument &inst) const;

signals:
    void operatorChanged();

private:
    std::unique_ptr<Ui::OperatorEditor> m_ui;
    unsigned m_operatorNumber = 0;
};

#endif // OPERATOR_EDITOR_H
