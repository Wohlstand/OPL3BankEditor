/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2021 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2021 Vitaly Novichkov <admin@wohlnet.ru>
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
#include <QUrl>
#include <QMimeData>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>

#include "ins_names.h"

#include "FileFormats/ffmt_factory.h"

#include "common.h"

Importer::Importer(QWidget *parent) :
    QDialog(parent),
    m_main(qobject_cast<BankEditor * >(parent)),
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
    ui->stackedWidget->setCurrentWidget(ui->pageAssoc);
    this->setWindowFlags(windowFlags() & ~Qt::WindowContextHelpButtonHint);
    //this->setFixedSize(this->window()->width(), this->window()->height());
}

Importer::~Importer()
{
    delete ui;
}

bool Importer::openFile(QString filePath, bool isBank, FfmtErrCode *errp)
{
    FfmtErrCode err = FfmtErrCode::ERR_UNKNOWN;
    BankFormats format = BankFormats::FORMAT_UNKNOWN;
    ui->importAssoc->setEnabled(true);
    ui->importReplace->setEnabled(true);
    ui->melodic->setEnabled(true);
    ui->percussion->setEnabled(true);

    ui->instruments->clearSelection();
    ui->instruments->setCurrentItem(NULL);

    if(isBank)
        err = FmBankFormatFactory::ImportBankFile(filePath, m_bank, &format);
    else
    {
        m_bank.reset();
        m_bank.Ins_Melodic_box.clear();
        m_bank.Ins_Percussion_box.clear();
        FmBank::Instrument ins = FmBank::emptyInst();
        bool isDrum = false;
        err = FmBankFormatFactory::OpenInstrumentFile(filePath, ins, 0, &isDrum, true);
        if(err == FfmtErrCode::ERR_OK)
        {
            ui->importReplace->click();
            ui->importAssoc->setEnabled(false);
            if(isDrum)
            {
                m_bank.Ins_Percussion_box.push_back(ins);
                m_bank.Ins_Percussion = m_bank.Ins_Percussion_box.data();
                ui->percussion->setDisabled(false);
                setDrums();
                ui->melodic->setDisabled(true);
            }
            else
            {
                m_bank.Ins_Melodic_box.push_back(ins);
                m_bank.Ins_Melodic = m_bank.Ins_Melodic_box.data();
                ui->melodic->setDisabled(false);
                setMelodic();
                ui->percussion->setDisabled(true);
            }
        }
        else
            m_bank.reset();
    }

    if(err != FfmtErrCode::ERR_OK)
    {
        if(!errp)
        {
            QString errText = FileFormats::getErrorText(err);
            ErrMessageO(this, errText, isBank);
        }
        else
            *errp = err;
        return false;
    }
    if(FmBankFormatFactory::isImportOnly(format))
    {
        ui->importReplace->click();
        ui->importAssoc->setEnabled(false);
    }
    initFileData(filePath);
    return true;
}

