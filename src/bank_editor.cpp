/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2023 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QSettings>
#include <QUrl>
#include <QMimeData>
#include <QClipboard>
#include <QActionGroup>
#include <QtDebug>

#include "importer.h"
#include "formats_sup.h"
#include "bank_editor.h"
#include "ui_bank_editor.h"
#include "operator_editor.h"
#include "bank_comparison.h"
#include "audio_config.h"
#include "hardware.h"
#include "ins_names.h"
#include "main.h"
#if defined(ENABLE_PLOTS)
#include "delay_analysis.h"
#endif
#ifdef ENABLE_HW_OPL_PROXY // to set hardware port
#include "opl/chips/win9x_opl_proxy.h"
#endif
#ifdef ENABLE_HW_OPL_SERIAL_PORT // to set port and rate
#include "opl/chips/opl_serial_port.h"
#endif

#include "FileFormats/ffmt_factory.h"
#include "FileFormats/ffmt_enums.h"

#include "opl/measurer.h"

#include "common.h"
#include "version.h"

#define INS_INDEX   (Qt::UserRole)
#define INS_BANK_ID (Qt::UserRole + 1)
#define INS_INS_ID  (Qt::UserRole + 2)

static void setInstrumentMetaInfo(QListWidgetItem *item, int index)
{
    item->setData(INS_INDEX, index);
    item->setData(INS_BANK_ID, index / 128);
    item->setData(INS_INS_ID, index % 128);
    item->setToolTip(QObject::tr("Bank %1, ID: %2").arg(index / 128).arg(index % 128));
}

static QIcon makeWindowIcon()
{
    QIcon icon;
    icon.addPixmap(QPixmap(":/icons/opl3_16.png"));
    icon.addPixmap(QPixmap(":/icons/opl3_32.png"));
    icon.addPixmap(QPixmap(":/icons/opl3_48.png"));
    return icon;
}

BankEditor::BankEditor(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::BankEditor)
{
    FmBankFormatFactory::registerAllFormats();
    m_curInst = nullptr;
    m_curInstBackup = nullptr;
    m_lock = false;
    m_recentFormat = BankFormats::FORMATS_DEFAULT_FORMAT;
    m_currentFileFormat = BankFormats::FORMAT_UNKNOWN;
    m_recentNum     = -1;
    m_recentPerc    = false;
    ui->setupUi(this);
    this->setWindowIcon(makeWindowIcon());
    ui->version->setText(QString("%1, v.%2").arg(PROGRAM_NAME).arg(VERSION));
    m_recentMelodicNote = ui->noteToTest->value();
    m_bank.Ins_Melodic_box.fill(FmBank::blankInst());
    m_bank.Ins_Percussion_box.fill(FmBank::blankInst(true));

    QActionGroup *actionGroupStandard = new QActionGroup(this);
    m_actionGroupStandard = actionGroupStandard;
    ui->actionStandardGM->setData(kMidiSpecGM1);
    ui->actionStandardGM2->setData(kMidiSpecGM2|kMidiSpecGM1);
    ui->actionStandardGS->setData(kMidiSpecGS|kMidiSpecSC|kMidiSpecGM1);
    ui->actionStandardXG->setData(kMidiSpecXG|kMidiSpecGM1);
    actionGroupStandard->addAction(ui->actionStandardGM);
    actionGroupStandard->addAction(ui->actionStandardGM2);
    actionGroupStandard->addAction(ui->actionStandardGS);
    actionGroupStandard->addAction(ui->actionStandardXG);
    actionGroupStandard->setExclusive(true);
    ui->actionStandardXG->setChecked(true);
    connect(actionGroupStandard, SIGNAL(triggered(QAction *)),
            this, SLOT(reloadInstrumentNames()));
    connect(actionGroupStandard, SIGNAL(triggered(QAction *)),
            this, SLOT(reloadBankNames()));

    setMelodic();
    connect(ui->melodic,    SIGNAL(clicked(bool)),  this,   SLOT(setMelodic()));
    connect(ui->percussion, SIGNAL(clicked(bool)),  this,   SLOT(setDrums()));
    loadInstrument();
#if QT_VERSION >= 0x050000
    this->setWindowFlags(Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
                         Qt::WindowCloseButtonHint |
                         Qt::WindowMinimizeButtonHint);
#else
    this->setWindowFlags(this->windowFlags() & ~Qt::WindowMaximizeButtonHint);
#endif
    m_importer = new Importer(this);
    m_measurer = new Measurer(this);
    connect(ui->actionImport, SIGNAL(triggered()), m_importer, SLOT(show()));
    connect(ui->actionEmulatorNuked, SIGNAL(triggered()), this, SLOT(toggleEmulator()));
    connect(ui->actionEmulatorDosBox, SIGNAL(triggered()), this, SLOT(toggleEmulator()));
    connect(ui->actionEmulatorOpal, SIGNAL(triggered()), this, SLOT(toggleEmulator()));
    connect(ui->actionEmulatorJava, SIGNAL(triggered()), this, SLOT(toggleEmulator()));
    connect(ui->actionWin9xOPLProxy, SIGNAL(triggered()), this, SLOT(toggleEmulator()));
    connect(ui->actionSerialPortOPL, SIGNAL(triggered()), this, SLOT(toggleEmulator()));

#ifdef ENABLE_HW_OPL_PROXY
    m_proxyOpl = &Generator::oplProxy();
#else
    ui->actionWin9xOPLProxy->setVisible(false);
#endif

#ifdef ENABLE_HW_OPL_SERIAL_PORT
    m_serialPortOpl = &Generator::serialPortOpl();
#else
    ui->actionSerialPortOPL->setVisible(false);
#endif

#if !defined(ENABLE_HW_OPL_PROXY) && !defined(ENABLE_HW_OPL_SERIAL_PORT)
    ui->actionHardware_OPL->setVisible(false);
#endif

    OperatorEditor *op_editors[4] = {ui->c1edit, ui->m1edit, ui->c2edit, ui->m2edit};
    for(size_t i = 0; i < 4; i++)
    {
        OperatorEditor *ed = op_editors[i];
        ed->setOperatorNumber(i);
        connect(ed, SIGNAL(operatorChanged()), this, SLOT(onOperatorChanged()));
    }

    ui->instruments->installEventFilter(this);

    ui->pitchBendSlider->setTracking(true);

    connect(ui->actionLanguageDefault, SIGNAL(triggered()), this, SLOT(onActionLanguageTriggered()));

    loadSettings();
    m_bank.deep_tremolo = ui->deepTremolo->isChecked();
    m_bank.deep_vibrato = ui->deepVibrato->isChecked();
    m_bankBackup = m_bank;

    initAudio();

#ifdef ENABLE_HW_OPL_PROXY
    initChip();
#endif

#ifdef ENABLE_MIDI
    QAction *midiInAction = m_midiInAction = new QAction(
        ui->midiIn->icon(), ui->midiIn->text(), this);
    ui->midiIn->setDefaultAction(midiInAction);
    QMenu *midiInMenu = new QMenu(this);
    midiInAction->setMenu(midiInMenu);
#else
    ui->midiIn_zone->hide();
#endif

#if !defined(ENABLE_PLOTS)
    ui->actionDelayAnalysis->setVisible(false);
#endif

    createLanguageChoices();

    Application::instance()->translate(m_language);
}

BankEditor::~BankEditor()
{
    if (m_audioOut)
        m_audioOut->stop();
    delete m_audioOut;
    m_audioOut = nullptr;

#ifdef ENABLE_MIDI
    delete m_midiIn;
    m_midiIn = nullptr;
#endif
    delete m_measurer;
    delete m_generator;
    delete m_importer;
    delete ui;
}

