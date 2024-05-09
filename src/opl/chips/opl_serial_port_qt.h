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


#ifndef OPL_SERIAL_PORT_QT_H
#define OPL_SERIAL_PORT_QT_H

#ifdef ENABLE_HW_OPL_SERIAL_PORT

#include "opl_serial_port.h"
#include <QObject>
#include <QMutex>

class OPL_SerialPortQt : public QObject, public OPL_SerialPort
{
    Q_OBJECT

public:
    OPL_SerialPortQt();
    virtual ~OPL_SerialPortQt() override;

    bool connectPort(const QString &name, unsigned baudRate, unsigned protocol);

    void writeReg(uint16_t addr, uint8_t data) override;

    ChipType chipType() override;

private slots:
    void sendSerial(uint addr, uint data);

private:
    QMutex m_protoMutex;
};

#endif // ENABLE_HW_OPL_SERIAL_PORT

#endif // OPL_SERIAL_PORT_QT_H
