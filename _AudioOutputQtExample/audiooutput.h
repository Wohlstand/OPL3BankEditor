/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef AUDIOOUTPUT_H
#define AUDIOOUTPUT_H

#include <math.h>
#include "dbopl.h"

#include <QAudioOutput>
#include <QByteArray>
#include <QComboBox>
#include <QIODevice>
#include <QLabel>
#include <QMainWindow>
#include <QObject>
#include <QSpinBox>
#include <QPushButton>
#include <QSlider>
#include <QTimer>

class Generator : public QIODevice
{
    Q_OBJECT

public:
    Generator(int sampleRate, QObject *parent);
    ~Generator();

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 bytesAvailable() const;

    void NoteOn(unsigned c, double hertz);
    void NoteOff(unsigned c);
    void Touch_Real(unsigned c, unsigned volume);
    void Touch(unsigned c, unsigned volume);
    void Patch(unsigned c, unsigned i);
    void Pan(unsigned c, unsigned value);
    void PlayNoteF(int noteID, int patch, int chan2op1 = 8, int chan2op2 = 7, int chan4op1 = 1, int chan4op2 = 4);
public slots:
    void PlayChord();
    void PlayNote();
    void MuteNote();
    void changePatch(int patch);

signals:
    void saySomething(QString text);

private:
    int note;
    int metainstr;
    DBOPL::Handler chip;
    qint64 m_pos;
    QByteArray m_buffer;
};

class AudioTest : public QMainWindow
{
    Q_OBJECT

public:
    AudioTest();
    ~AudioTest();

private:
    void initializeWindow();
    void initializeAudio();
    void createAudioOutput();

private:
    QTimer *m_pushTimer;

    // Owned by layout
    QPushButton *m_modeButton;
    QPushButton *m_suspendResumeButton;
    QComboBox *m_deviceBox;
    QLabel *m_volumeLabel;
    QSlider *m_volumeSlider;
    QLabel *m_counter;

    QSpinBox *m_patchChange;

    QPushButton *m_pokeNote;

    QAudioDeviceInfo m_device;
    Generator *m_generator;
    QAudioOutput *m_audioOutput;
    QIODevice *m_output; // not owned
    QAudioFormat m_format;

    bool m_pullMode;
    QByteArray m_buffer;

private slots:
    void pushTimerExpired();
    void toggleMode();
    void toggleSuspendResume();
    void deviceChanged(int index);
    void volumeChanged(int);
};

#endif // AUDIOOUTPUT_H
