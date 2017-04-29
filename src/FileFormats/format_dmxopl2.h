/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef DMXOPL2_H
#define DMXOPL2_H

#include "ffmt_base.h"

/**
 * @brief Reader and Writer of the DMX OP2 Bank format
 */
class DmxOPL2 final : public FmBankFormatBase
{
public:
    DmxOPL2();
    ~DmxOPL2() = default;

    enum DmxFlags
    {
        Dmx_FixedPitch  = 0x0001,
        Dmx_Unknown     = 0x0002,
        Dmx_DoubleVoice = 0x0004
    };
    bool detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    FfmtErrCode saveFile(QString filePath, FmBank &bank) override;
    int  formatCaps() override;
    QString formatName() override;
    QString formatExtensionMask() override;
    BankFormats formatId() override;
};

#endif // DMXOPL2_H
