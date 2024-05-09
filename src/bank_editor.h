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

#ifndef BANK_EDITOR_H
#define BANK_EDITOR_H

#include <QTimer>
#include <QMainWindow>
#include <QList>
#include <QListWidgetItem>
#include "bank.h"
#include "opl/generator.h"
#include "opl/generator_realtime.h"
#include "opl/measurer.h"
#include "audio/ao_rtaudio.h"
#include "midi/midi_rtmidi.h"

#include "FileFormats/ffmt_base.h"

#ifdef NO_NATIVE_OPEN_DIALOGS
#   define FILE_OPEN_DIALOG_OPTIONS QFileDialog::DontUseNativeDialog
#else
#   define FILE_OPEN_DIALOG_OPTIONS QFileDialog::Options()
#endif

namespace Ui
{
    class BankEditor;
}

class Importer;
class TextFormat;
class QActionGroup;

/**
 * @brief Main application window
 */
class BankEditor : public QMainWindow
{
    Q_OBJECT

    friend class Importer;
private:

    Importer           *m_importer;
    //! Path for currently opened file
    QString             m_recentPath;
    //! Recently opened bank file
    QString             m_recentBankFilePath;
    //! Action group of MIDI specs
    QActionGroup       *m_actionGroupStandard;
    //! Choosen UI language
    QString             m_language;
    //! Currently using chip
    Generator::OPL_Chips m_currentChip;
    //! Audio latency (ms)
    double m_audioLatency;
    //! Name of the audio device
    QString m_audioDevice;
    //! Name of the audio driver
    QString m_audioDriver;

public:
    //! Audio latency constants (ms)
    enum
    {
        audioMinimumLatency = 1,
        audioDefaultLatency = 20,
        audioMaximumLatency = 100,
    };

private:
    //! Currently loaded FM bank
    FmBank              m_bank;
    //! Backup of currently loaded FM bank
    FmBank              m_bankBackup;

    //! Backup for melodic note while percusive mode is enabled
    int                 m_recentMelodicNote;

    //! Currently selected instrument
    FmBank::Instrument *m_curInst;
    //! Currently selected instrument
    FmBank::Instrument *m_curInstBackup;

    //! Recent index of instrument
    int m_recentNum;
    //! Recent flag of percussion
    bool m_recentPerc;

    //! Ignore all controls change events
    bool m_lock;

    //! OPL chip emulator frontent
    IRealtimeControl *m_generator = nullptr;

    //! Sound length measurer
    Measurer        *m_measurer;

    //! Recent bank file format which was been used
    BankFormats     m_recentFormat;

    //! Recent instrument file format which was been used
    InstFormats     m_recentInstFormat;

    /* ********** Audio output stuff ********** */
    typedef AudioOutRt AudioOutDefault;
    AudioOutDefault *m_audioOut = nullptr;

    /* ********** MIDI input stuff ********** */
    #ifdef ENABLE_MIDI
    MidiInRt        *m_midiIn = nullptr;
    QAction         *m_midiInAction = nullptr;
    #endif

#ifdef ENABLE_HW_OPL_PROXY
    /* ********** OPL proxy stuff ********** */
    Win9x_OPL_Proxy *m_proxyOpl = nullptr;
    unsigned m_proxyOplAddress = 0x388;
#endif

#ifdef ENABLE_HW_OPL_SERIAL_PORT
    OPL_SerialPortQt *m_serialPortOpl = nullptr;
    QString m_serialPortName;
    unsigned m_serialPortBaudRate = 115200;
    unsigned m_serialPortProtocol = 0;
#endif

    /*!
     * \brief Initializes audio subsystem and FM generator
     */
    void initAudio();

    #ifdef ENABLE_MIDI
    /*!
     * \brief Updates the available choices of MIDI inputs
     */
    void updateMidiInMenu();
    #endif

public:
    explicit BankEditor(QWidget *parent = 0);
    ~BankEditor();

