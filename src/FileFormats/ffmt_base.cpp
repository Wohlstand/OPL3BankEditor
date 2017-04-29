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

#include "../common.h"

#include <memory>
#include <list>
#include "adlibbnk.h"
#include "apogeetmb.h"
#include "dmxopl2.h"
#include "junlevizion.h"
#include "milesopl.h"
#include "sb_ibk.h"
#include "ffmt_base.h"

typedef std::unique_ptr<FmBankFormatBase> FmBankFormatBase_uptr;
typedef std::list<FmBankFormatBase_uptr>  FmBankFormatsL;

//! Bank formats
static FmBankFormatsL g_formats;
//! Single-Instrument formats
static FmBankFormatsL g_formatsInstr;

void FmBankFormatBase::registerBankFormat(FmBankFormatBase *format)
{
    g_formats.push_back(FmBankFormatBase_uptr(format));
}

void FmBankFormatBase::registerInstFormat(FmBankFormatBase *format)
{
    g_formatsInstr.push_back(FmBankFormatBase_uptr(format));
}

void FmBankFormatBase::registerAllFormats()
{
    g_formats.clear();
    registerBankFormat(new JunleVizion());
    registerBankFormat(new DmxOPL2());
    registerBankFormat(new ApogeeTMB());
    registerBankFormat(new MilesOPL());
    registerBankFormat(new SbIBK_DOS());
    registerInstFormat(new SbIBK_DOS());
    registerBankFormat(new SbIBK_UNIX_READ());
    registerBankFormat(new SbIBK_UNIX2OP_SAVE());
    registerBankFormat(new SbIBK_UNIX4OP_SAVE());
    registerBankFormat(new AdLibBnk_read());
    registerBankFormat(new AdLibBnk_save());
    registerBankFormat(new HmiBnk_save());
}

FmBankFormatBase::FmBankFormatBase() {}

FmBankFormatBase::~FmBankFormatBase()
{}

QString FmBankFormatBase::getSaveFiltersList()
{
    QString formats;
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(p->formatCaps() & FORMAT_CAPS_SAVE)
        {
            formats.append(QString("%1 (%2);;").arg(p->formatName()).arg(p->formatExtensionMask()));
        }
    }
    if(formats.endsWith(";;"))
        formats.remove(formats.size()-2, 2);
    return formats;
}

QString FmBankFormatBase::getOpenFiltersList(bool import)
{
    QString out;
    QString masks;
    QString formats;
    //! Look for importable or openable formats?
    FormatCaps dst = import ? FORMAT_CAPS_IMPORT : FORMAT_CAPS_OPEN;

    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(!masks.isEmpty())
            masks.append(' ');
        if(p->formatCaps() & dst)
        {
            masks.append(p->formatExtensionMask());
            formats.append(QString("%1 (%2);;").arg(p->formatName()).arg(p->formatExtensionMask()));
        }
    }
    out.append(QString("Supported bank files (%1);;").arg(masks));
    out.append(formats);
    out.push_back("All files (*.*)");
    return out;
}

QString FmBankFormatBase::getInstOpenFiltersList(bool import)
{
    QString out;
    QString masks;
    QString formats;
    //! Look for importable or openable formats?
    FormatCaps dst = import ? FORMAT_CAPS_IMPORT : FORMAT_CAPS_OPEN;
    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(!masks.isEmpty())
            masks.append(' ');
        if(p->formatCaps() & dst)
        {
            masks.append(p->formatInstExtensionMask());
            formats.append(QString("%1 (%2);;").arg(p->formatInstName()).arg(p->formatInstExtensionMask()));
        }
    }
    out.append(QString("Supported instrument files (%1);;").arg(masks));
    out.append(formats);
    out.push_back("All files (*.*)");
    return out;
}

FmBankFormatBase::Formats FmBankFormatBase::getFormatFromFilter(QString filter)
{
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        QString f = QString("%1 (%2)").arg(p->formatName()).arg(p->formatExtensionMask());
        if(f == filter)
            return p->formatId();
    }
    return FORMAT_UNKNOWN;
}

