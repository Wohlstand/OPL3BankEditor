/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2024 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef FORMAT_MISC_HSC_H
#define FORMAT_MISC_HSC_H

#include "ffmt_base.h"

/**
 * @brief Reader and Writer of the HSC format used by HSC tracker and Adlib Tracker II
 */
class Misc_HSC final : public FmBankFormatBase
{
public:
    bool        detectInst(const QString &filePath, char* magic) override;
    FfmtErrCode loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = nullptr) override;
    FfmtErrCode saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum = false) override;
    int         formatInstCaps() const override;
    QString     formatInstName() const override;
    QString     formatInstExtensionMask() const override;
    QString     formatInstDefaultExtension() const override;
    InstFormats formatInstId() const override;
};

#endif // FORMAT_MISC_HSC_H