void Importer::setMelodic()
{
    //setDrumMode(false);
    ui->instruments->clear();
    for(int i = 0; i < m_bank.countMelodic(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(m_bank.Ins_Melodic[i].name[0] != '\0' ?
                      QString::fromUtf8(m_bank.Ins_Melodic[i].name) : getInstrumentName(i, false, false));
        item->setData(Qt::UserRole, i);
        item->setToolTip(QString("ID: %1").arg(i));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->instruments->addItem(item);
    }
}

void Importer::setDrums()
{
    //setDrumMode(true);
    ui->instruments->clear();
    for(int i = 0; i < m_bank.countDrums(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(m_bank.Ins_Percussion[i].name[0] != '\0' ?
                      QString::fromUtf8(m_bank.Ins_Percussion[i].name) : getInstrumentName(i, false, true));
        item->setData(Qt::UserRole, i);
        item->setToolTip(QString("ID: %1").arg(i));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        ui->instruments->addItem(item);
    }
}

void Importer::initFileData(QString &filePath)
{
    m_recentPath = QFileInfo(filePath).absoluteDir().absolutePath();
    ui->doImport->setEnabled(true);

    if(ui->melodic->isChecked())
    {
        if(ui->instruments->count() != m_bank.countMelodic())
            setMelodic();
    }
    else
    {
        if(ui->instruments->count() != m_bank.countDrums())
            setDrums();
    }

    if(!ui->instruments->selectedItems().isEmpty())
        on_instruments_currentItemChanged(ui->instruments->selectedItems().first(), NULL);
    else
        on_instruments_currentItemChanged(NULL, NULL);

    ui->openedBank->setText(filePath);
    reloadInstrumentNames();
}

void Importer::on_openBank_clicked()
{
    QString filters = FmBankFormatFactory::getOpenFiltersList(true);
    QString fileToOpen;
    fileToOpen = QFileDialog::getOpenFileName(this, "Open bank file",
                                              m_recentPath, filters, nullptr,
                                              FILE_OPEN_DIALOG_OPTIONS);
    if(fileToOpen.isEmpty())
        return;
    openFile(fileToOpen, true);
}

void Importer::on_openInst_clicked()
{
    QString filters = FmBankFormatFactory::getInstOpenFiltersList(true);
    QString fileToOpen;
    fileToOpen = QFileDialog::getOpenFileName(this, "Open instrument file",
                                              m_recentPath, filters, nullptr,
                                              FILE_OPEN_DIALOG_OPTIONS);
    if(fileToOpen.isEmpty())
        return;
    openFile(fileToOpen, false);
}

void Importer::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
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

void Importer::keyPressEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    IRealtimeControl *m_generator = m_main->m_generator;
    if(ui->melodic->isChecked())
    {
        bool pn = false;

        switch(event->key())
        {
        case Qt::Key_Z:
            m_generator->changeNote(48);
            pn = true;
            break;
        case Qt::Key_S:
            m_generator->changeNote(49);
            pn = true;
            break;
        case Qt::Key_X:
            m_generator->changeNote(50);
            pn = true;
            break;
        case Qt::Key_D:
            m_generator->changeNote(51);
            pn = true;
            break;
        case Qt::Key_C:
            m_generator->changeNote(52);
            pn = true;
            break;
        case Qt::Key_V:
            m_generator->changeNote(53);
            pn = true;
            break;
        case Qt::Key_G:
            m_generator->changeNote(54);
            pn = true;
            break;
        case Qt::Key_B:
            m_generator->changeNote(55);
            pn = true;
            break;
        case Qt::Key_H:
            m_generator->changeNote(56);
            pn = true;
            break;
        case Qt::Key_N:
            m_generator->changeNote(57);
            pn = true;
            break;
        case Qt::Key_J:
            m_generator->changeNote(58);
            pn = true;
            break;
        case Qt::Key_M:
            m_generator->changeNote(59);
            pn = true;
            break;
        case Qt::Key_Q:
        case Qt::Key_Comma:
            m_generator->changeNote(60);
            pn = true;
            break;
        case Qt::Key_2:
        case Qt::Key_L:
            m_generator->changeNote(61);
            pn = true;
            break;
        case Qt::Key_W:
        case Qt::Key_Period:
            m_generator->changeNote(62);
            pn = true;
            break;
        case Qt::Key_3:
        case Qt::Key_Semicolon:
            m_generator->changeNote(63);
            pn = true;
            break;
        case Qt::Key_E:
        case Qt::Key_Slash:
            m_generator->changeNote(64);
            pn = true;
            break;
        case Qt::Key_R:
            m_generator->changeNote(65);
            pn = true;
            break;
        case Qt::Key_5:
            m_generator->changeNote(66);
            pn = true;
            break;
        case Qt::Key_T:
            m_generator->changeNote(67);
            pn = true;
            break;
        case Qt::Key_6:
            m_generator->changeNote(68);
            pn = true;
            break;
        case Qt::Key_Y:
            m_generator->changeNote(69);
            pn = true;
            break;
        case Qt::Key_7:
            m_generator->changeNote(70);
            pn = true;
            break;
        case Qt::Key_U:
            m_generator->changeNote(71);
            pn = true;
            break;
        case Qt::Key_I:
            m_generator->changeNote(72);
            pn = true;
            break;
        case Qt::Key_9:
            m_generator->changeNote(73);
            pn = true;
            break;
        case Qt::Key_O:
            m_generator->changeNote(74);
            pn = true;
            break;
        case Qt::Key_0:
            m_generator->changeNote(75);
            pn = true;
            break;
        case Qt::Key_P:
            m_generator->changeNote(76);
            pn = true;
            break;
        default:
            break;
        }

        if(pn)
            m_generator->ctl_playNote();
    }
    else
    {
        switch(event->key())
        {
        case Qt::Key_Space:
            m_generator->ctl_playNote();
            break;

        default:
            break;
        }
    }
    QDialog::keyPressEvent(event);
}

void Importer::keyReleaseEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    IRealtimeControl *m_generator = m_main->m_generator;

    if(ui->melodic->isChecked())
    {
        switch(event->key())
        {
        case Qt::Key_Z:
        case Qt::Key_S:
        case Qt::Key_X:
        case Qt::Key_D:
        case Qt::Key_C:
        case Qt::Key_V:
        case Qt::Key_G:
        case Qt::Key_B:
        case Qt::Key_H:
        case Qt::Key_N:
        case Qt::Key_J:
        case Qt::Key_M:
        case Qt::Key_Q:
        case Qt::Key_Comma:
        case Qt::Key_2:
        case Qt::Key_L:
        case Qt::Key_W:
        case Qt::Key_Period:
        case Qt::Key_3:
        case Qt::Key_Semicolon:
        case Qt::Key_E:
        case Qt::Key_Slash:
        case Qt::Key_R:
        case Qt::Key_5:
        case Qt::Key_T:
        case Qt::Key_6:
        case Qt::Key_Y:
        case Qt::Key_7:
        case Qt::Key_U:
        case Qt::Key_I:
        case Qt::Key_9:
        case Qt::Key_O:
        case Qt::Key_0:
        case Qt::Key_P:
            m_generator->ctl_noteOffAllChans();
            break;
        default:
            break;
        }
    }
    else
    {
        switch(event->key())
        {
        case Qt::Key_Space:
            m_generator->ctl_noteOffAllChans();
            break;
        default:
            break;
        }
    }

    QDialog::keyReleaseEvent(event);
}

