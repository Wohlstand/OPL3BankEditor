/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2021 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "bank_editor.h"
#include "ui_bank_editor.h"

#include "importer.h"
#include "ui_importer.h"

#include <QtDebug>

void BankEditor::initAudio()
{
    qDebug() << "Init audioOut...";
    m_audioOut = new AudioOutDefault(m_audioLatency * 1e-3, m_audioDevice.toStdString(), m_audioDriver.toStdString(), this);
    qDebug() << "Init Generator...";
    std::shared_ptr<Generator> generator(
        new Generator(uint32_t(m_audioOut->sampleRate()), m_currentChip));
    qDebug() << "Init Rt-Generator...";
    RealtimeGenerator *rtgenerator = new RealtimeGenerator(generator, this);
    qDebug() << "Seting pointer of RT Generator...";
    m_generator = rtgenerator;

    qDebug() << "Init signals and slots of keys...";
    //Test note
    connect(ui->testNote,  SIGNAL(pressed()),  m_generator,  SLOT(ctl_playNote()));
    connect(ui->testNote,  SIGNAL(released()), m_generator,  SLOT(ctl_noteOffAllChans()));
    //Test major chord
    connect(ui->testMajor, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMajorChord()));
    connect(ui->testMajor, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test minor chord
    connect(ui->testMinor, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMinorChord()));
    connect(ui->testMinor, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test augmented chord
    connect(ui->testAugmented, SIGNAL(pressed()),  m_generator, SLOT(ctl_playAugmentedChord()));
    connect(ui->testAugmented, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test diminished chord
    connect(ui->testDiminished, SIGNAL(pressed()),  m_generator, SLOT(ctl_playDiminishedChord()));
    connect(ui->testDiminished, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test major 7-chord
    connect(ui->testMajor7, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMajor7Chord()));
    connect(ui->testMajor7, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Test minor 7-chord
    connect(ui->testMinor7, SIGNAL(pressed()),  m_generator, SLOT(ctl_playMinor7Chord()));
    connect(ui->testMinor7, SIGNAL(released()), m_generator, SLOT(ctl_noteOffAllChans()));
    //Shut up that annoying noice!
    connect(ui->shutUp, SIGNAL(clicked()), m_generator, SLOT(ctl_silence()));
    //Note to test
#if QT_VERSION >= 0x050000
    connect(ui->noteToTest, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_generator, &IRealtimeControl::changeNote);
#else
    connect(ui->noteToTest, SIGNAL(valueChanged(int)), m_generator, SLOT(changeNote(int)));
#endif
    m_generator->changeNote((unsigned)ui->noteToTest->value());
    //Deep tremolo and vibrato
    connect(ui->deepVibrato,  SIGNAL(toggled(bool)), m_generator,  SLOT(ctl_changeDeepVibrato(bool)));
    connect(ui->deepTremolo,  SIGNAL(toggled(bool)), m_generator,  SLOT(ctl_changeDeepTremolo(bool)));
    connect(ui->volumeModel,  SIGNAL(currentIndexChanged(int)), m_generator,  SLOT(ctl_changeVolumeModel(int)));
    //Generator's debug info
    connect(m_generator, SIGNAL(debugInfo(QString)), ui->debugBox, SLOT(setText(QString)));
    //Key pressed on piano bar
#if QT_VERSION >= 0x050000
    connect(ui->piano, &Piano::gotNote,     ui->noteToTest, &QSpinBox::setValue);
    connect(ui->piano, &Piano::pressed,     m_generator,    &IRealtimeControl::ctl_playNote);
    connect(ui->piano, &Piano::released,    m_generator,    &IRealtimeControl::ctl_stopNote);
#else
    connect(ui->piano, SIGNAL(gotNote(int)), ui->noteToTest, SLOT(setValue(int)));
    connect(ui->piano, SIGNAL(pressed()),   m_generator,    SLOT(ctl_playNote()));
    connect(ui->piano, SIGNAL(released()),  m_generator,    SLOT(ctl_stopNote()));
#endif
    //Piano on the importer dialog pressed
    m_importer->connect(m_importer->ui->piano, SIGNAL(gotNote(int)), ui->noteToTest, SLOT(setValue(int)));
    m_importer->connect(m_importer->ui->piano, SIGNAL(pressed()),    m_generator, SLOT(ctl_playNote()));
    m_importer->connect(m_importer->ui->piano, SIGNAL(released()),   m_generator, SLOT(ctl_stopNote()));
    //Test note button on the importer dialog box
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(pressed()),  m_generator,  SLOT(ctl_playNote()));
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(released()), m_generator,  SLOT(ctl_stopNote()));

    qDebug() << "Trying to start audio... (with dereferencing of RtGenerator!)";
    //Start generator!
    m_audioOut->start(*rtgenerator);

#ifdef ENABLE_MIDI
    qDebug() << "Trying to init MIDI-IN...";
    m_midiIn = new MidiInRt(*rtgenerator, this);
#endif
}

