/*
 * Interfaces over Yamaha OPL3 (YMF262) chip emulators
 *
 * Copyright (c) 2017-2023 Vitaly Novichkov (Wohlstand)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifdef ENABLE_HW_OPL_SERIAL_PORT

#include "opl_serial_port_qt.h"


OPL_SerialPortQt::OPL_SerialPortQt() :
    QObject(),
    OPL_SerialPort()
{}

OPL_SerialPortQt::~OPL_SerialPortQt()
{}

bool OPL_SerialPortQt::connectPort(const QString& name, unsigned int baudRate, unsigned int protocol)
{
    bool ret;
    m_protoMutex.lock();
    ret = OPL_SerialPort::connectPort(name.toStdString(), baudRate, protocol);
    m_protoMutex.unlock();
    return ret;
}

void OPL_SerialPortQt::writeReg(uint16_t addr, uint8_t data)
{
    QMetaObject::invokeMethod(this, "sendSerial",
                              Qt::QueuedConnection, Q_ARG(uint, addr), Q_ARG(uint, data));
}

OPLChipBase::ChipType OPL_SerialPortQt::chipType()
{
    m_protoMutex.lock();
    OPLChipBase::ChipType ret = OPL_SerialPort::chipType();
    m_protoMutex.unlock();
    return ret;
}

void OPL_SerialPortQt::sendSerial(uint addr, uint data)
{
    OPL_SerialPort::writeReg(addr, data);
}

#endif // ENABLE_HW_OPL_SERIAL_PORT
