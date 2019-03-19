/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <QObject>
#include <RtAudio.h>
#include <memory>

class IRealtimeProcess;

class AudioOutRt : public QObject
{
public:
    explicit AudioOutRt(double latency,
                        const std::string &device_name = std::string(),
                        QObject *parent = nullptr);
    unsigned sampleRate() const;
    void start(IRealtimeProcess &rt);
    void stop();
    std::vector<std::string> listCompatibleDevices();
private:
    static int process(void *outputbuffer, void *, unsigned nframes, double, RtAudioStreamStatus, void *userdata);
    static void errorCallback(RtAudioError::Type type, const std::string &errorText);
    static bool isCompatibleDevice(const RtAudio::DeviceInfo &info);
    IRealtimeProcess *m_rt = nullptr;
    std::unique_ptr<RtAudio> m_audioOut;
};
