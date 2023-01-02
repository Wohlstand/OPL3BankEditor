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

#ifndef MIDI_RTMIDI_H
#define MIDI_RTMIDI_H
#ifdef ENABLE_MIDI

#include <QObject>
#include <QVector>
#include <RtMidi.h>

class IRealtimeMIDI;

class MidiInRt : public QObject
{
public:
    explicit MidiInRt(IRealtimeMIDI &rt, QObject *parent = nullptr);
    ~MidiInRt();
    void close();
    bool open(unsigned port);
    bool openVirtual();
    bool canOpenVirtual();
    bool getPortList(QVector<QString> &ports);

    static QString defaultClientName();
    static QString defaultPortName();

    const QString &getErrorText() const
    {
        return m_errorText;
    }

private:
    IRealtimeMIDI &m_rt;
    RtMidiIn *m_midiin = nullptr;
    bool m_errorSignaled = false;
    RtMidiError::Type m_errorCode = RtMidiError::UNSPECIFIED;
    QString m_errorText;
    RtMidiIn *lazyInstance();
    static void onReceive(double timeStamp, std::vector<unsigned char> *message, void *userData);
    static void onError(RtMidiError::Type type, const std::string &errorText, void *userData);
};

#endif
#endif
