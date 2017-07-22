#ifndef FFMT_ENUMS_H
#define FFMT_ENUMS_H

/**
 * @brief The bank formats enum
 */
enum class BankFormats
{
    FORMAT_UNKNOWN = -1,

    FORMAT_JUNGLEVIZION         =   0,
    FORMAT_DMX_OP2              =   1,
    FORMAT_APOGEE               =   2,
    FORMAT_IBK                  =   3,
    FORMAT_ADLIB_BKN1           =   4,
    FORMAT_ADLIB_BKNHMI         =   5,
    FORMAT_ADLIB_BKNHMI_DRUMS   =   6,
    FORMAT_MILES                =   7,
    FORMAT_SB2OP                =   8,
    FORMAT_SB2OP_DRUMS          =   9,
    FORMAT_SB4OP                =   10,
    FORMAT_SB4OP_DRUMS          =   11,
    FORMAT_IMF_IMPORTER         =   12,
    FORMAT_CMF_IMPORTER         =   13,
    FORMAT_BISQWIT              =   14,

    FORMATS_END,
    FORMATS_BEGIN = FORMAT_JUNGLEVIZION,
};

enum class InstFormats
{
    FORMAT_INST_UNKNOWN = -1,
    FORMAT_INST_SBI     = 0,
    FORMAT_INST_SBIex   = 1,
    FORMAT_INST_ADLIB_INS = 2,
};

enum class FormatCaps
{
    FORMAT_CAPS_NOTHING = 0x00,
    FORMAT_CAPS_OPEN    = 0x01,
    FORMAT_CAPS_SAVE    = 0x02,
    FORMAT_CAPS_IMPORT  = 0x04,
    FORMAT_CAPS_EVERYTHING = FORMAT_CAPS_OPEN|FORMAT_CAPS_SAVE|FORMAT_CAPS_IMPORT
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
    ERR_NOT_IMLEMENTED,
    //! Detected file format is not supported
    ERR_UNSUPPORTED_FORMAT,
    //! Any other error
    ERR_UNKNOWN
};

#endif // FFMT_ENUMS_H
