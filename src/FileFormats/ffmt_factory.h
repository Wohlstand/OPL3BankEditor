/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2022 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef FFMT_FACTORY_H
#define FFMT_FACTORY_H

#include <QString>
#include "../bank.h"
#include "ffmt_base.h"

class FmBankFormatFactory
{
public:
    static void registerAllFormats();

    static QString getSaveFiltersList();
    static QString getOpenFiltersList(bool import = false);
    static QString getInstOpenFiltersList(bool import = false);
    static QString getInstSaveFiltersList();

    static QList<const FmBankFormatBase *> allBankFormats();
    static QList<const FmBankFormatBase *> allInstrumentFormats();

    static BankFormats  getFormatFromFilter(QString filter);
    static QString      getFilterFromFormat(BankFormats format, int requiredCaps);

    static InstFormats  getInstFormatFromFilter(QString filter);
    static QString      getInstFilterFromFormat(InstFormats format, int requiredCaps);

    /**
     * @brief Is given format designed for the instrument importing only
     * @param format file format identificator
     * @return true if this format is instrument import only
     */
    static bool isImportOnly(BankFormats format);
    static bool hasCaps(BankFormats format, int capsQuery);
    static QString formatName(BankFormats format);
    static FfmtErrCode OpenBankFile(QString filePath, FmBank &bank, BankFormats *recent = nullptr);
    static FfmtErrCode ImportBankFile(QString filePath, FmBank &bank, BankFormats *recent = nullptr);
    static FfmtErrCode SaveBankFile(QString &filePath, FmBank &bank, BankFormats dest);
    static FfmtErrCode OpenInstrumentFile(QString filePath, FmBank::Instrument &ins, InstFormats *recent=0, bool *isDrum = 0, bool import = false);
    static FfmtErrCode SaveInstrumentFile(QString &filePath, FmBank::Instrument &ins, InstFormats format, bool isDrum);
};

#endif // FFMT_FACTORY_H
