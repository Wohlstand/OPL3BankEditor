/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <cmath>
#include "ao_rtaudio.h"
#include "../opl/generator_realtime.h"

AudioOutRt::AudioOutRt(double latency, const std::string &device_name, QObject *parent)
    : QObject(parent)
{
    RtAudio *audioOut = new RtAudio(RtAudio::Api::UNSPECIFIED);
    m_audioOut.reset(audioOut);

    fprintf(stderr, "Using RtAudio API %d\n", audioOut->getCurrentApi());

    unsigned num_audio_devices = audioOut->getDeviceCount();
    if (num_audio_devices == 0) {
        QMessageBox::warning(
            nullptr, tr("Error"), tr("No audio devices are present for output."));
        qApp->exit(1);
        return;
    }

    unsigned outputDeviceId = ~0u;
    RtAudio::DeviceInfo deviceInfo;

    if(!device_name.empty())
    {
        for (unsigned i = 0; i < num_audio_devices; ++i)
        {
            deviceInfo = audioOut->getDeviceInfo(i);
            if (deviceInfo.name == device_name && isCompatibleDevice(deviceInfo))
                outputDeviceId = i;
        }
    }
    if(outputDeviceId == ~0u)
    {
        outputDeviceId = audioOut->getDefaultOutputDevice();
        deviceInfo = audioOut->getDeviceInfo(outputDeviceId);
    }

    unsigned sampleRate = deviceInfo.preferredSampleRate;
    if(sampleRate == 0)
    {
        qWarning() << "Returned zero sample rate. Using 44100...";
        sampleRate = 44100;
    }

    RtAudio::StreamParameters streamParam;
    streamParam.deviceId = outputDeviceId;
    streamParam.nChannels = 2;

    RtAudio::StreamOptions streamOpts;
    streamOpts.flags = RTAUDIO_ALSA_USE_DEFAULT;
    streamOpts.streamName = QCoreApplication::applicationName().toStdString();

    unsigned bufferSize = std::ceil(latency * sampleRate);
    qDebug() << "Desired latency" << latency;
    qDebug() << "Buffer size" << bufferSize;

    audioOut->openStream(
        &streamParam, nullptr, RTAUDIO_SINT16, sampleRate, &bufferSize,
        &process, this, &streamOpts, &errorCallback);
}

unsigned AudioOutRt::sampleRate() const
{
    return m_audioOut->getStreamSampleRate();
}

void AudioOutRt::start(IRealtimeProcess &rt)
{
    qDebug() << "Trying to start stream...";
    m_rt = &rt;
    m_audioOut->startStream();
    qDebug() << "Stream started!";
}

void AudioOutRt::stop()
{
    m_audioOut->stopStream();
    qDebug() << "Stream stopped!";
}

std::vector<std::string> AudioOutRt::listCompatibleDevices()
{
    std::vector<std::string> list;

    RtAudio *audioOut = m_audioOut.get();
    unsigned num_audio_devices = audioOut->getDeviceCount();

    for (unsigned i = 0; i < num_audio_devices; ++i)
    {
        RtAudio::DeviceInfo info = audioOut->getDeviceInfo(i);
        if (isCompatibleDevice(info))
            list.push_back(info.name);
    }

    return list;
}

int AudioOutRt::process(void *outputbuffer, void *, unsigned nframes, double, RtAudioStreamStatus, void *userdata)
{
    AudioOutRt *self = (AudioOutRt *)userdata;
    IRealtimeProcess &rt = *self->m_rt;
    rt.rt_generate((int16_t *)outputbuffer, nframes);
    return 0;
}

void AudioOutRt::errorCallback(RtAudioError::Type type, const std::string &errorText)
{
    qWarning() << "Audio error: " << errorText.c_str();
    //fprintf(stderr, "Audio error: %s\n", errorText.c_str());
    if (type != RtAudioError::WARNING && type != RtAudioError::DEBUG_WARNING)
        throw RtAudioError(errorText, type);
}

bool AudioOutRt::isCompatibleDevice(const RtAudio::DeviceInfo &info)
{
    // need at minimum 2 channels for output
    return info.outputChannels >= 2;
}
