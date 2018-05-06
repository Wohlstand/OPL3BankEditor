/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifdef ENABLE_AUDIO_TESTING
#   ifdef USE_AUDIO_QTMM
#       include "audio/ao_qtmm.h"
#   endif
#   ifdef USE_AUDIO_ALSA
#       include "audio/ao_alsa.h"
#   endif
#endif

void BankEditor::initAudio()
{
    /*INIT AUDIO!!!*/
    int rate = 44100;
    int channels = 2;

    #   ifdef USE_AUDIO_ALSA
    if (!m_audioOut)
    {
        m_audioOut = new AudioOutALSA(this);
        if(!m_audioOut->init(rate, channels))
        {
            qWarning() << "Failed to initialize ALSA";
            delete m_audioOut;
        }
    }
    #endif

    #ifdef USE_AUDIO_QTMM
    if (!m_audioOut)
    {
        m_audioOut = new AudioOutQtMM(this);
        m_audioOut->init(rate, channels);
    }
    #endif

    m_generator = new Generator(uint32_t(rate), m_currentChip, this);
    #ifdef ENABLE_AUDIO_TESTING
    if (m_audioOut)
        m_audioOut->setAudioSource(m_generator);
    #endif

    //Test note
    connect(ui->testNote,  SIGNAL(pressed()),  m_generator,  SLOT(PlayNote()));
    connect(ui->testNote,  SIGNAL(released()), m_generator,  SLOT(NoteOffAllChans()));
    //Test major chord
    connect(ui->testMajor, SIGNAL(pressed()),  m_generator, SLOT(PlayMajorChord()));
    connect(ui->testMajor, SIGNAL(released()), m_generator, SLOT(NoteOffAllChans()));
    //Test minor chord
    connect(ui->testMinor, SIGNAL(pressed()),  m_generator, SLOT(PlayMinorChord()));
    connect(ui->testMinor, SIGNAL(released()), m_generator, SLOT(NoteOffAllChans()));
    //Test augmented chord
    connect(ui->testAugmented, SIGNAL(pressed()),  m_generator, SLOT(PlayAugmentedChord()));
    connect(ui->testAugmented, SIGNAL(released()), m_generator, SLOT(NoteOffAllChans()));
    //Test diminished chord
    connect(ui->testDiminished, SIGNAL(pressed()),  m_generator, SLOT(PlayDiminishedChord()));
    connect(ui->testDiminished, SIGNAL(released()), m_generator, SLOT(NoteOffAllChans()));
    //Test major 7-chord
    connect(ui->testMajor7, SIGNAL(pressed()),  m_generator, SLOT(PlayMajor7Chord()));
    connect(ui->testMajor7, SIGNAL(released()), m_generator, SLOT(NoteOffAllChans()));
    //Test minor 7-chord
    connect(ui->testMinor7, SIGNAL(pressed()),  m_generator, SLOT(PlayMinor7Chord()));
    connect(ui->testMinor7, SIGNAL(released()), m_generator, SLOT(NoteOffAllChans()));
    //Shut up that annoying noice!
    connect(ui->shutUp, SIGNAL(clicked()), m_generator, SLOT(Silence()));
    //Note to test
    #if QT_VERSION >= 0x050000
    connect(ui->noteToTest, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_generator, &Generator::changeNote);
    #else
    connect(ui->noteToTest, SIGNAL(valueChanged(int)), m_generator, SLOT(changeNote(int)));
    #endif
    m_generator->changeNote(ui->noteToTest->value());
    //Deep tremolo and vibrato
    connect(ui->deepVibrato,  SIGNAL(toggled(bool)), m_generator,  SLOT(changeDeepVibrato(bool)));
    connect(ui->deepTremolo,  SIGNAL(toggled(bool)), m_generator,  SLOT(changeDeepTremolo(bool)));
    //Generator's debug info
    connect(m_generator, SIGNAL(debugInfo(QString)), ui->debugBox, SLOT(setText(QString)));
    //Key pressed on piano bar
    #if QT_VERSION >= 0x050000
    connect(ui->piano, &Piano::gotNote,     ui->noteToTest, &QSpinBox::setValue);
    connect(ui->piano, &Piano::pressed,     m_generator,    &Generator::PlayNote);
    connect(ui->piano, &Piano::released,    m_generator,    &Generator::StopNote);
    #else
    connect(ui->piano, SIGNAL(gotNote(int)), ui->noteToTest, SLOT(setValue(int)));
    connect(ui->piano, SIGNAL(pressed()),   m_generator,    SLOT(PlayNote()));
    connect(ui->piano, SIGNAL(released()),  m_generator,    SLOT(NoteOffAllChans()));
    #endif
    //Piano on the importer dialog pressed
    m_importer->connect(m_importer->ui->piano, SIGNAL(gotNote(int)), ui->noteToTest, SLOT(setValue(int)));
    m_importer->connect(m_importer->ui->piano, SIGNAL(pressed()),    m_generator, SLOT(PlayNote()));
    m_importer->connect(m_importer->ui->piano, SIGNAL(released()),   m_generator, SLOT(StopNote()));
    //Test note button on the importer dialog box
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(pressed()),  m_generator,  SLOT(PlayNote()));
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(released()), m_generator,  SLOT(StopNote()));
    //Start generator!
    m_generator->start();
    #ifdef ENABLE_AUDIO_TESTING
    if (m_audioOut)
        m_audioOut->start();
    #endif
}

