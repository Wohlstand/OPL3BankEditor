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

#include "formats_sup.h"
#include "ui_formats_sup.h"

#include "FileFormats/ffmt_factory.h"

#include <QHeaderView>
#include <QSet>
#include <QTableWidget>
#include <QTableWidgetItem>

formats_sup::formats_sup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::formats_sup)
{
    ui->setupUi(this);

    QStringList header;
    header << tr("Title") << tr("Extension") << tr("Type") << tr("Capabilities");

    ui->formats->setColumnCount(4);
    ui->formats->setHorizontalHeaderLabels(header);
    ui->formats->setColumnWidth(0, 240);
    ui->formats->setColumnWidth(1, 100);
    ui->formats->setColumnWidth(2, 60);
    ui->formats->setColumnWidth(3, 120);

    QTableWidgetItem *item;
    QList<const FmBankFormatBase *> formats = FmBankFormatFactory::allFormats();
    int row = 0;
    QSet<int> fmts_bank;
    QSet<int> fmts_inst;
    for(const FmBankFormatBase *b : formats)
    {
        if((b->formatCaps() != (int)FormatCaps::FORMAT_CAPS_NOTHING) &&
            !fmts_bank.contains((int)b->formatId()))
        {
            ui->formats->insertRow(row);
            item = new QTableWidgetItem(b->formatName());
            ui->formats->setItem(row, 0, item);
            item = new QTableWidgetItem(b->formatExtensionMask());
            ui->formats->setItem(row, 1, item);
            item = new QTableWidgetItem(tr("Bank"));
            ui->formats->setItem(row, 2, item);
            QString caps;
            if(b->formatCaps() == (int)FormatCaps::FORMAT_CAPS_EVERYTHING)
                caps.append(tr("Open, Save, Import"));
            else
            {
                if(b->formatCaps() & (int)FormatCaps::FORMAT_CAPS_OPEN)
                    caps.append("Open");
                if(b->formatCaps() & (int)FormatCaps::FORMAT_CAPS_SAVE)
                {
                    if(!caps.isEmpty()) caps.append(", ");
                    caps.append(tr("Save"));
                }
                if(b->formatCaps() & (int)FormatCaps::FORMAT_CAPS_IMPORT)
                {
                    if(!caps.isEmpty()) caps.append(", ");
                    caps.append(tr("Import"));
                }
            }
            item = new QTableWidgetItem(caps);
            ui->formats->setItem(row, 3, item);
            fmts_bank.insert((int)b->formatId());
            row++;
        }

        if((b->formatInstCaps() != (int)FormatCaps::FORMAT_CAPS_NOTHING) &&
            !fmts_inst.contains((int)b->formatInstId()))
        {
            ui->formats->insertRow(row);
            item = new QTableWidgetItem(b->formatInstName());
            ui->formats->setItem(row, 0, item);
            item = new QTableWidgetItem(b->formatInstExtensionMask());
            ui->formats->setItem(row, 1, item);
            item = new QTableWidgetItem(tr("Instrument"));
            ui->formats->setItem(row, 2, item);
            QString caps;
            if(b->formatInstCaps() == (int)FormatCaps::FORMAT_CAPS_EVERYTHING)
                caps.append(tr("Open, Save, Import"));
            else
            {
                if(b->formatInstCaps() & (int)FormatCaps::FORMAT_CAPS_OPEN)
                    caps.append("Open");
                if(b->formatInstCaps() & (int)FormatCaps::FORMAT_CAPS_SAVE)
                {
                    if(!caps.isEmpty()) caps.append(", ");
                    caps.append(tr("Save"));
                }
                if(b->formatInstCaps() & (int)FormatCaps::FORMAT_CAPS_IMPORT)
                {
                    if(!caps.isEmpty()) caps.append(", ");
                    caps.append(tr("Import"));
                }
            }
            item = new QTableWidgetItem(caps);
            ui->formats->setItem(row, 3, item);
            fmts_inst.insert((int)b->formatInstId());
            row++;
        }
    }

    ui->formats->sortByColumn(0, Qt::AscendingOrder);
    ui->formats->verticalHeader()->hide();
}

formats_sup::~formats_sup()
{
    delete ui;
}
