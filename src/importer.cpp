/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "importer.h"
#include "ui_importer.h"
#include "bank_editor.h"

#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QMimeType>
#include <QFileDialog>
#include <QMessageBox>

#include "ins_names.h"

#include "FileFormats/adlibbnk.h"
#include "FileFormats/apogeetmb.h"
#include "FileFormats/dmxopl2.h"
#include "FileFormats/junlevizion.h"
#include "FileFormats/sb_ibk.h"

#include "common.h"

Importer::Importer(QWidget *parent) :
    QDialog(parent),
    m_main(qobject_cast<BankEditor*>(parent)),
    ui(new Ui::Importer)
{
    ui->setupUi(this);

    setMelodic();
    connect(ui->melodic,    SIGNAL(clicked(bool)),  this,   SLOT(setMelodic()));
    connect(ui->percussion, SIGNAL(clicked(bool)),  this,   SLOT(setDrums()));

    connect(ui->clear, SIGNAL(clicked()), ui->instruments, SLOT(clearSelection()));
    connect(ui->selectAll, SIGNAL(clicked()), ui->instruments, SLOT(selectAll()));

    ui->doImport->setEnabled(false);
    ui->instruments->setSelectionMode(QAbstractItemView::MultiSelection);

    this->setWindowFlags( windowFlags() & ~Qt::WindowContextHelpButtonHint );
    //this->setFixedSize(this->window()->width(), this->window()->height());
}

Importer::~Importer()
{
    delete ui;
}

bool Importer::openFile(QString filePath)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;

    //Check out for Junglevision file format
    if(JunleVizion::detect(magic))
        err = JunleVizion::loadFile(filePath, m_bank);

    //Check for DMX OPL2 file format
    else if(DmxOPL2::detect(magic))
        err = DmxOPL2::loadFile(filePath, m_bank);

    //Check for Sound Blaster IBK file format
    else if(SbIBK::detect(magic))
        err = SbIBK::loadFile(filePath, m_bank);

    //Check for AdLib BNK file format
    else if(AdLibBnk::detect(magic))
        err = AdLibBnk::loadFile(filePath, m_bank);

    //Check for Apogee Sound System TMB file format
    else if(ApogeeTMB::detect(filePath))
        err = ApogeeTMB::loadFile(filePath, m_bank);

    if( err != FmBankFormatBase::ERR_OK )
    {
        QString errText;
        switch(err)
        {
        case FmBankFormatBase::ERR_BADFORMAT:
            errText = tr("bad file format"); break;
        case FmBankFormatBase::ERR_NOFILE:
            errText = tr("can't open file"); break;
        case FmBankFormatBase::ERR_NOT_IMLEMENTED:
            errText = tr("reading of this format is not implemented yet"); break;
        case FmBankFormatBase::ERR_UNSUPPORTED_FORMAT:
            errText = tr("unsupported file format"); break;
        case FmBankFormatBase::ERR_UNKNOWN:
            errText = tr("unknown error occouped"); break;
        }
        ErrMessageO(this, errText);
        return false;
    }
    else
    {
        initFileData(filePath);
        return true;
    }

    return false;
}

void Importer::setMelodic()
{
    //setDrumMode(false);
    ui->instruments->clear();
    for(int i=0; i < m_bank.countMelodic(); i++)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(m_bank.Ins_Melodic[i].name[0]!='\0' ?
                            m_bank.Ins_Melodic[i].name : getMidiInsNameM(i));
        item->setData(Qt::UserRole, i);
        item->setToolTip(QString("ID: %1").arg(i));
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        ui->instruments->addItem(item);
    }
}

void Importer::setDrums()
{
    //setDrumMode(true);
    ui->instruments->clear();
    for(int i=0; i<m_bank.countDrums(); i++)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(m_bank.Ins_Percussion[i].name[0]!='\0' ?
                            m_bank.Ins_Percussion[i].name : getMidiInsNameP(i));
        item->setData(Qt::UserRole, i);
        item->setToolTip(QString("ID: %1").arg(i));
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        ui->instruments->addItem(item);
    }
}

void Importer::initFileData(QString &filePath)
{
    m_recentPath = filePath;
    ui->doImport->setEnabled(true);

    if(!ui->instruments->selectedItems().isEmpty())
        on_instruments_currentItemChanged(ui->instruments->selectedItems().first(), NULL);
    else
        on_instruments_currentItemChanged(NULL, NULL);
    ui->openedBank->setText(filePath);
    reloadInstrumentNames();
}