static bool keyToNote(int k, Generator *generator)
{
    bool pn = false;

    switch(k)
    {
    case Qt::Key_Z:
        generator->changeNote(48);
        pn = true;
        break;
    case Qt::Key_S:
        generator->changeNote(49);
        pn = true;
        break;
    case Qt::Key_X:
        generator->changeNote(50);
        pn = true;
        break;
    case Qt::Key_D:
        generator->changeNote(51);
        pn = true;
        break;
    case Qt::Key_C:
        generator->changeNote(52);
        pn = true;
        break;
    case Qt::Key_V:
        generator->changeNote(53);
        pn = true;
        break;
    case Qt::Key_G:
        generator->changeNote(54);
        pn = true;
        break;
    case Qt::Key_B:
        generator->changeNote(55);
        pn = true;
        break;
    case Qt::Key_H:
        generator->changeNote(56);
        pn = true;
        break;
    case Qt::Key_N:
        generator->changeNote(57);
        pn = true;
        break;
    case Qt::Key_J:
        generator->changeNote(58);
        pn = true;
        break;
    case Qt::Key_M:
        generator->changeNote(59);
        pn = true;
        break;
    case Qt::Key_Q:
    case Qt::Key_Comma:
        generator->changeNote(60);
        pn = true;
        break;
    case Qt::Key_2:
    case Qt::Key_L:
        generator->changeNote(61);
        pn = true;
        break;
    case Qt::Key_W:
    case Qt::Key_Period:
        generator->changeNote(62);
        pn = true;
        break;
    case Qt::Key_3:
    case Qt::Key_Semicolon:
        generator->changeNote(63);
        pn = true;
        break;
    case Qt::Key_E:
    case Qt::Key_Slash:
        generator->changeNote(64);
        pn = true;
        break;
    case Qt::Key_R:
        generator->changeNote(65);
        pn = true;
        break;
    case Qt::Key_5:
        generator->changeNote(66);
        pn = true;
        break;
    case Qt::Key_T:
        generator->changeNote(67);
        pn = true;
        break;
    case Qt::Key_6:
        generator->changeNote(68);
        pn = true;
        break;
    case Qt::Key_Y:
        generator->changeNote(69);
        pn = true;
        break;
    case Qt::Key_7:
        generator->changeNote(70);
        pn = true;
        break;
    case Qt::Key_U:
        generator->changeNote(71);
        pn = true;
        break;
    case Qt::Key_I:
        generator->changeNote(72);
        pn = true;
        break;
    case Qt::Key_9:
        generator->changeNote(73);
        pn = true;
        break;
    case Qt::Key_O:
        generator->changeNote(74);
        pn = true;
        break;
    case Qt::Key_0:
        generator->changeNote(75);
        pn = true;
        break;
    case Qt::Key_P:
        generator->changeNote(76);
        pn = true;
        break;
    default:
        break;
    }

    return pn;
}

void BankEditor::keyPressEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    if(ui->melodic->isChecked())
    {
        if(keyToNote(event->key(), m_generator))
            m_generator->PlayNote();
    }
    else
    {
        switch(event->key())
        {
        case Qt::Key_Space:
            m_generator->PlayNote();
            break;
        default:
            break;
        }
    }

    QMainWindow::keyPressEvent(event);
}

void BankEditor::keyReleaseEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

    if(ui->melodic->isChecked())
    {
        if(keyToNote(event->key(), m_generator))
            m_generator->StopNote();
    }
    else
    {
        switch(event->key())
        {
        case Qt::Key_Space:
            m_generator->NoteOffAllChans();
            break;
        default:
            break;
        }
    }

    QMainWindow::keyReleaseEvent(event);
}

#ifdef ENABLE_MIDI
void BankEditor::onMidiDataReceived(const unsigned char *data, size_t length)
{
    if(length == 3)
    {
        unsigned msg = data[0] >> 4;
        unsigned note = data[1] & 0x7f;
        unsigned vel = data[2] & 0x7f;

        if(msg == 0x9 && vel == 0)
            msg = 0x8;

        switch(msg) {
            case 0x8:
                m_generator->changeNote((int)note);
                m_generator->StopNote();
                break;
            case 0x9:
                m_generator->changeNote((int)note);
                m_generator->PlayNote();
                break;
        }
    }
}
#endif
