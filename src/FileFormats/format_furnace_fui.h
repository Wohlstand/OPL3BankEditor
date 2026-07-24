/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2026 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef FORMAT_FURNACE_FUI_H
#define FORMAT_FURNACE_FUI_H

#include "ffmt_base.h"

/**
 * @brief Furnace Tracker single-instrument format (.fui)
 *
 * Reads and writes the "featural" (FINS) instrument files used by the Furnace
 * tracker, limited to the OPL (type 14) FM instrument type which maps directly
 * onto the OPL2/OPL3 operators handled by this editor.
 */
class FurnaceFUI final : public FmBankFormatBase
{
public:
    bool        detectInst(const QString &filePath, char *magic) override;
    FfmtErrCode loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0) override;
    FfmtErrCode saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum = false) override;
    int         formatInstCaps() const override;
    QString     formatInstName() const override;
    QString     formatInstModuleName() const override;
    QString     formatInstExtensionMask() const override;
    QString     formatInstDefaultExtension() const override;
    InstFormats formatInstId() const override;
};

#endif // FORMAT_FURNACE_FUI_H
