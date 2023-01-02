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

#ifndef IMPORTER_H
#define IMPORTER_H

#include <QDialog>
#include <QListWidget>
#include <QListWidgetItem>
#include "bank.h"
#include "FileFormats/ffmt_enums.h"

namespace Ui {
class Importer;
}

class BankEditor;

class Importer : public QDialog
{
    Q_OBJECT

    friend class BankEditor;
public:
    explicit Importer(QWidget *parent = 0);
    ~Importer();

    bool openFile(QString filePath, bool isBank = true, FfmtErrCode *errp = nullptr);
    void initFileData(QString &filePath);
    void reloadInstrumentNames();
    void setCurrentInstrument(int num, bool isPerc);

    QString getInstrumentName(int instrument, bool isAuto = true, bool isPerc = false);

public slots:
    void setMelodic();
    void setDrums();

private slots:
    void on_openBank_clicked();
    void on_openInst_clicked();
    void on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *);

    void on_importAssoc_clicked();
    void on_importReplace_clicked();
    void on_doImport_clicked();

protected:
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    void changeEvent(QEvent *event);

private:
    /**
     * @brief Updates the text to display after a language change
     */
    void onLanguageChanged();

private:
    //! Currently selected instrument
    FmBank::Instrument* m_curInst;

    BankEditor  *m_main;
    FmBank  m_bank;
    Ui::Importer *ui;
    QString m_recentPath;
};

#endif // IMPORTER_H