QString FmBankFormatBase::getFilterFromFormat(FmBankFormatBase::Formats format, int requiredCaps)
{
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatId() == format) && (p->formatCaps() & requiredCaps))
            return QString("%1 (%2)").arg(p->formatName()).arg(p->formatExtensionMask());
    }
    return "UNKNOWN";
}



bool FmBankFormatBase::detect(const QString&, char*)
{
    return false;
}

bool FmBankFormatBase::detectInst(const QString &, char *)
{
    return false;
}

int FmBankFormatBase::loadFile(QString, FmBank &)
{
    return ERR_NOT_IMLEMENTED;
}

int FmBankFormatBase::saveFile(QString, FmBank &)
{
    return ERR_NOT_IMLEMENTED;
}

int FmBankFormatBase::loadFileInst(QString, FmBank::Instrument &, bool *)
{
    return ERR_NOT_IMLEMENTED;
}

int FmBankFormatBase::saveFileInst(QString, FmBank::Instrument &, bool)
{
    return ERR_NOT_IMLEMENTED;
}

int FmBankFormatBase::formatCaps()
{
    return FORMAT_CAPS_NOTHING;
}

int FmBankFormatBase::formatInstCaps()
{
    return FORMAT_CAPS_NOTHING;
}

QString FmBankFormatBase::formatInstName()
{
    return "Unknown format";
}

QString FmBankFormatBase::formatInstExtensionMask()
{
    return "*.*";
}

QString FmBankFormatBase::formatName()
{
    return "Unknown format";
}

QString FmBankFormatBase::formatExtensionMask()
{
    return "*.*";
}

FmBankFormatBase::Formats FmBankFormatBase::formatId()
{
    return FORMAT_UNKNOWN;
}

FmBankFormatBase::InsFormats FmBankFormatBase::formatInstId()
{
    return FORMAT_INST_UNKNOWN;
}


bool FmBankFormatBase::isImportOnly(FmBankFormatBase::Formats format)
{
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if(p->formatId() == format)
            return (p->formatCaps() == FORMAT_CAPS_IMPORT);
    }
     return false;;\
}

int FmBankFormatBase::OpenBankFile(QString filePath, FmBank &bank, Formats *recent)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    Formats fmt = FORMAT_UNKNOWN;

    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatCaps() & FORMAT_CAPS_OPEN) && p->detect(filePath, magic))
        {
            err = p->loadFile(filePath, bank);
            fmt = p->formatId();
            break;
        }
    }
    if(recent)
        *recent = fmt;

    return err;
}

int FmBankFormatBase::ImportBankFile(QString filePath, FmBank &bank, FmBankFormatBase::Formats *recent)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    Formats fmt = FORMAT_UNKNOWN;

    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatCaps() & FORMAT_CAPS_IMPORT) && p->detect(filePath, magic))
        {
            err = p->loadFile(filePath, bank);
            fmt = p->formatId();
            break;
        }
    }

    if(recent)
        *recent = fmt;
    return err;
}

int FmBankFormatBase::SaveBankFile(QString filePath, FmBank &bank, FmBankFormatBase::Formats dest)
{
    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    for(FmBankFormatBase_uptr &p : g_formats)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatCaps() & FORMAT_CAPS_SAVE) && (p->formatId() == dest))
        {
            err = p->saveFile(filePath, bank);
            break;
        }
    }
    return err;
}

int FmBankFormatBase::OpenInstrumentFile(QString filePath,
                                         FmBank::Instrument &ins,
                                         FmBankFormatBase::InsFormats *recent,
                                         bool *isDrum)
{
    char magic[32];
    getMagic(filePath, magic, 32);

    int err = FmBankFormatBase::ERR_UNSUPPORTED_FORMAT;
    InsFormats fmt = FORMAT_INST_UNKNOWN;

    for(FmBankFormatBase_uptr &p : g_formatsInstr)
    {
        Q_ASSERT(p.get());//It must be non-null!
        if((p->formatInstCaps() & FORMAT_CAPS_OPEN) && p->detectInst(filePath, magic))
        {
            err = p->loadFileInst(filePath, ins, isDrum);
            fmt = p->formatInstId();
            break;
        }
    }
    if(recent)
        *recent = fmt;

    return err;
}
