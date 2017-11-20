/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef MEASURER_H
#define MEASURER_H

#include <QObject>
#include <QProgressDialog>
#include <QWidget>
#include <QQueue>
#include <QFuture>
#include "../bank.h"

class Measurer : public QObject
{
    Q_OBJECT

    QWidget *m_parentWindow;
    QProgressDialog m_progressBox;

public:
    explicit Measurer(QWidget *parent = NULL);
    ~Measurer();

    void doMeasurement(FmBank &bank);

signals:
    void updateProgress(int);
    void workCompleted();

};



#endif // MEASURER_H
