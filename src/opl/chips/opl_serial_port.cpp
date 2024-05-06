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

#ifdef __linux__
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <unistd.h>
#endif


class ChipSerialPort
{
    std::string m_portName;

#ifdef __unix__
    int m_port;
    struct termios m_portSetup;

    static unsigned int baud2enum(unsigned int baud)
    {
        if(baud == 0)
            return B0;
        else if(baud <= 50)
            return B50;
        else if(baud <= 75)
            return B75;
        else if(baud <= 110)
            return B110;
        else if(baud <= 134)
            return B134;
        else if(baud <= 150)
            return B150;
        else if(baud <= 200)
            return B200;
        else if(baud <= 300)
            return B300;
        else if(baud <= 600)
            return B600;
        else if(baud <= 1200)
            return B1200;
        else if(baud <= 1800)
            return B1800;
        else if(baud <= 2400)
            return B2400;
        else if(baud <= 4800)
            return B4800;
        else if(baud <= 9600)
            return B9600;
        else if(baud <= 19200)
            return B19200;
        else if(baud <= 38400)
            return B38400;
        else if(baud <= 57600)
            return B57600;
        else if(baud <= 115200)
            return B115200;
        else
            return B230400;
    }
#endif

public:
    ChipSerialPort()
    {
#ifdef __linux__
        m_port = 0;
        memset(&m_portSetup, 0, sizeof(struct termios));
#endif
    }

    ~ChipSerialPort()
    {
        close();
    }

    bool isOpen()
    {
        return m_port != 0;
    }

    void close()
    {
        if(m_port)
            ::close(m_port);

        m_port = 0;
    }

    bool open(const std::string &portName, unsigned baudRate)
    {
        if(m_port)
            this->close();

        std::string portPath = "/dev/" + portName;
        m_port = ::open(portPath.c_str(), O_WRONLY);

        if(m_port < 0)
        {
            m_port = 0;
            return false;
        }

        if(tcgetattr(m_port, &m_portSetup) != 0)
        {
            close();
            return false;
        }

        cfsetospeed(&m_portSetup, baud2enum(baudRate));

        if(tcsetattr(m_port, TCSANOW, &m_portSetup) != 0)
        {
            close();
            return false;
        }

        return true;
    }

    int write(uint8_t *data, size_t size)
    {
        if(!m_port)
            return 0;

        return ::write(m_port, data, size);
    }
};



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

    // QSerialPort *port = m_port = new QSerialPort(name);
    // port->setBaudRate(baudRate);
    // return port->open(QSerialPort::WriteOnly);

    m_port = new ChipSerialPort();
    return m_port->open(name.toStdString(), baudRate);
}

void OPL_SerialPort::writeReg(uint16_t addr, uint8_t data)
{
    QMetaObject::invokeMethod(this, "sendSerial",
                              Qt::QueuedConnection, Q_ARG(uint, addr), Q_ARG(uint, data));
}

void OPL_SerialPort::sendSerial(uint addr, uint data)
{
    ChipSerialPort *port = m_port;

    if(!port || !port->isOpen())
        return;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    unsigned protocol = m_protocol.loadRelaxed();
#else
    unsigned protocol = m_protocol.load();
#endif

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
        port->write(sendBuffer, 2);
        break;
    }
    case ProtocolNukeYktOPL3:
    {
        sendBuffer[0] = (addr >> 6) | 0x80;
        sendBuffer[1] = ((addr & 0x3f) << 1) | (data >> 7);
        sendBuffer[2] = (data & 0x7f);
        port->write(sendBuffer, 3);
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
        port->write(sendBuffer, packed_len);
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