static int keyToNote(int k)
{
    int note = -1;

    switch(k)
    {
    case Qt::Key_Z:
        note = 48;
        break;
    case Qt::Key_S:
        note = 49;
        break;
    case Qt::Key_X:
        note = 50;
        break;
    case Qt::Key_D:
        note = 51;
        break;
    case Qt::Key_C:
        note = 52;
        break;
    case Qt::Key_V:
        note = 53;
        break;
    case Qt::Key_G:
        note = 54;
        break;
    case Qt::Key_B:
        note = 55;
        break;
    case Qt::Key_H:
        note = 56;
        break;
    case Qt::Key_N:
        note = 57;
        break;
    case Qt::Key_J:
        note = 58;
        break;
    case Qt::Key_M:
        note = 59;
        break;
    case Qt::Key_Q:
    case Qt::Key_Comma:
        note = 60;
        break;
    case Qt::Key_2:
    case Qt::Key_L:
        note = 61;
        break;
    case Qt::Key_W:
    case Qt::Key_Period:
        note = 62;
        break;
    case Qt::Key_3:
    case Qt::Key_Semicolon:
        note = 63;
        break;
    case Qt::Key_E:
    case Qt::Key_Slash:
        note = 64;
        break;
    case Qt::Key_R:
        note = 65;
        break;
    case Qt::Key_5:
        note = 66;
        break;
    case Qt::Key_T:
        note = 67;
        break;
    case Qt::Key_6:
        note = 68;
        break;
    case Qt::Key_Y:
        note = 69;
        break;
    case Qt::Key_7:
        note = 70;
        break;
    case Qt::Key_U:
        note = 71;
        break;
    case Qt::Key_I:
        note = 72;
        break;
    case Qt::Key_9:
        note = 73;
        break;
    case Qt::Key_O:
        note = 74;
        break;
    case Qt::Key_0:
        note = 75;
        break;
    case Qt::Key_P:
        note = 76;
        break;
    default:
        break;
    }

    return note;
}

void BankEditor::pianoKeyPress(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    if(ui->melodic->isChecked())
    {
        int key = event->key();
        int note = keyToNote(key);
        if(note != -1 && !pianoKeyCodes.contains(key)) {
            m_generator->changeNote(note);
            m_generator->ctl_playNote();
            pianoKeyCodes.insert(key);
            ui->piano->setNoteHighlighted(note, true);
        }
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
}

void BankEditor::pianoKeyRelease(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    if(ui->melodic->isChecked())
    {
        int key = event->key();
        int note = keyToNote(key);
        if(note != -1 && pianoKeyCodes.contains(key)) {
            m_generator->changeNote(note);
            m_generator->ctl_stopNote();
            pianoKeyCodes.remove(key);
            ui->piano->setNoteHighlighted(note, false);
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
}

void BankEditor::keyPressEvent(QKeyEvent *event)
{
    pianoKeyPress(event);
    QMainWindow::keyPressEvent(event);
}

void BankEditor::keyReleaseEvent(QKeyEvent *event)
{
    pianoKeyRelease(event);
    QMainWindow::keyReleaseEvent(event);
}
