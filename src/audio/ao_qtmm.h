/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2017 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef AO_QTMM_H
#define AO_QTMM_H

#include <QAudioOutput>
#include <QTimer>

#include "ao_base.h"

class AudioOutQtMM : public AudioOutBase
{
    Q_OBJECT

    //! Buffer for audio data transfering
    QByteArray       m_buffer;
    //! Timer to push audio data
    QTimer           m_pushTimer;

    //! Audio device spec
    QAudioDeviceInfo m_device;
    //! Audio output interface
    QAudioOutput    *m_audioOutput;
    //! Pointer to audio output interface
    QIODevice       *m_output;//not owned
    //! Audio format preferences
    QAudioFormat     m_format;

public:
    explicit AudioOutQtMM(QObject *parent);
    ~AudioOutQtMM();

    bool init(int sampleRate, int channels);
    void start();
    void stop();

private slots:
    /**
     * @brief Waveout playing callback
     */
    void pushTimerExpired();
};

#endif // AO_QTMM_H