void BankEditor::loadSettings()
{
    QApplication::setOrganizationName(COMPANY);
    QApplication::setOrganizationDomain(PGE_URL);
    QApplication::setApplicationName("OPL FM Banks Editor");

    int preferredMidiStandard = 3;
    int defaultChip = Generator::CHIP_Nuked;
#ifdef ENABLE_WIN9X_OPL_PROXY
    defaultChip = Generator::CHIP_Win9xProxy;
#endif

    QSettings setup;
    ui->deepTremolo->setChecked(setup.value("deep-tremolo", false).toBool());
    ui->deepVibrato->setChecked(setup.value("deep-vibrato", false).toBool());
    m_recentPath = setup.value("recent-path").toString();
    {
        int chipEmulator = setup.value("chip-emulator", defaultChip).toInt();
        if(chipEmulator >= Generator::CHIP_END)
            chipEmulator = Generator::CHIP_BEGIN;
        if(chipEmulator < Generator::CHIP_BEGIN)
            chipEmulator = Generator::CHIP_BEGIN;
        m_currentChip = static_cast<Generator::OPL_Chips>(chipEmulator);
    }
    m_language = setup.value("language").toString();
    m_audioLatency = setup.value("audio-latency", audioDefaultLatency).toDouble();
    m_audioDevice = setup.value("audio-device", QString()).toString();
    m_audioDriver = setup.value("audio-driver", QString()).toString();

#ifdef ENABLE_HW_OPL_PROXY
    m_proxyOplAddress = setup.value("hw-opl-address", 0x388).toUInt();
    Win9x_OPL_Proxy &proxy = *m_proxyOpl;
    proxy.setOplAddress(m_proxyOplAddress);
#endif

#ifdef ENABLE_HW_OPL_SERIAL_PORT
    m_serialPortName = setup.value("hw-opl-serial-port", QString()).toString();
    m_serialPortBaudRate = setup.value("hw-opl-serial-baud-rate", 115200).toUInt();
    m_serialPortProtocol = setup.value("hw-opl-serial-protocol", 0u).toUInt();
    OPL_SerialPort &serial = *m_serialPortOpl;
    serial.connectPort(m_serialPortName, m_serialPortBaudRate, m_serialPortProtocol);
#endif

    preferredMidiStandard = setup.value("preferred-midi-standard", 3).toInt();

#if defined(ENABLE_HW_OPL_PROXY) || defined(ENABLE_HW_OPL_SERIAL_PORT)
    initChip();
#endif

    if (m_audioLatency < audioMinimumLatency)
        m_audioLatency = audioMinimumLatency;
    else if (m_audioLatency > audioMaximumLatency)
        m_audioLatency = audioMaximumLatency;

    ui->actionEmulatorNuked->setChecked(false);
    ui->actionEmulatorDosBox->setChecked(false);
    ui->actionEmulatorOpal->setChecked(false);
    ui->actionEmulatorJava->setChecked(false);
    ui->actionWin9xOPLProxy->setChecked(false);
    ui->actionSerialPortOPL->setChecked(false);

    switch(m_currentChip)
    {
    default:
    case Generator::CHIP_Nuked:
        ui->actionEmulatorNuked->setChecked(true);
        break;
    case Generator::CHIP_DosBox:
        ui->actionEmulatorDosBox->setChecked(true);
        break;
    case Generator::CHIP_Opal:
        ui->actionEmulatorOpal->setChecked(true);
        break;
    case Generator::CHIP_Java:
        ui->actionEmulatorJava->setChecked(true);
        break;
    case Generator::CHIP_Win9xProxy:
        ui->actionWin9xOPLProxy->setChecked(true);
        break;
    case Generator::CHIP_SerialPort:
        ui->actionSerialPortOPL->setChecked(true);
        break;
    }

    switch(preferredMidiStandard)
    {
    case 0: // GM
        ui->actionStandardGM->setChecked(true);
        break;

    case 1: // GM2
        ui->actionStandardGM2->setChecked(true);
        break;

    case 2: // GS
        ui->actionStandardGS->setChecked(true);
        break;
    default:
    case 3: // XG, initially set by default
        break;
    }
}

void BankEditor::saveSettings()
{
    QSettings setup;
    setup.setValue("deep-tremolo", ui->deepTremolo->isChecked());
    setup.setValue("deep-vibrato", ui->deepVibrato->isChecked());
    setup.setValue("recent-path", m_recentPath);
    setup.setValue("chip-emulator", (int)m_currentChip);
    setup.setValue("language", m_language);
    setup.setValue("audio-latency", m_audioLatency);
    setup.setValue("audio-device", m_audioDevice);
    setup.setValue("audio-driver", m_audioDriver);

#ifdef ENABLE_HW_OPL_PROXY
    setup.setValue("hw-opl-address", m_proxyOplAddress);
#endif

#ifdef ENABLE_HW_OPL_SERIAL_PORT
    setup.setValue("hw-opl-serial-port", m_serialPortName);
    setup.setValue("hw-opl-serial-baud-rate", m_serialPortBaudRate);
    setup.setValue("hw-opl-serial-protocol", m_serialPortProtocol);
#endif

    int preferredMidiStandard = 3;
    if(ui->actionStandardGM->isChecked())
        preferredMidiStandard = 0;
    else if(ui->actionStandardGM2->isChecked())
        preferredMidiStandard = 1;
    else if(ui->actionStandardGS->isChecked())
        preferredMidiStandard = 2;
    else if(ui->actionStandardXG->isChecked())
        preferredMidiStandard = 3;
    setup.setValue("preferred-midi-standard", preferredMidiStandard);
}



void BankEditor::closeEvent(QCloseEvent *event)
{
    if(!askForSaving())
    {
        event->ignore();
        return;
    }

    saveSettings();
}

void BankEditor::dragEnterEvent(QDragEnterEvent *e)
{
    if(e->mimeData()->hasUrls())
        e->acceptProposedAction();
}

void BankEditor::dropEvent(QDropEvent *e)
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

void BankEditor::showEvent(QShowEvent *event)
{
    QMainWindow::showEvent(event);
    QTimer::singleShot(0, this, SLOT(onBankEditorShown()));
}

void BankEditor::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
        onLanguageChanged();
    QMainWindow::changeEvent(event);
}

