/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2020 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "opl_serial_port.h"

#ifdef ENABLE_HW_OPL_SERIAL_PORT

#include <QSerialPort>

OPL_SerialPort::OPL_SerialPort()
    : m_port(nullptr), m_protocol(ProtocolUnknown)
{
}

OPL_SerialPort::~OPL_SerialPort()
{
    delete m_port;
    m_port = nullptr;
}

bool OPL_SerialPort::connectPort(const QString &name, unsigned baudRate, unsigned protocol)
{
    delete m_port;
    m_port = nullptr;

    // ensure audio thread reads protocol atomically and in order,
    // so chipType() will be correct after the port is live
    m_protocol.storeRelease(protocol);

    QSerialPort *port = m_port = new QSerialPort(name);
    port->setBaudRate(baudRate);
    return port->open(QSerialPort::WriteOnly);
}

void OPL_SerialPort::writeReg(uint16_t addr, uint8_t data)
{
    QMetaObject::invokeMethod(
        this, "sendSerial", Qt::QueuedConnection, Q_ARG(uint, addr), Q_ARG(uint, data));
}

void OPL_SerialPort::sendSerial(uint addr, uint data)
{
    QSerialPort *port = m_port;
    if(!port || !port->isOpen())
        return;

    unsigned protocol = m_protocol.load();

    uint8_t sendBuffer[3];

    switch(protocol)
    {
    default:
    case ProtocolArduinoOPL2:
    {
        if(addr >= 0x100)
            break;
        sendBuffer[0] = (uint8_t)addr;
        sendBuffer[1] = (uint8_t)data;
        port->write((char *)sendBuffer, 2);
        break;
    }
    case ProtocolNukeYktOPL3:
    {
        sendBuffer[0] = (addr >> 6) | 0x80;
        sendBuffer[1] = ((addr & 0x3f) << 1) | (data >> 7);
        sendBuffer[2] = (data & 0x7f);
        port->write((char *)sendBuffer, 3);
        break;
    }
    }
}

void OPL_SerialPort::nativeGenerate(int16_t *frame)
{
    frame[0] = 0;
    frame[1] = 0;
}

const char *OPL_SerialPort::emulatorName()
{
    return "OPL Serial Port Driver";
}

OPLChipBase::ChipType OPL_SerialPort::chipType()
{
    unsigned protocol = m_protocol.loadAcquire();

    switch(protocol)
    {
    default:
    case ProtocolArduinoOPL2:
        return OPLChipBase::CHIPTYPE_OPL2;
    case ProtocolNukeYktOPL3:
        return OPLChipBase::CHIPTYPE_OPL3;
    }

}

#endif // ENABLE_HW_OPL_SERIAL_PORT