void Importer::on_openBank_clicked()
{
    QString supported   = "Supported bank files (*.op3 *.op2  *.htc *.hxn *.tmb *.ibk *.bnk)";
    QString jv          = "JunleVision bank (*.op3)";
    QString dmx         = "DMX OPL-2 bank (*.op2 *.htc *.hxn)";
    QString tmb         = "Apogee Sound System timbre bank (*.tmb)";
    QString ibk         = "Sound Blaster IBK file (*.ibk)";
    QString bnk         = "AdLib Instrument Bank (*.bnk)";
    QString allFiles    = "All files (*.*)";

    QString filters =   supported+";;"
                       +jv+";;"
                       +dmx+";;"
                       +tmb+";;"
                       +ibk+";;"
                       +bnk+";;"
                       +allFiles;

    QString fileToOpen;
    fileToOpen = QFileDialog::getOpenFileName(this, "Open bank file", m_recentPath, filters);
    if(fileToOpen.isEmpty())
        return;

    openFile(fileToOpen);
}

void Importer::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasUrls())
    {
        e->acceptProposedAction();
    }
}

void Importer::dropEvent(QDropEvent *e)
{
    this->raise();
    this->setFocus(Qt::ActiveWindowFocusReason);

    foreach(const QUrl &url, e->mimeData()->urls())
    {
        const QString &fileName = url.toLocalFile();
        if(openFile(fileName))
            break; //Only first valid file!
    }
}

void Importer::on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if(!current)
    {
        //ui->curInsInfo->setText("<Not Selected>");
        m_main->m_curInst = NULL;
    } else  {
        //ui->curInsInfo->setText(QString("%1 - %2").arg(current->data(Qt::UserRole).toInt()).arg(current->text()));
        setCurrentInstrument(current->data(Qt::UserRole).toInt(), ui->percussion->isChecked() );
    }
    m_main->flushInstrument();
}

void Importer::setCurrentInstrument(int num, bool isPerc)
{
    m_main->m_curInst = isPerc ? &m_bank.Ins_Percussion[num] : &m_bank.Ins_Melodic[num];
}


void Importer::reloadInstrumentNames()
{
    QList<QListWidgetItem*> items = ui->instruments->findItems("*", Qt::MatchWildcard);
    if(ui->percussion->isChecked())
    {
        for(int i=0; i<items.size(); i++)
        {
            int index = items[i]->data(Qt::UserRole).toInt();
            items[i]->setText( m_bank.Ins_Percussion[index].name[0]!='\0' ?
                                                m_bank.Ins_Percussion[index].name :
                                                getMidiInsNameP(index) );
        }
    } else {
        for(int i=0; i<items.size(); i++)
        {
            int index = items[i]->data(Qt::UserRole).toInt();
            items[i]->setText( m_bank.Ins_Melodic[index].name[0]!='\0' ?
                                                m_bank.Ins_Melodic[index].name :
                                                getMidiInsNameM(index) );
        }
    }
}

void Importer::on_importAssoc_clicked()
{
    ui->instruments->setSelectionMode(QAbstractItemView::MultiSelection);
}

void Importer::on_importReplace_clicked()
{
    ui->instruments->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->instruments->clearSelection();
}

void Importer::on_doImport_clicked()
{
    QList<QListWidgetItem *> selected = ui->instruments->selectedItems();
    if(selected.isEmpty())
    {
        QMessageBox::warning(this, tr("Nothing to import"), tr("You have no selected instruments to import!\nPlease select something filrst!"));
        return;
    }

    if(ui->importAssoc->isChecked())
    {
        for(int i=0; i<selected.size(); i++)
        {
            int id = selected[i]->data(Qt::UserRole).toInt();
            if(ui->melodic->isChecked())
            {
                if(m_main->m_bank.Ins_Melodic_box.size()<=id)
                {
                    FmBank::Instrument nins = FmBank::emptyInst();
                    m_main->m_bank.Ins_Melodic_box.fill(nins, id-m_main->m_bank.Ins_Melodic_box.size());
                }
                m_main->m_bank.Ins_Melodic_box[id] = m_bank.Ins_Melodic_box[id];
            }
            else
            {
                if(m_main->m_bank.Ins_Percussion_box.size()<=id)
                {
                    FmBank::Instrument nins = FmBank::emptyInst();
                    m_main->m_bank.Ins_Percussion_box.fill(nins, id-m_main->m_bank.Ins_Percussion_box.size());
                }
                m_main->m_bank.Ins_Percussion_box[id] = m_bank.Ins_Percussion_box[id];
            }
        }
    }
    else
    {
        if( m_main->m_recentNum >= 0 )
        {
            int id = selected[0]->data(Qt::UserRole).toInt();
            if(ui->melodic->isChecked())
            {
                m_main->m_bank.Ins_Melodic_box[m_main->m_recentNum] = m_bank.Ins_Melodic_box[id];
            } else {
                m_main->m_bank.Ins_Percussion_box[m_main->m_recentNum] = m_bank.Ins_Percussion_box[id];
            }
        } else {
            QMessageBox::warning(this, tr("No target"), tr("No target instrument selected.\nPlease select target instrument in the main window and retry again!"));
            return;
        }
    }
    m_main->reloadInstrumentNames();
    m_main->loadInstrument();
}
