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

#include "midi_rtmidi.h"
#include <QCoreApplication>
#include <RtMidi.h>

MidiInRt::MidiInRt(QObject *parent)
    : QObject(parent)
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

void MidiInRt::open(unsigned port)
{
    RtMidiIn *midiin = lazyInstance();
    close();
    midiin->openPort(port, "MIDI input");
}

void MidiInRt::openVirtual()
{
    RtMidiIn *midiin = lazyInstance();
    close();
    midiin->openVirtualPort("MIDI input");
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

QVector<QString> MidiInRt::getPortList()
{
    RtMidiIn *midiin = lazyInstance();
    unsigned count = midiin->getPortCount();
    QVector<QString> ports(count);
    for(unsigned i = 0; i < count; ++i)
        ports[i] = QString::fromStdString(midiin->getPortName(i));
    return ports;
}

RtMidiIn *MidiInRt::lazyInstance()
{
    RtMidiIn *midiin = m_midiin;
    if(!midiin) {
        const QString &name = QCoreApplication::applicationName();
        unsigned bufferSize = 1024;
        midiin = m_midiin = new RtMidiIn(RtMidi::UNSPECIFIED, name.toStdString(), bufferSize);
        midiin->setCallback(&onReceive, this);
    }
    return midiin;
}

void MidiInRt::onReceive(double timeStamp, std::vector<unsigned char> *message, void *userData)
{
    MidiInRt *self = static_cast<MidiInRt *>(userData);
    (void)timeStamp;

    Q_EMIT self->midiDataReceived(message->data(), message->size());
}