void Importer::on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if(!current)
    {
        //ui->curInsInfo->setText("<Not Selected>");
        m_main->m_curInst = NULL;
    }
    else
    {
        //ui->curInsInfo->setText(QString("%1 - %2").arg(current->data(Qt::UserRole).toInt()).arg(current->text()));
        setCurrentInstrument(current->data(Qt::UserRole).toInt(), ui->percussion->isChecked());
    }

    m_main->flushInstrument();
}

void Importer::setCurrentInstrument(int num, bool isPerc)
{
    m_main->m_curInst = isPerc ? &m_bank.Ins_Percussion[num] : &m_bank.Ins_Melodic[num];
}

QString Importer::getInstrumentName(int instrument, bool isAuto, bool isPerc)
{
    int index = instrument / 128;
    QString name = tr("<Unknown>");
    if(index >= 0)
    {
        int lsb = 0; // TODO importer: ui->bank_lsb->value();
        int msb = 0; // TODO importer: ui->bank_msb->value();
        MidiProgramId pr = MidiProgramId(isAuto ? ui->percussion->isChecked() : isPerc, msb, lsb, instrument);
        unsigned spec = kMidiSpecXG|kMidiSpecGM1; // TODO importer: getSelectedMidiSpec();
        unsigned specObtained = kMidiSpecXG;
        const MidiProgram *p = getMidiProgram(pr, spec, &specObtained);
        p = p ? p : getFallbackProgram(pr, spec, &specObtained);
        name = p ? p->patchName : tr("<Reserved %1>").arg(instrument % 128);
    }
    return name;
}

