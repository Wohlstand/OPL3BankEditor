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

#ifndef FMBANKFORMATBASE_H
#define FMBANKFORMATBASE_H

#include <QString>
#include "../bank.h"

/*!
 * \brief Base class provides errors enum and commonly used headers
 */
class FmBankFormatBase
{
protected:
    static void registerBankFormat(FmBankFormatBase* format);
    static void registerInstFormat(FmBankFormatBase *format);
public:
    static void registerAllFormats();

    FmBankFormatBase();
    virtual ~FmBankFormatBase();
    /**
     * @brief The bank formats enum
     */
    enum Formats
    {
        FORMAT_UNKNOWN = -1,

        FORMAT_JUNGLEVIZION =   0,
        FORMAT_DMX_OP2      =   1,
        FORMAT_APOGEE       =   2,
        FORMAT_IBK          =   3,
        FORMAT_ADLIB_BKN1   =   4,
        FORMAT_ADLIB_BKNHMI =   5,
        FORMAT_MILES        =   6,
        FORMAT_SB2OP         =   7,
        FORMAT_SB4OP        =   8,

        FORMATS_END,
        FORMATS_BEGIN = FORMAT_JUNGLEVIZION,
    };

    enum InsFormats
    {
        FORMAT_INST_UNKNOWN = -1,
        FORMAT_INST_SBI     = 0,
        FORMAT_INST_SBIex   = 1,
    };

    enum FormatCaps
    {
        FORMAT_CAPS_NOTHING = 0x00,
        FORMAT_CAPS_OPEN    = 0x01,
        FORMAT_CAPS_SAVE    = 0x02,
        FORMAT_CAPS_IMPORT  = 0x04,
        FORMAT_CAPS_EVERYTHING = FORMAT_CAPS_OPEN|FORMAT_CAPS_SAVE|FORMAT_CAPS_IMPORT
    };

    static QString getSaveFiltersList();
    static QString getOpenFiltersList(bool import = false);
    static QString getInstOpenFiltersList(bool import = false);

    static Formats getFormatFromFilter(QString filter);
    static QString getFilterFromFormat(Formats format, int requiredCaps);

    virtual bool detect(const QString &filePath, char* magic);
    virtual bool detectInst(const QString &filePath, char* magic);
    virtual int  loadFile(QString filePath, FmBank &bank);
    virtual int  saveFile(QString filePath, FmBank &bank);
    virtual int  loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum = 0);
    virtual int  saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum = false);
    virtual int     formatCaps();
    virtual QString formatName();
    virtual QString formatExtensionMask();
    virtual Formats formatId();

    virtual int         formatInstCaps();
    virtual QString     formatInstName();
    virtual QString     formatInstExtensionMask();
    virtual InsFormats  formatInstId();

    /**
     * @brief Is given format designed for the instrument importing only
     * @param format file format identificator
     * @return true if this format is instrument import only
     */
    static bool isImportOnly(Formats format);

    /**
     * @brief Error codes
     */
    enum ErrCode
    {
        //! Everything is OK
        ERR_OK=0,
        //! File wasn't opened because not exists or permission denied
        ERR_NOFILE,
        //! File format is corrupted/invalid/damaged
        ERR_BADFORMAT,
        //! Reading or Writing operation is not implemented for this file format
        ERR_NOT_IMLEMENTED,
        //! Detected file format is not supported
        ERR_UNSUPPORTED_FORMAT,
        //! Any other error
        ERR_UNKNOWN
    };

    static int  OpenBankFile(QString filePath, FmBank &bank, Formats *recent=0);
    static int  ImportBankFile(QString filePath, FmBank &bank, Formats *recent=0);
    static int  SaveBankFile(QString filePath, FmBank &bank, Formats dest);
    static int  OpenInstrumentFile(QString filePath, FmBank::Instrument &ins, InsFormats *recent=0, bool *isDrum = 0);
};

#endif // FMBANKFORMATBASE_H
