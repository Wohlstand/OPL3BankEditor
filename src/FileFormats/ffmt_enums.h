/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2025 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef FFMT_ENUMS_H
#define FFMT_ENUMS_H

/**
 * @brief The bank formats enum
 */
enum class BankFormats
{
    FORMAT_UNKNOWN = -1,

    FORMAT_ADLIB_BKN1,
    FORMAT_ADLIB_BKNHMI,
    FORMAT_ADLIB_BKNHMI_DRUMS,
    FORMAT_ADLIB_TIM,
    FORMAT_ADLIBGOLD_BNK2,
    FORMAT_AIL2,
    FORMAT_APOGEE,
    FORMAT_BISQWIT,
    FORMAT_DMX_OP2,
    FORMAT_IBK,
    FORMAT_IBK_DRUMS,
    FORMAT_JUNGLEVIZION,
    FORMAT_SB2OP,
    FORMAT_SB2OP_DRUMS,
    FORMAT_SB4OP,
    FORMAT_SB4OP_DRUMS,
    FORMAT_WOHLSTAND_OPL3,
    FORMAT_WOHLSTAND_OPL3_GM,
    FORMAT_WOHLSTAND_OPL3_TEXT,
    FORMAT_FLATBUFFER_OPL3,
    FORMAT_PATCH_FM4,

    FORMAT_IMF_IMPORTER,
    FORMAT_CMF_IMPORTER,
    FORMAT_RAD_IMPORTER,
    FORMAT_DRO_IMPORTER,
    FORMAT_VGM_IMPORTER,
    FORMAT_SMAF_IMPORTER,
    FORMAT_KLM_IMPORTER,

    FORMATS_END,
    FORMATS_BEGIN = FORMAT_ADLIB_BKN1,
    FORMATS_DEFAULT_FORMAT = FORMAT_WOHLSTAND_OPL3,
};

enum class InstFormats
{
    FORMAT_INST_UNKNOWN = -1,
    FORMAT_INST_WOPL3 = 0,
    FORMAT_INST_WOPLX,
    FORMAT_INST_SBI,
    FORMAT_INST_SBIex,
    FORMAT_INST_ADLIB_INS,
    FORMAT_INST_SGI,
    FORMAT_INST_CIF,
    FORMAT_INST_HSC,
};

enum class FormatCaps
{
    //! No capabilities, dummy
    FORMAT_CAPS_NOTHING = 0x00,
    //! Can be opened for editing
    FORMAT_CAPS_OPEN    = 0x01,
    //! Can be saved
    FORMAT_CAPS_SAVE    = 0x02,
    //! Can be opened to import data into currently opened bank
    FORMAT_CAPS_IMPORT  = 0x04,
    //! On every save the delay measures will be generated
    FORMAT_CAPS_NEEDS_MEASURE = 0x08,
    //! Format is able to keep only one bank only
    FORMAT_CAPS_GM_BANK = 0x10,
    //! Format allows to save melodic only
    FORMAT_CAPS_MELODIC_ONLY = 0x20,
    //! Format allows to save percussion only
    FORMAT_CAPS_PERCUSSION_ONLY = 0x40,

    //! Open/Save/Import capabilities
    FORMAT_CAPS_EVERYTHING = FORMAT_CAPS_OPEN|FORMAT_CAPS_SAVE|FORMAT_CAPS_IMPORT,
    //! Open/Save/Import capabilities
    FORMAT_CAPS_EVERYTHING_GM = FORMAT_CAPS_OPEN|FORMAT_CAPS_SAVE|FORMAT_CAPS_IMPORT|FORMAT_CAPS_GM_BANK
};

/**
 * @brief Error codes
 */
enum class FfmtErrCode
{
    //! Everything is OK
    ERR_OK=0,
    //! File wasn't opened because not exists or permission denied
    ERR_NOFILE,
    //! File format is corrupted/invalid/damaged
    ERR_BADFORMAT,
    //! Reading or Writing operation is not implemented for this file format
    ERR_NOT_IMPLEMENTED,
    //! Detected file format is not supported
    ERR_UNSUPPORTED_FORMAT,
    //! Any other error
    ERR_UNKNOWN
};

class QString;
namespace FileFormats
{
    /**
     * @brief Obtain a translated text from an error code.
     */
    QString getErrorText(FfmtErrCode err);
}

#endif // FFMT_ENUMS_H