void Importer::reloadInstrumentNames()
{
    QList<QListWidgetItem *> items = ui->instruments->findItems("*", Qt::MatchWildcard);

    if(ui->percussion->isChecked())
    {
        for(int i = 0; i < items.size(); i++)
        {
            int index = items[i]->data(Qt::UserRole).toInt();
            items[i]->setText(m_bank.Ins_Percussion[index].name[0] != '\0' ?
                              QString::fromUtf8(m_bank.Ins_Percussion[index].name) :
                              getInstrumentName(index, false, true));
        }
    }
    else
    {
        for(int i = 0; i < items.size(); i++)
        {
            int index = items[i]->data(Qt::UserRole).toInt();
            items[i]->setText(m_bank.Ins_Melodic[index].name[0] != '\0' ?
                              QString::fromUtf8(m_bank.Ins_Melodic[index].name) :
                              getInstrumentName(index, false, false));
        }
    }
}

void Importer::on_importAssoc_clicked()
{
    ui->instruments->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->stackedWidget->setCurrentWidget(ui->pageAssoc);
}

void Importer::on_importReplace_clicked()
{
    ui->instruments->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->instruments->clearSelection();
    ui->stackedWidget->setCurrentWidget(ui->pageReplace);
}

void Importer::on_doImport_clicked()
{
    QList<QListWidgetItem *> selected = ui->instruments->selectedItems();

    if(selected.isEmpty())
    {
        QMessageBox::warning(this, tr("Nothing to import"), tr("You have no selected instruments to import!\nPlease select something first!"));
        return;
    }

    FmBank &srcFmBank = m_bank;
    FmBank &dstFmBank = m_main->m_bank;

    bool srcPercussive = !ui->melodic->isChecked();
    bool dstPercussive = m_main->isDrumsMode();

    if(ui->importAssoc->isChecked())
    {
        if(ui->destinationMelodic->isChecked())
            dstPercussive = false;
        else if(ui->destinationPercussion->isChecked())
            dstPercussive = true;
        else
            dstPercussive = srcPercussive;

        for(QListWidgetItem *item : selected)
        {
            int id = item->data(Qt::UserRole).toInt();

            FmBank::MidiBank *srcMidiBank = &
                (srcPercussive ? srcFmBank.Banks_Percussion : srcFmBank.Banks_Melodic)
                [id / 128];
            FmBank::Instrument *srcIns = srcPercussive ?
                srcFmBank.Ins_Percussion : srcFmBank.Ins_Melodic;

            FmBank::MidiBank *dstMidiBank;
            FmBank::Instrument *dstIns;
            if(dstFmBank.createBank(srcMidiBank->msb, srcMidiBank->lsb, dstPercussive, &dstMidiBank, &dstIns))
                memcpy(dstMidiBank->name, srcMidiBank->name, sizeof(FmBank::MidiBank::name));

            dstIns[id % 128] = srcIns[id];
        }
        m_main->statusBar()->showMessage(tr("%1 instruments have been imported!").arg(selected.size()), 5000);
    }
    else
    {
        int dstId = m_main->m_recentNum;
        if(dstId >= 0)
        {
            int srcId = selected[0]->data(Qt::UserRole).toInt();

            FmBank::Instrument *srcIns = srcPercussive ?
                srcFmBank.Ins_Percussion : srcFmBank.Ins_Melodic;
            FmBank::Instrument *dstIns = dstPercussive ?
                dstFmBank.Ins_Percussion : dstFmBank.Ins_Melodic;

            dstIns[dstId] = srcIns[srcId];

            m_main->statusBar()->showMessage(tr("Instrument #%1 has been imported!").arg(srcId), 5000);
        }
        else
        {
            QMessageBox::warning(this, tr("No target"), tr("No target instrument selected.\nPlease select target instrument in the main window and retry again!"));
            return;
        }
    }

    //Drop instrument editing away from importer to don't confuse user after instrument was imported
    m_main->setCurrentInstrument(m_main->m_recentNum, m_main->m_recentPerc);

    m_main->reloadInstrumentNames();
    m_main->loadInstrument();
    m_main->sendPatch();
}

void Importer::onLanguageChanged()
{
    ui->retranslateUi(this);

    const QString &filePath = m_recentPath;
    if (!filePath.isEmpty())
        ui->openedBank->setText(filePath);
    else
        ui->openedBank->setText(tr("<No opened files>"));
}

void Importer::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        onLanguageChanged();
    QDialog::changeEvent(event);
}
