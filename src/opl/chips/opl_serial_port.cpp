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

#include "opl_serial_port.h"

#ifdef ENABLE_HW_OPL_SERIAL_PORT

#include <QSerialPort>

static size_t retrowave_protocol_serial_pack(const uint8_t *buf_in, size_t len_in, uint8_t *buf_out)
{
    size_t in_cursor = 0;
    size_t out_cursor = 0;

    buf_out[out_cursor] = 0x00;
    out_cursor += 1;

    uint8_t shift_count = 0;

    while(in_cursor < len_in)
    {
        uint8_t cur_byte_out = buf_in[in_cursor] >> shift_count;
        if(in_cursor > 0)
            cur_byte_out |= (buf_in[in_cursor - 1] << (8 - shift_count));

        cur_byte_out |= 0x01;
        buf_out[out_cursor] = cur_byte_out;

        shift_count += 1;
        in_cursor += 1;
        out_cursor += 1;
        if(shift_count > 7)
        {
            shift_count = 0;
            in_cursor -= 1;
        }
    }

    if(shift_count)
    {
        buf_out[out_cursor] = buf_in[in_cursor - 1] << (8 - shift_count);
        buf_out[out_cursor] |= 0x01;
        out_cursor += 1;
    }

    buf_out[out_cursor] = 0x02;
    out_cursor += 1;

    return out_cursor;
}

OPL_SerialPort::OPL_SerialPort()
    : m_port(nullptr), m_protocol(ProtocolUnknown)
{}

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
    QMetaObject::invokeMethod(this, "sendSerial",
                              Qt::QueuedConnection, Q_ARG(uint, addr), Q_ARG(uint, data));
}

void OPL_SerialPort::sendSerial(uint addr, uint data)
{
    QSerialPort *port = m_port;

    if(!port || !port->isOpen())
        return;

    unsigned protocol = m_protocol.load();

    uint8_t sendBuffer[16];

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
    case ProtocolRetroWaveOPL3:
    {
        bool port1 = (addr & 0x100) != 0;
        uint8_t buf[8] =
        {
            static_cast<uint8_t>(0x21 << 1), 0x12,
            static_cast<uint8_t>(port1 ? 0xe5 : 0xe1), static_cast<uint8_t>(addr & 0xff),
            static_cast<uint8_t>(port1 ? 0xe7 : 0xe3), static_cast<uint8_t>(data),
            0xfb, static_cast<uint8_t>(data)
        };
        size_t packed_len = retrowave_protocol_serial_pack(buf, sizeof(buf), sendBuffer);
        port->write((char *)sendBuffer, (qint64)packed_len);
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
    case ProtocolRetroWaveOPL3:
        return OPLChipBase::CHIPTYPE_OPL3;
    }

}

#endif // ENABLE_HW_OPL_SERIAL_PORT
