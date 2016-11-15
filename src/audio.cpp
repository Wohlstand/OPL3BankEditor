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

#include "bank_editor.h"
#include "ui_bank_editor.h"

#include "importer.h"
#include "ui_importer.h"

void BankEditor::initAudio()
{
    /*INIT AUDIO!!!*/
    m_device = QAudioDeviceInfo::defaultOutputDevice();
    connect(&m_pushTimer, SIGNAL(timeout()), SLOT(pushTimerExpired()));
    m_format.setSampleRate(44100);
    m_format.setChannelCount(2);
    m_format.setSampleSize(16);
    m_format.setCodec("audio/pcm");
    m_format.setByteOrder(QAudioFormat::LittleEndian);
    m_format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info(m_device);

    if(!info.isFormatSupported(m_format))
    {
        //qWarning() << "Default format not supported - trying to use nearest";
        m_format = info.nearestFormat(m_format);
    }

    m_audioOutput = new QAudioOutput(m_device, m_format, this);
    m_audioOutput->setVolume(1.0);
    m_generator = new Generator(44100, this);
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
    connect(ui->noteToTest, static_cast<void(QSpinBox::*)(int)>(&QSpinBox::valueChanged), m_generator, &Generator::changeNote);
    m_generator->changeNote(ui->noteToTest->value());
    //Deep tremolo and vibrato
    connect(ui->deepVibrato,  SIGNAL(toggled(bool)), m_generator,  SLOT(changeDeepVibrato(bool)));
    connect(ui->deepTremolo,  SIGNAL(toggled(bool)), m_generator,  SLOT(changeDeepTremolo(bool)));
    //Generator's debug info
    connect(m_generator, SIGNAL(debugInfo(QString)), ui->debugBox, SLOT(setText(QString)));
    //Key pressed on piano bar
    connect(ui->piano, &Piano::gotNote,     ui->noteToTest, &QSpinBox::setValue);
    connect(ui->piano, &Piano::pressed,     m_generator,    &Generator::PlayNote);
    connect(ui->piano, &Piano::released,    m_generator,    &Generator::NoteOffAllChans);
    //Piano on the importer dialog pressed
    m_importer->connect(m_importer->ui->piano, SIGNAL(gotNote(int)), ui->noteToTest, SLOT(setValue(int)));
    m_importer->connect(m_importer->ui->piano, SIGNAL(pressed()),    m_generator, SLOT(PlayNote()));
    m_importer->connect(m_importer->ui->piano, SIGNAL(released()),   m_generator, SLOT(NoteOffAllChans()));
    //Test note button on the importer dialog box
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(pressed()),  m_generator,  SLOT(PlayNote()));
    m_importer->connect(m_importer->ui->testNote,  SIGNAL(released()), m_generator,  SLOT(NoteOffAllChans()));
    //Start generator!
    m_generator->start();
    m_output = m_audioOutput->start();
    m_pushTimer.start(4);
}

void BankEditor::pushTimerExpired()
{
    if(m_audioOutput && m_audioOutput->state() != QAudio::StoppedState)
    {
        int chunks = m_audioOutput->bytesFree() / m_audioOutput->periodSize();

        while(chunks)
        {
            const qint64 len = m_generator->read(m_buffer.data(), m_audioOutput->periodSize());

            if(len)
                m_output->write(m_buffer.data(), len);

            if(len != m_audioOutput->periodSize())
                break;

            --chunks;
        }
    }
}

void BankEditor::keyPressEvent(QKeyEvent *event)
{
    if(event->isAutoRepeat())
        return;

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
            m_generator->NoteOffAllChans();
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
            m_generator->NoteOffAllChans();
            break;

        default:
            break;
        }
    }

    QMainWindow::keyReleaseEvent(event);
}