bool BankEditor::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == ui->instruments)
    {
        /* Take arrow and page key events, reserve others for piano. */
        QEvent::Type type = event->type();
        if (type == QEvent::KeyPress || type == QEvent::KeyRelease)
        {
            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
            int key = keyEvent->key();
            bool accepted =
                key == Qt::Key_Up || key == Qt::Key_Down ||
                key == Qt::Key_Left || key == Qt::Key_Right ||
                key == Qt::Key_PageUp || key == Qt::Key_PageDown;
            if (!accepted) {
                if (type == QEvent::KeyPress)
                    pianoKeyPress(keyEvent);
                else if (type == QEvent::KeyRelease)
                    pianoKeyRelease(keyEvent);
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}

void BankEditor::onBankEditorShown()
{
    adjustSize();
}

void BankEditor::onLanguageChanged()
{
    ui->retranslateUi(this);
    OperatorEditor *op_editors[4] = {ui->c1edit, ui->m1edit, ui->c2edit, ui->m2edit};
    for(size_t i = 0; i < 4; i++)
    {
        OperatorEditor *ed = op_editors[i];
        ed->onLanguageChanged();
    }
    ui->currentFile->setText(m_currentFilePath);
    ui->version->setText(QString("%1, v.%2").arg(PROGRAM_NAME).arg(VERSION));
    reloadBanks();
    displayDebugDelaysInfo();
}

void BankEditor::initFileData(QString &filePath)
{
    m_recentPath = QFileInfo(filePath).absoluteDir().absolutePath();
    m_recentBankFilePath = filePath;

    if(!ui->instruments->selectedItems().isEmpty())
    {
        int idOfSelected = ui->instruments->selectedItems().first()->data(Qt::UserRole).toInt();
        if(ui->melodic->isChecked())
            setMelodic();
        else
            setDrums();
        ui->instruments->clearSelection();
        QList<QListWidgetItem *> items = ui->instruments->findItems("*", Qt::MatchWildcard);
        for(int i = 0; i < items.size(); i++)
        {
            if(items[i]->data(Qt::UserRole).toInt() == idOfSelected)
            {
                items[i]->setSelected(true);
                break;
            }
        }
        if(!ui->instruments->selectedItems().isEmpty())
            on_instruments_currentItemChanged(ui->instruments->selectedItems().first(), NULL);
    }
    else
        on_instruments_currentItemChanged(NULL, NULL);

    ui->currentFile->setText(filePath);
    m_currentFilePath = filePath;
    m_bankBackup = m_bank;

    //Set global flags and states
    m_lock = true;
    ui->deepTremolo->setChecked(m_bank.deep_tremolo);
    ui->deepVibrato->setChecked(m_bank.deep_vibrato);
    ui->volumeModel->setCurrentIndex((int)m_bank.volume_model);
    m_lock = false;

    reloadInstrumentNames();
    reloadBanks();
    setCurrentInstrument(m_recentNum, m_recentPerc);
}

void BankEditor::reInitFileDataAfterSave(QString &filePath)
{
    ui->currentFile->setText(filePath);
    m_currentFilePath = filePath;
    m_recentPath = QFileInfo(filePath).absoluteDir().absolutePath();
    m_recentBankFilePath = filePath;
    m_bankBackup = m_bank;
}

bool BankEditor::openFile(QString filePath, FfmtErrCode *errp)
{
    BankFormats format;
    FfmtErrCode err = FmBankFormatFactory::OpenBankFile(filePath, m_bank, &format);
    m_recentFormat = format;
    if(err != FfmtErrCode::ERR_OK)
    {
        if (!errp) {
            QString errText = FileFormats::getErrorText(err);
            ErrMessageO(this, errText);
        }
        else
            *errp = err;
        return false;
    }
    else
    {
        m_currentFileFormat = format;
        initFileData(filePath);
        statusBar()->showMessage(tr("Bank '%1' has been loaded!").arg(filePath), 5000);
        return true;
    }
}

bool BankEditor::openOrImportFile(QString filePath)
{
    FfmtErrCode openErr;
    if (openFile(filePath, &openErr))
        return true;

    Importer &importer = *m_importer;
    FfmtErrCode importErr;
    if(!importer.openFile(filePath, true, &importErr) &&
       !importer.openFile(filePath, false, &importErr))
    {
        QString errText = FileFormats::getErrorText(openErr);
        ErrMessageO(this, errText);
        return false;
    }

    importer.show();
    return true;
}

bool BankEditor::saveBankFile(QString filePath, BankFormats format)
{
    if(FmBankFormatFactory::hasCaps(format, (int)FormatCaps::FORMAT_CAPS_MELODIC_ONLY))
    {
        int reply = QMessageBox::question(this,
                                          tr("Save melodic-only bank file"),
                                          tr("Saving into '%1' format allows to save "
                                             "one melodic only bank. "
                                             "All other banks include percussion will be ignored while saving into the file.\n\n"
                                             "Do you want to continue file saving?")
                                          .arg(FmBankFormatFactory::formatName(format)),
                                          QMessageBox::Yes|QMessageBox::Cancel);
        if(reply != QMessageBox::Yes)
            return false;
    }
    else
    if(FmBankFormatFactory::hasCaps(format, (int)FormatCaps::FORMAT_CAPS_PERCUSSION_ONLY))
    {
        int reply = QMessageBox::question(this,
                                          tr("Save percussion-only bank file"),
                                          tr("Saving into '%1' format allows to save "
                                             "one percussion only bank. "
                                             "All other banks include melodic will be ignored while saving into the file.\n\n"
                                             "Do you want to continue file saving?")
                                          .arg(FmBankFormatFactory::formatName(format)),
                                          QMessageBox::Yes|QMessageBox::Cancel);
        if(reply != QMessageBox::Yes)
            return false;
    }
    else
    if(FmBankFormatFactory::hasCaps(format, (int)FormatCaps::FORMAT_CAPS_GM_BANK) &&
        ((m_bank.Banks_Melodic.size() > 1) || (m_bank.Banks_Percussion.size() > 1))
    )
    {
        int reply = QMessageBox::question(this,
                                          tr("Save GeneralMIDI bank file"),
                                          tr("Saving into '%1' format allows you to have "
                                             "one melodic and one percussion banks only. "
                                             "All extra banks will be ignored while saving into the file.\n\n"
                                             "Do you want to continue file saving?")
                                          .arg(FmBankFormatFactory::formatName(format)),
                                          QMessageBox::Yes|QMessageBox::Cancel);
        if(reply != QMessageBox::Yes)
            return false;
    }

    if(FmBankFormatFactory::hasCaps(format, (int)FormatCaps::FORMAT_CAPS_NEEDS_MEASURE))
    {
        if(!m_measurer->doMeasurement(m_bank, m_bankBackup))
            return false;//Measurement was cancelled
    }

    FfmtErrCode err = FmBankFormatFactory::SaveBankFile(filePath, m_bank, format);

    if(err != FfmtErrCode::ERR_OK)
    {
        QString errText;
        switch(err)
        {
        case FfmtErrCode::ERR_BADFORMAT:
            errText = tr("bad file format");
            break;
        case FfmtErrCode::ERR_NOFILE:
            errText = tr("can't open file for write");
            break;
        case FfmtErrCode::ERR_NOT_IMPLEMENTED:
            errText = tr("writing into this format is not implemented yet");
            break;
        case FfmtErrCode::ERR_UNSUPPORTED_FORMAT:
            errText = tr("unsupported file format, please define file name extension to choice target file format");
            break;
        case FfmtErrCode::ERR_UNKNOWN:
            errText = tr("unknown error occurred");
            break;
        case FfmtErrCode::ERR_OK:
            break;
        }
        ErrMessageS(this, errText);
        return false;
    }
    else
    {
        //Override 'recently-saved' format
        m_recentFormat = format;
        reInitFileDataAfterSave(filePath);
        statusBar()->showMessage(tr("Bank file '%1' has been saved!").arg(filePath), 5000);
        return true;
    }
}

bool BankEditor::saveInstrumentFile(QString filePath, InstFormats format)
{
    Q_ASSERT(m_curInst);
    FfmtErrCode err = FmBankFormatFactory::SaveInstrumentFile(filePath, *m_curInst, format, ui->percussion->isChecked());
    if(err != FfmtErrCode::ERR_OK)
    {
        QString errText;
        switch(err)
        {
        case FfmtErrCode::ERR_BADFORMAT:
            errText = tr("bad file format");
            break;
        case FfmtErrCode::ERR_NOFILE:
            errText = tr("can't open file for write");
            break;
        case FfmtErrCode::ERR_NOT_IMPLEMENTED:
            errText = tr("writing into this format is not implemented yet");
            break;
        case FfmtErrCode::ERR_UNSUPPORTED_FORMAT:
            errText = tr("unsupported file format, please define file name extension to choice target file format");
            break;
        case FfmtErrCode::ERR_UNKNOWN:
            errText = tr("unknown error occurred");
            break;
        case FfmtErrCode::ERR_OK:
            break;
        }
        ErrMessageS(this, errText);
        return false;
    }
    else
    {
        statusBar()->showMessage(tr("Instrument file '%1' has been saved!").arg(filePath), 5000);
        return true;
    }
}

bool BankEditor::saveFileAs(const QString &optionalFilePath)
{
    QString fileToSave;
    bool canSaveDirectly = false;
    BankFormats saveFormat;

    if(!optionalFilePath.isEmpty())
    {
        saveFormat = m_currentFileFormat;
        canSaveDirectly =
            saveFormat != BankFormats::FORMAT_UNKNOWN &&
            !FmBankFormatFactory::hasCaps(saveFormat, (int)FormatCaps::FORMAT_CAPS_MELODIC_ONLY) &&
            !FmBankFormatFactory::hasCaps(saveFormat, (int)FormatCaps::FORMAT_CAPS_PERCUSSION_ONLY) &&
            !FmBankFormatFactory::hasCaps(saveFormat, (int)FormatCaps::FORMAT_CAPS_GM_BANK);
    }

    if(canSaveDirectly)
        fileToSave = optionalFilePath;
    else
    {
        QString filters         = FmBankFormatFactory::getSaveFiltersList();
        QString selectedFilter  = FmBankFormatFactory::getFilterFromFormat(m_recentFormat, (int)FormatCaps::FORMAT_CAPS_SAVE);
        fileToSave      = QFileDialog::getSaveFileName(this, "Save bank file",
                                                       m_recentBankFilePath, filters, &selectedFilter,
                                                       FILE_OPEN_DIALOG_OPTIONS);
        saveFormat = FmBankFormatFactory::getFormatFromFilter(selectedFilter);
    }
    if(fileToSave.isEmpty())
        return false;

    if(!saveBankFile(fileToSave, saveFormat))
       return false;
    m_currentFileFormat = saveFormat;
    return true;
}

bool BankEditor::saveInstFileAs()
{
    if(!m_curInst)
    {
        QMessageBox::information(this,
                                 tr("Nothing to save"),
                                 tr("No selected instrument to save. Please select an instrument first!"));
        return false;
    }
    QString filters = FmBankFormatFactory::getInstSaveFiltersList();
    QString selectedFilter = FmBankFormatFactory::getInstFilterFromFormat(m_recentInstFormat, (int)FormatCaps::FORMAT_CAPS_SAVE);
    QString fileToSave = QFileDialog::getSaveFileName(this, "Save instrument file",
                                                      m_recentPath, filters, &selectedFilter,
                                                      FILE_OPEN_DIALOG_OPTIONS);
    if(fileToSave.isEmpty())
        return false;
    return saveInstrumentFile(fileToSave, FmBankFormatFactory::getInstFormatFromFilter(selectedFilter));
}

bool BankEditor::askForSaving()
{
    if(m_bank != m_bankBackup)
    {
        QMessageBox::StandardButton res = QMessageBox::question(this, tr("File is not saved"), tr("File is modified and not saved. Do you want to save it?"), QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
        if((res == QMessageBox::Cancel) || (res == QMessageBox::NoButton))
            return false;
        else if(res == QMessageBox::Yes)
        {
            if(!saveFileAs())
                return false;
        }
    }

    return true;
}

QString BankEditor::getInstrumentName(int instrument, bool isAuto, bool isPerc) const
{
    int index = ui->bank_no->currentIndex();
    QString name = tr("<Unknown>");
    if(index >= 0)
    {
        if(isAuto)
            isPerc = isDrumsMode();
        int lsb = ui->bank_lsb->value();
        int msb = ui->bank_msb->value();
        MidiProgramId pr = MidiProgramId(isPerc, msb, lsb, instrument % 128);
        unsigned spec = getSelectedMidiSpec();
        unsigned specObtained = kMidiSpecXG;
        const MidiProgram *p = getMidiProgram(pr, spec, &specObtained);
        p = p ? p : getFallbackProgram(pr, spec, &specObtained);
        name = p ? p->patchName : tr("<Reserved %1>").arg(instrument % 128);
    }
    return name;
}

QString BankEditor::getBankName(int bank, bool isAuto, bool isPerc)
{
    QString name;
    if(bank >= 0)
    {
        if(isAuto)
            isPerc = isDrumsMode();
        FmBank::MidiBank *mb = isPerc ?
            &m_bank.Banks_Percussion[bank] : &m_bank.Banks_Melodic[bank];
        if(mb->name[0])
            name = QString::fromUtf8(mb->name);
        else
        {
            MidiProgramId id(isPerc, mb->msb, mb->lsb, 0);
            unsigned spec = getSelectedMidiSpec();
            unsigned specObtained = kMidiSpecXG;
            if(const MidiProgram *p = getMidiBank(id, spec, &specObtained))
                name = QString::fromUtf8(p->bankName);
        }
    }
    return name;
}

unsigned BankEditor::getSelectedMidiSpec() const
{
    QAction *act = m_actionGroupStandard->checkedAction();
    return act ? act->data().toUInt() : unsigned(kMidiSpecAny);
}

void BankEditor::flushInstrument()
{
    loadInstrument();
    if(m_curInst && ui->percussion->isChecked())
        ui->noteToTest->setValue(m_curInst->percNoteNum);
    sendPatch();
}

void BankEditor::syncInstrumentName()
{
    QListWidgetItem *curInstr = ui->instruments->currentItem();
    if(m_curInst && curInstr)
    {
        curInstr->setText(
            m_curInst->name[0] != '\0' ?
                    QString::fromUtf8(m_curInst->name) :
                    getInstrumentName(m_recentNum)
        );
    }
    syncInstrumentBlankness();
}

void BankEditor::syncInstrumentBlankness()
{
    QListWidgetItem *curInstr = ui->instruments->currentItem();
    if(m_curInst && curInstr)
    {
        curInstr->setForeground(m_curInst->is_blank ?
                                Qt::gray : Qt::black);
    }
}

void BankEditor::on_actionNew_triggered()
{
    if(!askForSaving())
        return;
    m_recentFormat = BankFormats::FORMATS_DEFAULT_FORMAT;
    ui->currentFile->setText(tr("<Untitled>"));
    m_currentFilePath.clear();
    m_currentFileFormat = BankFormats::FORMAT_UNKNOWN;
    ui->instruments->clearSelection();
    m_bank.reset();
    m_bank.Ins_Melodic_box.fill(FmBank::blankInst());
    m_bank.Ins_Percussion_box.fill(FmBank::blankInst(true));
    m_bankBackup.reset();
    on_instruments_currentItemChanged(NULL, NULL);
    reloadInstrumentNames();
    reloadBanks();
}

void BankEditor::on_actionOpen_triggered()
{
    if(!askForSaving())
        return;
    QString filters = FmBankFormatFactory::getOpenFiltersList();
    QString fileToOpen;
    fileToOpen = QFileDialog::getOpenFileName(this, "Open bank file",
                                              m_recentPath, filters, nullptr,
                                              FILE_OPEN_DIALOG_OPTIONS);
    if(fileToOpen.isEmpty())
        return;
    openFile(fileToOpen);
}

void BankEditor::on_actionSave_triggered()
{
    saveFileAs(m_currentFilePath);
}

void BankEditor::on_actionSaveAs_triggered()
{
    saveFileAs();
}

void BankEditor::on_actionSaveInstrument_triggered()
{
    saveInstFileAs();
}

void BankEditor::on_actionExit_triggered()
{
    this->close();
}

void BankEditor::on_actionCopy_triggered()
{
    if(!m_curInst) return;
    QByteArray data((char *)m_curInst, sizeof(FmBank::Instrument));
    QMimeData *md = new QMimeData;
    md->setData("application/x-vnd.wohlstand.opl3-fm-instrument", data);
    QApplication::clipboard()->setMimeData(md);
}

static bool instrumentFromClipboard(FmBank::Instrument &inst)
{
    const QMimeData *md = QApplication::clipboard()->mimeData();
    if(!md) return false;
    QByteArray data = md->data("application/x-vnd.wohlstand.opl3-fm-instrument");
    if(data.size() != sizeof(FmBank::Instrument)) return false;
    memcpy(&inst, data.data(), sizeof(FmBank::Instrument));
    return true;
}

void BankEditor::on_actionPaste_triggered()
{
    if(!m_curInst || !instrumentFromClipboard(*m_curInst)) return;
    flushInstrument();
    syncInstrumentName();
}

void BankEditor::on_actionPasteVoice11_triggered()
{
    if(!m_curInst) return;
    FmBank::Instrument clipboardInst;
    if(!instrumentFromClipboard(clipboardInst)) return;
    size_t buffSize = sizeof(FmBank::Operator) * 2;
    m_curInst->feedback1 = clipboardInst.feedback1;
    m_curInst->connection1 = clipboardInst.connection1;
    m_curInst->note_offset1 = clipboardInst.note_offset1;
    memcpy(m_curInst->OP, clipboardInst.OP, buffSize);
    flushInstrument();
}

void BankEditor::on_actionPasteVoice12_triggered()
{
    if(!m_curInst) return;
    FmBank::Instrument clipboardInst;
    if(!instrumentFromClipboard(clipboardInst)) return;
    size_t buffSize = sizeof(FmBank::Operator) * 2;
    m_curInst->feedback2 = clipboardInst.feedback1;
    m_curInst->connection2 = clipboardInst.connection1;
    m_curInst->note_offset2 = clipboardInst.note_offset1;
    memcpy(m_curInst->OP + 2, clipboardInst.OP, buffSize);
    flushInstrument();
}

void BankEditor::on_actionPasteVoice21_triggered()
{
    if(!m_curInst) return;
    FmBank::Instrument clipboardInst;
    if(!instrumentFromClipboard(clipboardInst)) return;
    size_t buffSize = sizeof(FmBank::Operator) * 2;
    m_curInst->feedback1 = clipboardInst.feedback2;
    m_curInst->connection1 = clipboardInst.connection2;
    m_curInst->note_offset1 = clipboardInst.note_offset2;
    memcpy(m_curInst->OP, clipboardInst.OP + 2, buffSize);
    flushInstrument();
}

void BankEditor::on_actionPasteVoice22_triggered()
{
    if(!m_curInst) return;
    FmBank::Instrument clipboardInst;
    if(!instrumentFromClipboard(clipboardInst)) return;
    size_t buffSize = sizeof(FmBank::Operator) * 2;
    m_curInst->feedback2 = clipboardInst.feedback2;
    m_curInst->connection2 = clipboardInst.connection2;
    m_curInst->note_offset2 = clipboardInst.note_offset2;
    memcpy(m_curInst->OP + 2, clipboardInst.OP + 2, buffSize);
    flushInstrument();
}

void BankEditor::on_actionSwapVoices_triggered()
{
    if(!m_curInst) return;
    size_t buffSize = sizeof(FmBank::Operator) * 2;
    FmBank::Operator buffer[2];
    std::swap(m_curInst->feedback1, m_curInst->feedback2);
    std::swap(m_curInst->connection1, m_curInst->connection2);
    std::swap(m_curInst->note_offset1, m_curInst->note_offset2);
    memcpy(buffer, m_curInst->OP, buffSize);
    memcpy(m_curInst->OP, m_curInst->OP + 2, buffSize);
    memcpy(m_curInst->OP + 2, buffer, buffSize);
    flushInstrument();
}

void BankEditor::on_actionReset_current_instrument_triggered()
{
    if(!m_curInstBackup || !m_curInst)
        return; //Some pointer is Null!!!
    if(memcmp(m_curInst, m_curInstBackup, sizeof(FmBank::Instrument)) == 0)
        return; //Nothing to do
    if(QMessageBox::Yes == QMessageBox::question(this,
            tr("Reset instrument to initial state"),
            tr("This instrument will be reset to initial state "
               "(since this file was loaded or saved).\n"
               "Do you wish to continue?"),
            QMessageBox::Yes | QMessageBox::No))
    {
        memcpy(m_curInst, m_curInstBackup, sizeof(FmBank::Instrument));
        flushInstrument();
        syncInstrumentName();
    }
}

void BankEditor::on_actionReMeasure_triggered()
{
    int reply = QMessageBox::question(this,
                          tr("Are you sure?"),
                          tr("All sounding delays measures will be re-calculated. "
                             "This operation may take a while. Do you want to continue? "
                             "You may cancel operation in any moment."),
                          QMessageBox::Yes|QMessageBox::Cancel);
    if(reply == QMessageBox::Yes)
    {
        if(m_measurer->doMeasurement(m_bank, m_bankBackup, true))
            statusBar()->showMessage(tr("Sounding delays calculation has been completed!"), 5000);
        else
            statusBar()->showMessage(tr("Sounding delays calculation was canceled!"), 5000);
    }
}

void BankEditor::on_actionReMeasureOne_triggered()
{
    FmBank::Instrument *inst = m_curInst;
    FmBank::Instrument *instBackup = m_curInstBackup;
    if(!inst)
    {
        QMessageBox::information(this,
                                 tr("Nothing to measure"),
                                 tr("No selected instrument to measure. Please select an instrument first!"));
        return;
    }

    FmBank::Instrument workInst = *inst;
    workInst.is_blank = false;

    const FmBank::Instrument blankInst = FmBank::emptyInst();
    if(memcmp(&workInst, &blankInst, sizeof(FmBank::Instrument)) == 0)
        return;

    if(m_measurer->doMeasurement(workInst))
    {
        *instBackup = *inst;
        *inst = workInst;
        loadInstrument();
    }
}

void BankEditor::on_actionChipsBenchmark_triggered()
{
    if(m_curInst)
    {
        QVector<Measurer::BenchmarkResult> res;
        m_measurer->runBenchmark(*m_curInst, res);
        QString resStr;
        for(Measurer::BenchmarkResult &r : res)
            resStr += tr("%1 passed in %2 milliseconds.\n").arg(r.name).arg(r.elapsed);
        QMessageBox::information(this,
                                 tr("Benchmark result"),
                                 tr("Result of emulators benchmark based on '%1' instrument:\n\n%2")
                                 .arg(QString::fromUtf8(m_curInst->name))
                                 .arg(resStr)
                                 );
    }
    else
    {
        QMessageBox::information(this,
                                 tr("Instrument is not selected"),
                                 tr("Please select any instrument to begin the benchmark of emulators!"));
    }
}

void BankEditor::on_actionCompareWith_triggered()
{
    QString filters = FmBankFormatFactory::getOpenFiltersList();
    QString fileToOpen;
    fileToOpen = QFileDialog::getOpenFileName(this, tr("Open other bank file"),
                                              m_recentPath, filters, nullptr,
                                              FILE_OPEN_DIALOG_OPTIONS);
    if(fileToOpen.isEmpty())
        return;

    FmBank otherBank;
    BankFormats format;
    FfmtErrCode err = FmBankFormatFactory::OpenBankFile(fileToOpen, otherBank, &format);
    if(err != FfmtErrCode::ERR_OK)
    {
        QString errText = FileFormats::getErrorText(err);
        ErrMessageO(this, errText);
        return;
    }

    BankCompareDialog dlg(getSelectedMidiSpec(), m_bank, otherBank, this);
    dlg.exec();
}

#if defined(ENABLE_PLOTS)
void BankEditor::on_actionDelayAnalysis_triggered()
{
    FmBank::Instrument *inst = m_curInst;
    if(!inst)
    {
        QMessageBox::information(this,
                                 tr("Nothing to measure"),
                                 tr("No selected instrument to measure. Please select an instrument first!"));
        return;
    }

    FmBank::Instrument workInst = *inst;
    workInst.is_blank = false;

    const FmBank::Instrument blankInst = FmBank::emptyInst();
    if(memcmp(&workInst, &blankInst, sizeof(FmBank::Instrument)) == 0)
        return;

    DelayAnalysisDialog dialog(this);
    if (!dialog.computeResult(*inst))
        return;
    dialog.exec();
}
#endif

void BankEditor::on_actionFormatsSup_triggered()
{
    formats_sup sup(this);
    sup.exec();
}

void BankEditor::on_actionAbout_triggered()
{
    QMessageBox::about(this,
                       tr("About bank editor"),
                       tr("FM Bank Editor for Yamaha OPL3/OPL2 chip, Version %1\n\n"
                          "%2\n"
                          "\n"
                          "Licensed under GNU GPLv3\n\n"
                          "Source code available on GitHub:\n"
                          "%3")
                       .arg(VERSION)
                       .arg(COPYRIGHT)
                       .arg("https://github.com/Wohlstand/OPL3BankEditor"));
}

void BankEditor::on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if(!current)
    {
        //ui->curInsInfo->setText("<Not Selected>");
        m_curInst = nullptr;
        m_curInstBackup = nullptr;
    }
    else
    {
        //ui->curInsInfo->setText(QString("%1 - %2").arg(current->data(Qt::UserRole).toInt()).arg(current->text()));
        setCurrentInstrument(current->data(Qt::UserRole).toInt(), ui->percussion->isChecked());
    }

    flushInstrument();
}

void BankEditor::toggleEmulator()
{
    QObject *menuItem = sender();
    ui->actionEmulatorNuked->setChecked(false);
    ui->actionEmulatorDosBox->setChecked(false);
    ui->actionEmulatorOpal->setChecked(false);
    ui->actionEmulatorJava->setChecked(false);
    ui->actionWin9xOPLProxy->setChecked(false);
    ui->actionSerialPortOPL->setChecked(false);

    if(menuItem == ui->actionEmulatorNuked)
    {
        ui->actionEmulatorNuked->setChecked(true);
        m_currentChip = Generator::CHIP_Nuked;
        m_generator->ctl_switchChip(m_currentChip);
    }
    else
    if(menuItem == ui->actionEmulatorDosBox)
    {
        ui->actionEmulatorDosBox->setChecked(true);
        m_currentChip = Generator::CHIP_DosBox;
        m_generator->ctl_switchChip(m_currentChip);
    }
    else
    if(menuItem == ui->actionEmulatorOpal)
    {
        ui->actionEmulatorOpal->setChecked(true);
        m_currentChip = Generator::CHIP_Opal;
        m_generator->ctl_switchChip(m_currentChip);
    }
    else
    if(menuItem == ui->actionEmulatorJava)
    {
        ui->actionEmulatorJava->setChecked(true);
        m_currentChip = Generator::CHIP_Java;
        m_generator->ctl_switchChip(m_currentChip);
    }
    else
    if(menuItem == ui->actionWin9xOPLProxy)
    {
        ui->actionWin9xOPLProxy->setChecked(true);
        m_currentChip = Generator::CHIP_Win9xProxy;
        m_generator->ctl_switchChip(m_currentChip);
    }
    else
    if(menuItem == ui->actionSerialPortOPL)
    {
        ui->actionSerialPortOPL->setChecked(true);
        m_currentChip = Generator::CHIP_SerialPort;
        m_generator->ctl_switchChip(m_currentChip);
    }
}


void BankEditor::setCurrentInstrument(int num, bool isPerc)
{
    m_recentNum = num;
    m_recentPerc = isPerc;

    if(num >= 0)
    {
        m_curInst = isPerc ? &m_bank.Ins_Percussion[num] : &m_bank.Ins_Melodic[num];
        m_curInstBackup = isPerc ?
                          (num < m_bankBackup.countDrums() ? &m_bankBackup.Ins_Percussion[num] : nullptr) :
                          (num < m_bankBackup.countMelodic() ? &m_bankBackup.Ins_Melodic[num] : nullptr);
    }
    else
    {
        m_curInst = nullptr;
        m_curInstBackup = nullptr;
    }
}

void BankEditor::loadInstrument()
{
    displayDebugDelaysInfo();
    if(!m_curInst)
    {
        ui->editzone->setEnabled(false);
        ui->editzone2->setEnabled(false);
        ui->testNoteBox->setEnabled(false);
        ui->piano->setEnabled(false);
        m_lock = true;
        ui->insName->setEnabled(false);
        ui->insName->clear();
        m_lock = false;
        return;
    }

    if(ui->melodic->isChecked())
    {
        // m_curInst->is_fixed_note = false;
        // m_curInst->percNoteNum = 0; // Don't pass drum-specific data to melodic
        m_curInst->rhythm_drum_type = 0;
    }

    ui->editzone->setEnabled(true);
    ui->editzone2->setEnabled(true);
    ui->testNoteBox->setEnabled(true);
    ui->piano->setEnabled(ui->melodic->isChecked());
    ui->insName->setEnabled(true);
    m_lock = true;
    ui->insName->setText(QString::fromUtf8(m_curInst->name));
    ui->fixedNote->setChecked(m_curInst->is_fixed_note);
    ui->perc_noteNum->setValue(m_curInst->percNoteNum);
    ui->percMode->setCurrentIndex(m_curInst->rhythm_drum_type > 0 ? (m_curInst->rhythm_drum_type - 5) : 0);
    ui->op4mode->setChecked(m_curInst->en_4op);
    ui->doubleVoice->setEnabled(m_curInst->en_4op);
    ui->doubleVoice->setChecked(m_curInst->en_pseudo4op);
    ui->carrier2->setEnabled(m_curInst->en_4op);
    ui->modulator2->setEnabled(m_curInst->en_4op);
    ui->feedback2->setEnabled(m_curInst->en_4op);
    ui->connect2->setEnabled(m_curInst->en_4op);
    ui->feedback2label->setEnabled(m_curInst->en_4op);
    ui->feedback1->setValue(m_curInst->feedback1);
    ui->feedback2->setValue(m_curInst->feedback2);
    ui->secVoiceFineTune->setValue(m_curInst->fine_tune);
    ui->noteOffset1->setValue(m_curInst->note_offset1);
    ui->noteOffset2->setValue(m_curInst->note_offset2);
    ui->velocityOffset->setValue(m_curInst->velocity_offset);
    ui->am1->setChecked(m_curInst->connection1 == FmBank::Instrument::AM);
    ui->fm1->setChecked(m_curInst->connection1 == FmBank::Instrument::FM);
    ui->am2->setChecked(m_curInst->connection2 == FmBank::Instrument::AM);
    ui->fm2->setChecked(m_curInst->connection2 == FmBank::Instrument::FM);

    OperatorEditor *op_editors[4] = {ui->c1edit, ui->m1edit, ui->c2edit, ui->m2edit};
    for(unsigned i = 0; i < 4; ++i)
        op_editors[i]->loadDataFromInst(*m_curInst);

    m_lock = false;
}

void BankEditor::displayDebugDelaysInfo()
{
    if(!m_curInst)
    {
        ui->debugDelaysInfo->setText(tr("Delays on: %1, off: %2").arg("--").arg("--"));
        return;
    }
    ui->debugDelaysInfo->setText(tr("Delays on: %1, off: %2")
                                 .arg(m_curInst->ms_sound_kon)
                                 .arg(m_curInst->ms_sound_koff));
}

void BankEditor::initChip()
{
    if(!m_generator) return;
    m_generator->ctl_initChip();
}

void BankEditor::sendPatch()
{
    if(!m_curInst) return;
    if(!m_generator) return;
    m_generator->ctl_changePatch(*m_curInst, ui->percussion->isChecked());
}

void BankEditor::setDrumMode(bool dmode)
{
    if(dmode)
    {
        if(ui->noteToTest->isEnabled())
            m_recentMelodicNote = ui->noteToTest->value();
    }
    else
        ui->noteToTest->setValue(m_recentMelodicNote);
    ui->percMode->setEnabled(dmode);
    ui->noteToTest->setDisabled(dmode);
    ui->testMajor->setDisabled(dmode);
    ui->testMinor->setDisabled(dmode);
    ui->testAugmented->setDisabled(dmode);
    ui->testDiminished->setDisabled(dmode);
    ui->testMajor7->setDisabled(dmode);
    ui->testMinor7->setDisabled(dmode);
    ui->piano->setDisabled(dmode);
}

bool BankEditor::isDrumsMode() const
{
    return !ui->melodic->isChecked() || ui->percussion->isChecked();
}

void BankEditor::reloadBanks()
{
    ui->bank_no->clear();
    int countOfBanks = 1;
    bool isDrum = isDrumsMode();
    if(isDrum)
        countOfBanks = ((m_bank.countDrums() - 1) / 128) + 1;
    else
        countOfBanks = ((m_bank.countMelodic() - 1) / 128) + 1;
    for(int i = 0; i < countOfBanks; i++)
    {
        ui->bank_no->addItem(QString(), i);
        refreshBankName(i);
    }
}

void BankEditor::refreshBankName(int index)
{
    if(index < 0)
        return;
    QString name = getBankName(index, true);
    ui->bank_no->setItemText(index, QString("%1: %2").arg(index).arg(name));
}

void BankEditor::createLanguageChoices()
{
    QDir dir(Application::instance()->getAppTranslationDir());

    const QString prefix = "opl3bankeditor_";
    const QString suffix = ".qm";

#if defined(Q_OS_WIN)
    const Qt::CaseSensitivity cs = Qt::CaseInsensitive;
#else
    const Qt::CaseSensitivity cs = Qt::CaseSensitive;
#endif

    QStringList languages;
    languages.push_back("en");  // generic english
    foreach(const QString &entry, dir.entryList())
    {
        if(entry.startsWith(prefix, cs) && entry.endsWith(suffix, cs))
        {
            QString lang = entry.mid(
                prefix.size(), entry.size() - prefix.size() - suffix.size());
            languages << lang;
        }
    }

    QMenu *menuLanguage = ui->menuLanguage;

    foreach(const QString &lang, languages)
    {
        QLocale loc(lang);

        QString name;
        if(lang == "en")
            name = "English";  // generic english with UK flag icon
        else
        {
#if QT_VERSION >= 0x040800
            name = QString("%1 (%2)")
                .arg(loc.nativeLanguageName())
                .arg(loc.nativeCountryName());
#else
            name = QLocale::languageToString(loc.language());
#endif
            if(!name.isEmpty())
                name[0] = name[0].toUpper();
        }

        QString languageCode = loc.name();
        languageCode = languageCode.left(languageCode.indexOf('_'));

        QAction *act = new QAction(name, menuLanguage);
        menuLanguage->addAction(act);
        act->setData(lang);
        act->setIcon(QIcon(":/languages/" + languageCode + ".png"));
        connect(act, SIGNAL(triggered()), this, SLOT(onActionLanguageTriggered()));
    }
}

void BankEditor::on_actionAdLibBnkMode_triggered(bool checked)
{
    ui->bankListFrame->setHidden(checked);
    ui->bankRename->setHidden(checked);
    ui->bank_no->setHidden(checked);
    ui->bank_lsbmsb->setHidden(checked);
    ui->actionAddBank->setDisabled(checked);
    ui->actionCloneBank->setDisabled(checked);
    ui->actionClearBank->setDisabled(checked);
    ui->actionDeleteBank->setDisabled(checked);
    if(checked)
        on_bank_no_currentIndexChanged(ui->bank_no->currentIndex());
    else
    {
        QList<QListWidgetItem *> selected = ui->instruments->selectedItems();
        if(!selected.isEmpty())
            ui->bank_no->setCurrentIndex(selected.front()->data(INS_BANK_ID).toInt());
    }
}

void BankEditor::on_actionAudioConfig_triggered()
{
    AudioConfigDialog dlg(m_audioOut, this);
    dlg.setLatency(m_audioLatency);
    dlg.setDeviceName(m_audioDevice);
    dlg.setDriverName(m_audioDriver);
    if(dlg.exec() == QDialog::Accepted)
    {
        m_audioLatency = dlg.latency();
        m_audioDevice = dlg.deviceName();
        m_audioDriver = dlg.driverName();
    }
}

#if defined(ENABLE_HW_OPL_PROXY) || defined(ENABLE_HW_OPL_SERIAL_PORT)
void BankEditor::on_actionHardware_OPL_triggered()
{
    HardwareDialog *dlg = new HardwareDialog;

#if defined(ENABLE_HW_OPL_PROXY)
    Win9x_OPL_Proxy &proxy = *m_proxyOpl;
    bool supportsChangeAddress = proxy.canSetOplAddress();
    dlg->setOplAddress(m_proxyOplAddress);
    dlg->setCanChangeOplAddress(supportsChangeAddress);
#else
    dlg->setSoundCardOptionsVisible(false);
#endif

#if defined(ENABLE_HW_OPL_SERIAL_PORT)
    OPL_SerialPort &serial = *m_serialPortOpl;
    dlg->setSerialPortName(m_serialPortName);
    dlg->setSerialBaudRate(m_serialPortBaudRate);
    dlg->setSerialProtocol(m_serialPortProtocol);
#else
    dlg->setSerialPortOptionsVisible(false);
#endif

    dlg->exec();

    bool mustReinitialize = false;

#if defined(ENABLE_HW_OPL_PROXY)
    if(supportsChangeAddress)
    {
        unsigned newAddress = dlg->oplAddress();
        if(newAddress != m_proxyOplAddress)
        {
            proxy.setOplAddress(newAddress);
            m_proxyOplAddress = newAddress;
            mustReinitialize = true;
        }
    }
#endif

#if defined(ENABLE_HW_OPL_SERIAL_PORT)
    m_serialPortName = dlg->serialPortName();
    m_serialPortBaudRate = dlg->serialBaudRate();
    m_serialPortProtocol = dlg->serialProtocol();
    serial.connectPort(m_serialPortName, m_serialPortBaudRate, m_serialPortProtocol);
    mustReinitialize = true;
#endif

    if(mustReinitialize)
    {
        initChip();
        sendPatch();
    }

    delete dlg;
}
#endif

void BankEditor::onActionLanguageTriggered()
{
    QAction *act = static_cast<QAction *>(sender());
    QString language = act->data().toString();
    m_language = language;
    Application::instance()->translate(language);
}

void BankEditor::on_bankRename_clicked()
{
    int index = ui->bank_no->currentIndex();
    QString label;
    bool isDrum = isDrumsMode();
    bool ok = false;
    label = QInputDialog::getText(this, tr("Change name of bank"), tr("Please type name of current bank (32 characters max):"), QLineEdit::EchoMode::Normal, label, &ok);
    if(ok)
    {
        QByteArray arr = label.toUtf8();
        if(isDrum)
        {
            memset(m_bank.Banks_Percussion[index].name, 0, 32);
            memcpy(m_bank.Banks_Percussion[index].name, arr.data(), (size_t)arr.size());
        }
        else
        {
            memset(m_bank.Banks_Melodic[index].name, 0, 32);
            memcpy(m_bank.Banks_Melodic[index].name, arr.data(), (size_t)arr.size());
        }
        refreshBankName(index);
    }
}

void BankEditor::on_bank_no_currentIndexChanged(int index)
{
    ui->bank_no->setHidden(ui->actionAdLibBnkMode->isChecked());
    ui->bank_lsbmsb->setHidden(ui->actionAdLibBnkMode->isChecked() || (index < 0));
    ui->bank_lsbmsb->setDisabled(index <= 0);
    if(index >= 0)
    {
        this->m_lock = true;
        if(isDrumsMode())
        {
            ui->bank_lsb->setValue(m_bank.Banks_Percussion[index].lsb);
            ui->bank_msb->setValue(m_bank.Banks_Percussion[index].msb);
        } else {
            ui->bank_lsb->setValue(m_bank.Banks_Melodic[index].lsb);
            ui->bank_msb->setValue(m_bank.Banks_Melodic[index].msb);
        }
        this->m_lock = false;
    }
    QList<QListWidgetItem *> items = ui->instruments->findItems("*", Qt::MatchWildcard);
    for(QListWidgetItem *it : items)
        it->setHidden(!ui->actionAdLibBnkMode->isChecked() && (it->data(INS_BANK_ID) != index));
    QList<QListWidgetItem *> selected = ui->instruments->selectedItems();
    if(!selected.isEmpty())
        ui->instruments->scrollToItem(selected.front());
    QMetaObject::invokeMethod(this, "reloadInstrumentNames", Qt::QueuedConnection);
}

void BankEditor::on_bank_msb_valueChanged(int value)
{
    Q_UNUSED(value);
    if(m_lock)
        return;
    int index = ui->bank_no->currentIndex();
    if(index > 0)//Allow set only non-default
    {
        if(isDrumsMode())
            m_bank.Banks_Percussion[index].msb = uint8_t(ui->bank_msb->value());
        else
            m_bank.Banks_Melodic[index].msb = uint8_t(ui->bank_msb->value());
    }
    refreshBankName(index);
    QMetaObject::invokeMethod(this, "reloadInstrumentNames", Qt::QueuedConnection);
}

void BankEditor::on_bank_lsb_valueChanged(int value)
{
    Q_UNUSED(value);
    if(m_lock)
        return;
    int index = ui->bank_no->currentIndex();
    if(index > 0)//Allow set only non-default
    {
        if(isDrumsMode())
            m_bank.Banks_Percussion[index].lsb = uint8_t(ui->bank_lsb->value());
        else
            m_bank.Banks_Melodic[index].lsb = uint8_t(ui->bank_lsb->value());
    }
    refreshBankName(index);
    QMetaObject::invokeMethod(this, "reloadInstrumentNames", Qt::QueuedConnection);
}


void BankEditor::setMelodic()
{
    setDrumMode(false);
    reloadBanks();
    ui->instruments->clear();
    for(int i = 0; i < m_bank.countMelodic(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(m_bank.Ins_Melodic[i].name[0] != '\0' ?
                      QString::fromUtf8(m_bank.Ins_Melodic[i].name) : getInstrumentName(i, false, false));
        setInstrumentMetaInfo(item, i);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setForeground(m_bank.Ins_Melodic[i].is_blank ?
                            Qt::gray : Qt::black);
        ui->instruments->addItem(item);
    }
    on_bank_no_currentIndexChanged(ui->bank_no->currentIndex());
}

void BankEditor::setDrums()
{
    setDrumMode(true);
    reloadBanks();
    ui->instruments->clear();
    for(int i = 0; i < m_bank.countDrums(); i++)
    {
        QListWidgetItem *item = new QListWidgetItem();
        item->setText(m_bank.Ins_Percussion[i].name[0] != '\0' ?
                      QString::fromUtf8(m_bank.Ins_Percussion[i].name) : getInstrumentName(i, false, true));
        setInstrumentMetaInfo(item, i);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setForeground(m_bank.Ins_Percussion[i].is_blank ?
                            Qt::gray : Qt::black);
        ui->instruments->addItem(item);
    }
    on_bank_no_currentIndexChanged(ui->bank_no->currentIndex());
}

void BankEditor::reloadInstrumentNames()
{
    if(ui->percussion->isChecked())
    {
        if(ui->instruments->count() != m_bank.Ins_Percussion_box.size())
            setDrums();//Completely rebuild an instruments list
        else
        {
            //Change instrument names of existing entries
            for(int i = 0; i < ui->instruments->count(); i++)
            {
                auto *it = ui->instruments->item(i);
                int index = it->data(Qt::UserRole).toInt();
                it->setText(m_bank.Ins_Percussion[index].name[0] != '\0' ?
                            QString::fromUtf8(m_bank.Ins_Percussion[index].name) :
                            getInstrumentName(index, false, true));
                it->setForeground(m_bank.Ins_Percussion[i].is_blank ?
                                  Qt::gray : Qt::black);
            }
        }
    }
    else
    {
        if(ui->instruments->count() != m_bank.Ins_Melodic_box.size())
            setMelodic();//Completely rebuild an instruments list
        else
        {
            //Change instrument names of existing entries
            for(int i = 0; i < ui->instruments->count(); i++)
            {
                auto *it = ui->instruments->item(i);
                int index = it->data(Qt::UserRole).toInt();
                it->setText(m_bank.Ins_Melodic[index].name[0] != '\0' ?
                            QString::fromUtf8(m_bank.Ins_Melodic[index].name) :
                            getInstrumentName(index, false, false));
                it->setForeground(m_bank.Ins_Melodic[i].is_blank ?
                                  Qt::gray : Qt::black);
            }
        }
    }
}

void BankEditor::reloadBankNames()
{
    int countOfBanks;
    bool isDrum = isDrumsMode();
    if(isDrum)
        countOfBanks = ((m_bank.countDrums() - 1) / 128) + 1;
    else
        countOfBanks = ((m_bank.countMelodic() - 1) / 128) + 1;
    for(int i = 0; i < countOfBanks; i++)
        refreshBankName(i);
}

void BankEditor::on_actionAddInst_triggered()
{
    FmBank::Instrument ins = FmBank::emptyInst();
    int id = 0;
    QListWidgetItem *item = new QListWidgetItem();

    if(ui->melodic->isChecked())
    {
        m_bank.Ins_Melodic_box.push_back(ins);
        m_bank.Ins_Melodic = m_bank.Ins_Melodic_box.data();
        ins = m_bank.Ins_Melodic_box.last();
        id = m_bank.countMelodic() - 1;
        item->setText(ins.name[0] != '\0' ? QString::fromUtf8(ins.name) : getInstrumentName(id, false, false));
    }
    else
    {
        m_bank.Ins_Percussion_box.push_back(ins);
        m_bank.Ins_Percussion = m_bank.Ins_Percussion_box.data();
        ins = m_bank.Ins_Percussion_box.last();
        id = m_bank.countDrums() - 1;
        item->setText(ins.name[0] != '\0' ? QString::fromUtf8(ins.name) : getInstrumentName(id, false, true));
    }

    setInstrumentMetaInfo(item, id);
    item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    ui->instruments->addItem(item);
    int oldCount = ui->bank_no->count();
    reloadBanks();
    if(oldCount < ui->bank_no->count())
    {
        if(isDrumsMode())
            m_bank.Banks_Percussion.push_back(FmBank::emptyBank(uint16_t(m_bank.Banks_Percussion.count())));
        else
            m_bank.Banks_Melodic.push_back(FmBank::emptyBank(uint16_t(m_bank.Banks_Melodic.count())));
    }
    ui->bank_no->setCurrentIndex(ui->bank_no->count() - 1);
    ui->instruments->scrollToItem(item);
    item->setSelected(true);
    on_instruments_currentItemChanged(item, nullptr);
}

void BankEditor::on_actionClearInstrument_triggered()
{
    QList<QListWidgetItem *> selected = ui->instruments->selectedItems();
    if(!m_curInst || selected.isEmpty())
    {
        QMessageBox::warning(this,
                             tr("Instrument is not selected"),
                             tr("Select instrument to clear please"));
        return;
    }

    *m_curInst = FmBank::blankInst();
    loadInstrument();
    syncInstrumentName();
}

void BankEditor::on_actionDelInst_triggered()
{
    QList<QListWidgetItem *> selected = ui->instruments->selectedItems();
    if(!m_curInst || selected.isEmpty())
    {
        QMessageBox::warning(this,
                             tr("Instrument is not selected"),
                             tr("Select instrument to remove please"));
        return;
    }

    int reply = QMessageBox::question(this,
                                      tr("Single instrument deletion"),
                                      tr("Deletion of instrument will cause offset of all next instrument indexes. "
                                         "Suggested to use 'Clear instrument' action instead. "
                                         "Do you want continue deletion?"), QMessageBox::Yes | QMessageBox::No);

    if(reply == QMessageBox::Yes)
    {
        QListWidgetItem *tokill = selected.first();

        if(ui->melodic->isChecked())
        {
            m_bank.Ins_Melodic_box.remove(tokill->data(INS_INDEX).toInt());
            m_bank.Ins_Melodic = m_bank.Ins_Melodic_box.data();
        }
        else
        {
            m_bank.Ins_Percussion_box.remove(tokill->data(INS_INDEX).toInt());
            m_bank.Ins_Percussion = m_bank.Ins_Percussion_box.data();
        }

        m_curInst = nullptr;
        ui->instruments->removeItemWidget(tokill);
        selected.clear();
        delete tokill;

        // Recount indeces
        QList<QListWidgetItem *> leftItems = ui->instruments->findItems("*", Qt::MatchWildcard);
        int counter = 0;
        for(QListWidgetItem *it : leftItems)
            setInstrumentMetaInfo(it, counter++);
        reloadInstrumentNames();
        int oldBank = ui->bank_no->currentIndex();
        reloadBanks();
        if(oldBank >= ui->bank_no->count())
        {
            if(isDrumsMode())
                m_bank.Banks_Percussion.remove(oldBank);
            else
                m_bank.Banks_Melodic.remove(oldBank);
            ui->bank_no->setCurrentIndex(ui->bank_no->count() - 1);
        }
        else
            ui->bank_no->setCurrentIndex(oldBank);
        loadInstrument();
    }
}

void BankEditor::on_actionAddBank_triggered()
{
    if(ui->actionAdLibBnkMode->isChecked())
    {
        QMessageBox::information(this,
                                 tr("Add bank error"),
                                 tr("United bank mode is turned on. "
                                    "Disable it to be able add or remove banks."));
        return;
    }

    if(isDrumsMode())
    {
        int oldSize = m_bank.Ins_Percussion_box.size();
        int addSize = 128 + ((oldSize % 128 == 0) ? 0 : (128 - (oldSize % 128)));
        m_bank.Ins_Percussion_box.resize(oldSize + addSize);
        m_bank.Ins_Percussion = m_bank.Ins_Percussion_box.data();
        m_bank.Banks_Percussion.push_back(FmBank::emptyBank(uint16_t(m_bank.Banks_Percussion.count())));
        std::fill(m_bank.Ins_Percussion_box.end() - addSize, m_bank.Ins_Percussion_box.end(), FmBank::blankInst());
        setDrums();
    }
    else
    {
        int oldSize = m_bank.Ins_Melodic_box.size();
        int addSize = 128 + ((oldSize % 128 == 0) ? 0 : (128 - (oldSize % 128)));
        m_bank.Ins_Melodic_box.resize(oldSize + int(addSize));
        m_bank.Ins_Melodic = m_bank.Ins_Melodic_box.data();
        m_bank.Banks_Melodic.push_back(FmBank::emptyBank(uint16_t(m_bank.Banks_Melodic.count())));
        std::fill(m_bank.Ins_Melodic_box.end() - addSize, m_bank.Ins_Melodic_box.end(), FmBank::blankInst());
        setMelodic();
    }

    reloadBanks();
    ui->bank_no->setCurrentIndex(ui->bank_no->count() - 1);
}

void BankEditor::on_actionCloneBank_triggered()
{
    if(ui->actionAdLibBnkMode->isChecked())
    {
        QMessageBox::information(this,
                                 tr("Clone bank error"),
                                 tr("United bank mode is turned on. "
                                    "Disable it to be able add or remove banks."));
        return;
    }

    int curBank = ui->bank_no->currentIndex();
    int newBank = ui->bank_no->count();

    if(isDrumsMode())
    {
        int oldSize = m_bank.Ins_Percussion_box.size();
        int addSize = 128 + ((oldSize % 128 == 0) ? 0 : (128 - (oldSize % 128)));
        m_bank.Ins_Percussion_box.resize(oldSize + addSize);
        m_bank.Ins_Percussion = m_bank.Ins_Percussion_box.data();
        std::fill(m_bank.Ins_Percussion_box.end() - addSize, m_bank.Ins_Percussion_box.end(), FmBank::blankInst());
        memcpy(m_bank.Ins_Percussion + (newBank * 128),
               m_bank.Ins_Percussion + (curBank * 128),
               sizeof(FmBank::Instrument) * 128);
        m_bank.Banks_Percussion.push_back(FmBank::emptyBank(uint16_t(m_bank.Banks_Percussion.count())));
        setDrums();
    }
    else
    {
        int oldSize = m_bank.Ins_Melodic_box.size();
        int addSize = 128 + ((oldSize % 128 == 0) ? 0 : (128 - (oldSize % 128)));
        m_bank.Ins_Melodic_box.resize(oldSize + addSize);
        m_bank.Ins_Melodic = m_bank.Ins_Melodic_box.data();
        std::fill(m_bank.Ins_Melodic_box.end() - addSize, m_bank.Ins_Melodic_box.end(), FmBank::blankInst());
        memcpy(m_bank.Ins_Melodic + (newBank * 128),
               m_bank.Ins_Melodic + (curBank * 128),
               sizeof(FmBank::Instrument) * 128);
        m_bank.Banks_Melodic.push_back(FmBank::emptyBank(uint16_t(m_bank.Banks_Melodic.count())));
        setMelodic();
    }

    reloadBanks();
    ui->bank_no->setCurrentIndex(ui->bank_no->count() - 1);
}

void BankEditor::on_actionClearBank_triggered()
{
    if(ui->actionAdLibBnkMode->isChecked())
    {
        QMessageBox::information(this,
                                 tr("Clear bank error"),
                                 tr("United bank mode is turned on. "
                                    "Disable it to be able clear banks."));
        return;
    }
    int reply = QMessageBox::question(this,
                                      tr("128-instrument bank erasure"),
                                      tr("All instruments in this bank will be cleared. "
                                         "Do you want continue erasure?"), QMessageBox::Yes | QMessageBox::No);

    if(reply == QMessageBox::Yes)
    {
        int curBank = ui->bank_no->currentIndex();
        int needToShoot_begin   = (curBank * 128);
        int needToShoot_end     = ((curBank + 1) * 128);

        if(isDrumsMode())
        {
            if(needToShoot_end >= m_bank.Ins_Percussion_box.size())
                needToShoot_end = m_bank.Ins_Percussion_box.size();
            std::fill(m_bank.Ins_Percussion + needToShoot_begin,
                      m_bank.Ins_Percussion + needToShoot_end,
                      FmBank::blankInst());
        }
        else
        {
            if(needToShoot_end >= m_bank.Ins_Melodic_box.size())
                needToShoot_end = m_bank.Ins_Melodic_box.size();
            std::fill(m_bank.Ins_Melodic + needToShoot_begin,
                      m_bank.Ins_Melodic + needToShoot_end,
                      FmBank::blankInst());
        }
        reloadInstrumentNames();
        loadInstrument();
    }
}

void BankEditor::on_actionDeleteBank_triggered()
{
    if(ui->actionAdLibBnkMode->isChecked())
    {
        QMessageBox::information(this,
                                 tr("Delete bank error"),
                                 tr("United bank mode is turned on. "
                                    "Disable it to be able add or remove banks."));
        return;
    }

    if((ui->bank_no->currentIndex() == 0) && (ui->bank_no->count() <= 1))
    {
        QMessageBox::warning(this,
                             tr("Delete bank error"),
                             tr("Removing of last bank is not allowed!"));
        return;
    }

    int reply = QMessageBox::question(this,
                                      tr("128-instrument bank deletion"),
                                      tr("Deletion of bank will cause offset of all next bank indexes. "
                                         "Suggested to use 'Clear bank' action instead. "
                                         "Do you want continue deletion?"), QMessageBox::Yes | QMessageBox::No);

    if(reply == QMessageBox::Yes)
    {
        int curBank = ui->bank_no->currentIndex();
        int needToShoot_begin   = (curBank * 128);
        int needToShoot_end     = ((curBank + 1) * 128);

        if(isDrumsMode())
        {
            if(needToShoot_end >= m_bank.Ins_Percussion_box.size())
                needToShoot_end = m_bank.Ins_Percussion_box.size();
            m_bank.Ins_Percussion_box.remove(needToShoot_begin, needToShoot_end - needToShoot_begin);
            m_bank.Ins_Percussion = m_bank.Ins_Percussion_box.data();
            m_bank.Banks_Percussion.remove(curBank);
            setDrums();
        }
        else
        {
            if(needToShoot_end >= m_bank.Ins_Melodic_box.size())
                needToShoot_end = m_bank.Ins_Melodic_box.size();
            m_bank.Ins_Melodic_box.remove(needToShoot_begin, needToShoot_end - needToShoot_begin);
            m_bank.Ins_Melodic = m_bank.Ins_Melodic_box.data();
            m_bank.Banks_Melodic.remove(curBank);
            setMelodic();
        }

        reloadBanks();
        if(curBank >= ui->bank_no->count())
            ui->bank_no->setCurrentIndex(ui->bank_no->count() - 1);
        else
            ui->bank_no->setCurrentIndex(curBank);
    }
}

#ifdef ENABLE_MIDI
void BankEditor::updateMidiInMenu()
{
    QAction *midiInAction = m_midiInAction;
    QMenu *menu = midiInAction->menu();
    menu->clear();

    MidiInRt *midiin = m_midiIn;
    QVector<QString> ports;
    if(!midiin->getPortList(ports))
        qWarning() << midiin->getErrorText();

    if(midiin->canOpenVirtual())
    {
        QAction *act = new QAction(tr("Virtual port"), menu);
        menu->addAction(act);
        act->setData((unsigned)-1);
        if(!ports.isEmpty())
            menu->addSeparator();
        connect(act, SIGNAL(triggered()),
                this, SLOT(onMidiPortTriggered()));
    }

    for(unsigned i = 0, n = ports.size(); i < n; ++i)
    {
        QAction *act = new QAction(ports[i], menu);
        menu->addAction(act);
        act->setData(i);
        connect(act, SIGNAL(triggered()),
                this, SLOT(onMidiPortTriggered()));
    }

    menu->addSeparator();
    QAction *act = new QAction(tr("Disable"), menu);
    menu->addAction(act);
    act->setData((unsigned)-2);
    connect(act, SIGNAL(triggered()),
            this, SLOT(onMidiPortTriggered()));
}

void BankEditor::on_midiIn_triggered(QAction *)
{
    updateMidiInMenu();
    QAction *midiInAction = m_midiInAction;
    QMenu *menu = midiInAction->menu();
    menu->exec(ui->midiIn->mapToGlobal(QPoint(0, ui->midiIn->height())));
}

void BankEditor::onMidiPortTriggered()
{
    MidiInRt *midiin = m_midiIn;
    QAction *act = qobject_cast<QAction *>(sender());
    unsigned port = act->data().toUInt();

    bool portOpen;
    bool portError = false;
    if(port == (unsigned)-2)
    {
        midiin->close();
        portOpen = false;
    }
    else if(port == (unsigned)-1)
    {
        portOpen = midiin->openVirtual();
        portError = !portOpen;
    }
    else
    {
        portOpen = midiin->open(port);
        portError = !portOpen;
    }

    if(portError)
        QMessageBox::warning(
            this, tr("Error"),
            tr("Cannot open the MIDI port.") + '\n' + midiin->getErrorText());

    QWidget *button = ui->midiIn;
    QPalette pal = button->palette();
    if(portOpen)
        pal.setColor(QPalette::Button, Qt::red);
    else
        pal.setColor(QPalette::Button, qApp->style()->standardPalette().color(QPalette::Button));
    button->setPalette(pal);
}
#endif