    /**
     * @brief Loads recent application settings
     */
    void loadSettings();

    /**
     * @brief Saves recent application settings to the registry
     */
    void saveSettings();

private:
    /*!
     * \brief Initializes file data after file loading
     * \param filePath Path to just opened file
     */
    void initFileData(QString &filePath);

    /*!
     * \brief Reinitializes some data after file saving
     * \param filePath Path to just saved file
     */
    void reInitFileDataAfterSave(QString &filePath);

public:
    /*!
     * \brief Open file
     * \param filePath absolute path to the file to open
     * \param errp a location to receive the error code in case of failure
     * \return true if file successfully opened, false if failed
     */
    bool openFile(QString filePath, FfmtErrCode *errp = nullptr);

    /*!
     * \brief Open file or import it if it cannot be loaded as bank
     * \param filePath absolute path to the file to open
     * \return true if file successfully opened, false if failed
     */
    bool openOrImportFile(QString filePath);

    /*!
     * \brief Save bank file
     * \param filePath absolute path where save a file
     * \param format Target format to save a file
     * \return true if file successfully saved, false if failed
     */
    bool saveBankFile(QString filePath, BankFormats format);
    /*!
     * \brief Save current instrument file
     * \param filePath absolute path where to save a file
     * \param format Target format to save a file
     * \return true if file successfully saved, false if failed
     */
    bool saveInstrumentFile(QString filePath, InstFormats format);
    /*!
     * \brief Saves current bank file, asking for file path if necessary
     * \param optionalFilePath absolute path where to save a file, or empty string
     * \return true if file successfuly saved, false on rejecting or on fail
     */
    bool saveFileAs(const QString &optionalFilePath = QString());
    /*!
     * \brief Open Save-As dialog box for single instrument
     * \return true if file successfuly saved, false on rejecting or on fail
     */
    bool saveInstFileAs();

    /*!
     * \brief Checks was file modified or not. If file modified, asks saving.
     * \return true if no modifications detected or file saved or rejected. false if operation has been cancel
     */
    bool askForSaving();

    /* ************** Helpful functions ************** */
    /**
     * @brief Get the instrument name by index from off the current bank state
     * @param instrument id
     * @return The title of instrument
     */
    QString getInstrumentName(int instrument, bool isAuto = true, bool isPerc = false) const;
    /**
     * @brief Get the bank name by index from off the current bank state
     * @param bank id
     * @return The title of bank
     */
    QString getBankName(int bank, bool isAuto = true, bool isPerc = false);
    /**
     * @brief Get the selected MIDI specification from GUI
     */
    unsigned getSelectedMidiSpec() const;
    /**
     * @brief Loads current instrument into GUI controlls and sends it to generator
     */
    void flushInstrument();
    /**
     * @brief Synchronize instrument name in the list widget with actual instrument name in the data store
     */
    void syncInstrumentName();
    /**
     * @brief Synchronize instrument blank state in the list widget with actual instrument name in the data store
     */
    void syncInstrumentBlankness();
    /**
     * @brief Sets current instrument to editand test
     * @param num Number of instrument (from 0 to 127)
     * @param isPerc Use percusive set
     */
    void setCurrentInstrument(int num, bool isPerc);

    /**
     * @brief Set parameters of currently selected instrument into the GUI controlls
     */
    void loadInstrument();

    void displayDebugDelaysInfo();

    /**
     * @brief Reinitialize the OPL chip
     */
    void initChip();

    /**
     * @brief Send current instrument to OPL chip emulator for to test
     */
    void sendPatch();

    /**
     * @brief Disable/Enable melodic specific GUI controlls which are useless while editing of percussion instrument
     * @param dmode if true, most of melodic specific controlls (such as piano, note selector and chords) are will be disabled
     */
    void setDrumMode(bool dmode);

    bool isDrumsMode() const;

    void reloadBanks();

    void refreshBankName(int index);

