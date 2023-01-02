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

#ifndef FORMAT_DRO_IMPORTER_H
#define FORMAT_DRO_IMPORTER_H

#include "ffmt_base.h"
class QFile;

/**
 * @brief Import FM instruments from DOSBox Raw OPL format
 */
class DRO_Importer : public FmBankFormatBase
{
public:
    bool        detect(const QString &filePath, char* magic) override;
    FfmtErrCode loadFile(QString filePath, FmBank &bank) override;
    int         formatCaps() const override;
    QString     formatName() const override;
    QString     formatModuleName() const override;
    QString     formatExtensionMask() const override;
    BankFormats formatId() const override;

private:
    FfmtErrCode loadFileV1(QFile &file, FmBank &bank);
    FfmtErrCode loadFileV2(QFile &file, FmBank &bank);
};

#endif // FORMAT_DRO_IMPORTER_H
