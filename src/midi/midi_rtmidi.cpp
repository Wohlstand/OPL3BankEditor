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

#include "midi_rtmidi.h"
#include "../opl/generator_realtime.h"
#include <QCoreApplication>

MidiInRt::MidiInRt(IRealtimeMIDI &rt, QObject *parent)
    : QObject(parent), m_rt(rt)
{
}

MidiInRt::~MidiInRt()
{
    delete m_midiin;
    m_midiin = nullptr;
}

void MidiInRt::close()
{
    if(m_midiin)
        m_midiin->closePort();
}

bool MidiInRt::open(unsigned port)
{
    RtMidiIn *midiin = lazyInstance();
    m_midiin->closePort();
    m_errorSignaled = false;
    midiin->openPort(port, defaultPortName().toStdString());
    return !m_errorSignaled;
}

bool MidiInRt::openVirtual()
{
    RtMidiIn *midiin = lazyInstance();
    m_midiin->closePort();
    m_errorSignaled = false;
    midiin->openVirtualPort(defaultPortName().toStdString());
    return !m_errorSignaled;
}

bool MidiInRt::canOpenVirtual()
{
    RtMidiIn *midiin = lazyInstance();
    switch(midiin->getCurrentApi())
    {
    default:
        return true;
    case RtMidi::WINDOWS_MM:
    case RtMidi::RTMIDI_DUMMY:
        return false;
    }
}

bool MidiInRt::getPortList(QVector<QString> &ports)
{
    RtMidiIn *midiin = lazyInstance();
#if (QT_VERSION < 0x050000) && defined(_WIN32)
    RtMidi::Api api = midiin->getCurrentApi();
#endif

    m_errorSignaled = false;
    unsigned count = midiin->getPortCount();
    if(m_errorSignaled)
        return false;

    ports.resize(count);
    for(unsigned i = 0; i < count; ++i)
    {
        m_errorSignaled = false;
#if (QT_VERSION < 0x050000) && defined(_WIN32)
        if(api == RtMidi::WINDOWS_MM)
        {
            MIDIINCAPSA deviceCaps;
            midiInGetDevCapsA(i, &deviceCaps, sizeof(MIDIINCAPSA));
            ports[i] = QString::fromLocal8Bit(deviceCaps.szPname);
        }
        else
#endif
        {
            std::string name = midiin->getPortName(i);
            if(m_errorSignaled)
                return false;
            ports[i] = QString::fromStdString(name);
        }
    }
    return true;
}

QString MidiInRt::defaultClientName()
{
    return QCoreApplication::applicationName();
}
QString MidiInRt::defaultPortName()
{
    return QCoreApplication::applicationName() + " MIDI input";
}

RtMidiIn *MidiInRt::lazyInstance()
{
    RtMidiIn *midiin = m_midiin;
    if(!midiin) {
        unsigned bufferSize = 1024;
        midiin = m_midiin = new RtMidiIn(
            RtMidi::UNSPECIFIED, defaultClientName().toStdString(), bufferSize);
        midiin->setCallback(&onReceive, this);
        midiin->setErrorCallback(&onError, this);
    }
    return midiin;
}

void MidiInRt::onReceive(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
    MidiInRt *self = static_cast<MidiInRt *>(userData);
    (void)timeStamp;
    self->m_rt.midi_event(message->data(), message->size());
}

void MidiInRt::onError(RtMidiError::Type type, const std::string &errorText, void *userData)
{
    MidiInRt *self = static_cast<MidiInRt *>(userData);

    self->m_errorSignaled = true;
    self->m_errorCode = type;
    self->m_errorText = QString::fromStdString(errorText);
}