    /**
     * @brief Creates the list of available languages
     */
    void createLanguageChoices();

private:
    /**
     * Path of the file which is currently edited
     */
    QString m_currentFilePath;
    /**
     * Format of the file which is currently edited
     */
    BankFormats m_currentFileFormat;

public slots:
    /**
     * @brief Toggle melodic mode and fill instruments list with melodic instruments names
     */
    void setMelodic();

    /**
     * @brief Toggle percussion mode and fill instruments list with percussion insruments names
     */
    void setDrums();

    /**
     * @brief Reload names of instruments in the list
     */
    void reloadInstrumentNames();

    /**
     * @brief Reload names of banks in the list
     */
    void reloadBankNames();

private slots:
    /* ***************** Common slots ***************** */
    /**
     * @brief When instrument list entry is selected
     * @param current Selected item from the instruments list, if NULL, turn into "unselected" mode and lock GUI
     * @param previous Unused, just pass NULL
     */
    void on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *);

    /**
     * @brief Toggle the chip emulator
     */
    void toggleEmulator();

    /**
     * @brief Clear all buffers and begin a new bank
     */
    void on_actionNew_triggered();
    /**
     * @brief Open existing bank file
     */
    void on_actionOpen_triggered();
    /**
     * @brief Save current bank state into the current file
     */
    void on_actionSave_triggered();
    /**
     * @brief Save current bank state into a selected file
     */
    void on_actionSaveAs_triggered();
    /**
     * @brief Save current instrument into the file
     */
    void on_actionSaveInstrument_triggered();
    /**
     * @brief Exit from the program
     */
    void on_actionExit_triggered();
    /**
     * @brief Copy current instrument into the clipboard
     */
    void on_actionCopy_triggered();
    /**
     * @brief Paste contents of clipboard into the current instrument
     */
    void on_actionPaste_triggered();
    /**
     * @brief Paste first vocice only as first voice
     */
    void on_actionPasteVoice11_triggered();
    /**
     * @brief Paste first vocice only as second voice
     */
    void on_actionPasteVoice12_triggered();
    /**
     * @brief Paste second vocice only as first voice
     */
    void on_actionPasteVoice21_triggered();
    /**
     * @brief Paste second vocice only as second voice
     */
    void on_actionPasteVoice22_triggered();
    /**
     * @brief Swap two voices of pseudo-4-operators instrument
     */
    void on_actionSwapVoices_triggered();
    /**
     * @brief Reset current instrument state to initial
     */
    void on_actionReset_current_instrument_triggered();

    /**
     * @brief Re-calculate all sounding delays
     */
    void on_actionReMeasure_triggered();

    /**
     * @brief Re-calculate sounding delays of the current instrument
     */
    void on_actionReMeasureOne_triggered();

    /**
     * @brief Run benchmark of all supported emulators basing on current instrument
     */
    void on_actionChipsBenchmark_triggered();

    /**
     * @brief Select another bank file and do a comparison
     */
    void on_actionCompareWith_triggered();

#if defined(ENABLE_PLOTS)
    /**
     * @brief Run the delay analysis of the current instrument
     */
    void on_actionDelayAnalysis_triggered();
