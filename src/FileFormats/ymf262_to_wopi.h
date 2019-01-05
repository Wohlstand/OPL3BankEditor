/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2019 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef YMF262_TO_WOPI_HPP
#define YMF262_TO_WOPI_HPP

#include <stdint.h>
#include <memory>
#include <QSet>
#include <QList>
#include <QByteArray>

#include "../bank.h"

class RawYmf262ToWopi
{
    struct InstrumentData
    {
        QSet<QByteArray> cache;
        QList<FmBank::Instrument> caughtInstruments;
    };

    enum ChannelCategory
    {
        ChanCat_2op,
        ChanCat_4opMaster,
        ChanCat_4opSlave
    };

    struct Operator
    {
        uint8_t reg20;
        uint8_t reg40;
        uint8_t reg60;
        uint8_t reg80;
        uint8_t regE0;
    };

    struct Channel
    {
        uint8_t regA0;
        uint8_t regB0;
        uint8_t regC0;
        Operator *pair[2];
        ChannelCategory cat;
        Channel *buddy;
    };

    Channel m_channel[18];
    Operator m_operator[36];
    std::shared_ptr<InstrumentData> m_insdata;

public:
    RawYmf262ToWopi();
    void reset();
    void shareInstruments(RawYmf262ToWopi &other);
    void passReg(uint16_t addr, uint8_t val);
    void doAnalyzeState();
    const QList<FmBank::Instrument> &caughtInstruments();

private:
    void updateChannelRoles(uint8_t mask);
};

#endif
