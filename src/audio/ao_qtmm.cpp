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

#include "ao_qtmm.h"

#include <QtDebug>

AudioOutQtMM::AudioOutQtMM(QObject *parent) :
    AudioOutBase(parent)
{
    connect(&m_pushTimer, SIGNAL(timeout()), SLOT(pushTimerExpired()));
}

AudioOutQtMM::~AudioOutQtMM()
{
    stop();
    m_pushTimer.stop();
    m_audioOutput->stop();
    delete m_audioOutput;
}

bool AudioOutQtMM::init(int sampleRate, int channels)
{
    qDebug() << "Initializing QAudioOutput...";

    m_device = QAudioDeviceInfo::defaultOutputDevice();

    m_format.setSampleRate(sampleRate);
    m_format.setChannelCount(channels);
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

    m_buffer.resize(8192);
    m_buffer.fill(0, 8192);

    qDebug() << "QAudioOutput is ready!";

    return true;
}

void AudioOutQtMM::start()
{
    m_output = m_audioOutput->start();
    m_pushTimer.start(4);
}

void AudioOutQtMM::stop()
{
    m_pushTimer.stop();
    m_audioOutput->stop();
}

void AudioOutQtMM::pushTimerExpired()
{
    if(m_audioOutput && m_audioOutput->state() != QAudio::StoppedState)
    {
        int chunks = m_audioOutput->bytesFree() / m_audioOutput->periodSize();

        while(chunks)
        {
            const qint64 len = m_audioSource->read(m_buffer.data(), m_audioOutput->periodSize());
            if(len)
                m_output->write(m_buffer.data(), len);
            if(len != m_audioOutput->periodSize())
                break;
            --chunks;
        }
    }
}