#endif

    /**
     * @brief Show all supported formats list
     */
    void on_actionFormatsSup_triggered();

    /**
     * @brief Show about dialog
     */
    void on_actionAbout_triggered();

    /**
     * @brief Rename current bank
     */
    void on_bankRename_clicked();

    /**
     * @brief Reload instruments list for specific bank ID
     * @param index ID of bank
     */
    void on_bank_no_currentIndexChanged(int index);

    /**
     * @brief Read value from MSB field and write into bank meta-data entry
     */
    void on_bank_msb_valueChanged(int value);

    /**
     * @brief Read value from LSB field and write into bank meta-data entry
     */
    void on_bank_lsb_valueChanged(int value);

    /**
     * @brief Add new instrument into end
     */
    void on_actionAddInst_triggered();
    /**
     * @brief Clear currently selected instrument entry
     */
    void on_actionClearInstrument_triggered();
    /**
     * @brief Delete currently selected instrument entry
     */
    void on_actionDelInst_triggered();

    /**
     * @brief Add new bank entry (add 128 instruments into end)
     */
    void on_actionAddBank_triggered();
    /**
     * @brief Add new bank into end and copy all instruments from current bank into it
     */
    void on_actionCloneBank_triggered();
    /**
     * @brief Clear all instruments in current bank
     */
    void on_actionClearBank_triggered();
    /**
     * @brief Delete current bank (delete 128 instruments) except of last bank
     */
    void on_actionDeleteBank_triggered();
    /**
     * @brief Turn on the AdLib BNK mode where all instruments shown as united list without dividing by banks
     * @param checked AdLib BNK mode is turned on
     */
    void on_actionAdLibBnkMode_triggered(bool checked);
    /**
     * @brief Opens the audio configuration dialog
     */
    void on_actionAudioConfig_triggered();
#if defined(ENABLE_HW_OPL_PROXY) || defined(ENABLE_HW_OPL_SERIAL_PORT)
    /**
     * @brief Opens the hardware OPL dialog
     */
    void on_actionHardware_OPL_triggered();
#endif
    /**
     * @brief Changes the current language
     */
    void onActionLanguageTriggered();

    /* ***************** Instrument Parameters editing ***************** */

    void on_insName_textChanged(const QString &arg1);
    void on_insName_editingFinished();

    void on_deepTremolo_clicked(bool checked);
    void on_deepVibrato_clicked(bool checked);
    void on_volumeModel_currentIndexChanged(int index);
    void on_volumeSlider_valueChanged(int value);

    void on_feedback1_valueChanged(int arg1);
    void on_am1_clicked(bool checked);
    void on_fm1_clicked(bool checked);

    void on_percMode_currentIndexChanged(int index);
    void on_perc_noteNum_valueChanged(int arg1);
    void on_fixedNote_clicked(bool checked);

    void on_feedback2_valueChanged(int arg1);
    void on_am2_clicked(bool checked);
    void on_fm2_clicked(bool checked);

    void on_op4mode_clicked(bool checked);

    void on_doubleVoice_toggled(bool checked);
    void on_secVoiceFineTune_valueChanged(int arg1);

    void on_noteOffset1_valueChanged(int arg1);
    void on_noteOffset2_valueChanged(int arg1);

    void onOperatorChanged();

    void on_velocityOffset_valueChanged(int arg1);

    void on_pitchBendSlider_valueChanged(int value);
    void on_pitchBendSlider_sliderReleased();

    void on_holdButton_toggled(bool checked);

    /**
     * @brief Update after a change of any control value
     */
    void afterChangeControlValue();

    /**
     * @brief Adjusts the size of the window after it has been shown
     */
    void onBankEditorShown();

    #ifdef ENABLE_MIDI
    void on_midiIn_triggered(QAction *);
    void onMidiPortTriggered();
    #endif

private:
    /**
     * @brief Updates the text to display after a language change
     */
    void onLanguageChanged();

protected:
    virtual void pianoKeyPress(QKeyEvent *event);
    virtual void pianoKeyRelease(QKeyEvent *event);

private:
    /**
     * @brief Set of key codes currently active on piano
     */
    QSet<int> pianoKeyCodes;

protected:
    void closeEvent(QCloseEvent *event);
    void dragEnterEvent(QDragEnterEvent *e);
    void dropEvent(QDropEvent *e);
    virtual void keyPressEvent(QKeyEvent *event);
    virtual void keyReleaseEvent(QKeyEvent *event);
    void showEvent(QShowEvent *event);
    void changeEvent(QEvent *event);
    bool eventFilter(QObject *watched, QEvent *event);

private:
    Ui::BankEditor *ui;
};

#endif // BANK_EDITOR_H
