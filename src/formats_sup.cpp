/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

static QString caps2string(int formatCaps)
{
    QString caps;
    if(formatCaps == (int)FormatCaps::FORMAT_CAPS_NOTHING)
        caps.append(formats_sup::tr("N/A"));
    else
    if(formatCaps == (int)FormatCaps::FORMAT_CAPS_EVERYTHING)
        caps.append(formats_sup::tr("Open, Save, Import"));
    else
    {
        if(formatCaps & (int)FormatCaps::FORMAT_CAPS_OPEN)
            caps.append(formats_sup::tr("Open"));

        if(formatCaps & (int)FormatCaps::FORMAT_CAPS_SAVE)
        {
            if(!caps.isEmpty()) caps.append(", ");
            caps.append(formats_sup::tr("Save"));
        }

        if(formatCaps & (int)FormatCaps::FORMAT_CAPS_IMPORT)
        {
            if(!caps.isEmpty()) caps.append(", ");
            caps.append(formats_sup::tr("Import"));
        }
    }
    return caps;
}

static QTableWidgetItem *makeItem(const QString &label)
{
    QTableWidgetItem *item = new QTableWidgetItem(label);
    item->setToolTip(label);
    return item;
}

formats_sup::formats_sup(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::formats_sup)
{
    ui->setupUi(this);

    QStringList header;
    header << tr("File format module") << tr("Extensions") << tr("Type") << tr("Capabilities");
    QList<int>  header_widths;
    header_widths << 240 << 110 << 75 << 120;

    ui->formats->setColumnCount(4);
    ui->formats->setHorizontalHeaderLabels(header);
    int total_width = 0;
    for(int col = 0; col < header_widths.size(); col++)
    {
        const int &w = header_widths[col];
        ui->formats->setColumnWidth(col, header_widths[col]);
        total_width += w;
    }

    this->resize(total_width + 50, this->size().height());

    QList<const FmBankFormatBase *> bank_formats = FmBankFormatFactory::allBankFormats();
    QList<const FmBankFormatBase *> inst_formats = FmBankFormatFactory::allInstrumentFormats();
    int row = 0;

    for(const FmBankFormatBase *b : bank_formats)
    {
        if((b->formatCaps() != (int)FormatCaps::FORMAT_CAPS_NOTHING))
        {
            ui->formats->insertRow(row);
            ui->formats->setItem(row, 0, makeItem(b->formatModuleName()));
            ui->formats->setItem(row, 1, makeItem(b->formatExtensionMask()));
            ui->formats->setItem(row, 2, makeItem(tr("Bank")));
            ui->formats->setItem(row, 3, makeItem(caps2string(b->formatCaps())));
            row++;
        }
    }

    for(const FmBankFormatBase *b : inst_formats)
    {
        if((b->formatInstCaps() != (int)FormatCaps::FORMAT_CAPS_NOTHING))
        {
            ui->formats->insertRow(row);
            ui->formats->setItem(row, 0, makeItem(b->formatInstModuleName()));
            ui->formats->setItem(row, 1, makeItem(b->formatInstExtensionMask()));
            ui->formats->setItem(row, 2, makeItem(tr("Instrument")));
            ui->formats->setItem(row, 3, makeItem(caps2string(b->formatInstCaps())));
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
