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

#include "format_furnace_fui.h"
#include "../common.h"
#include <QFile>
#include <QFileInfo>

/*
 * Furnace instrument (.fui) format, OPL FM subset.
 *
 * A .fui file is a "FINS" header followed by a stream of features, each being a
 * 2-byte code, a 2-byte little-endian block length and its payload. Only the
 * "NA" (name) and "FM" (FM operator data) features are meaningful for OPL; the
 * stream terminates at the "EN" feature. See ~/furnace/papers/newIns.md.
 *
 * The FM feature stores operators in Furnace's internal order. Mapping to the
 * editor's operator indices (CARRIER1=0, MODULATOR1=1, CARRIER2=2, MODULATOR2=3):
 *   - 2-op:  file order [modulator, carrier]              -> {MODULATOR1, CARRIER1}
 *   - 4-op:  file order [op1, op3, op2, op4]              -> {MODULATOR1, MODULATOR2, CARRIER1, CARRIER2}
 *            (= [mod1, mod2, car1, car2])
 * Both directions build/consume native OPL register bytes and route them through
 * the FmBank::Instrument getters/setters, which handle level/sustain mirroring.
 */

//! Furnace instrument type for OPL2/OPL3 FM
#define DIV_INS_OPL         14
//! Furnace instrument type for OPL FM drums
#define DIV_INS_OPL_DRUMS   32
//! Engine version stamped into produced files (matches the reference yrw801 writers)
#define FUI_ENGINE_VERSION  244

//! File-order -> editor-index maps (see comment above)
static const int s_fuiOrder2op[2] = { MODULATOR1, CARRIER1 };
static const int s_fuiOrder4op[4] = { MODULATOR1, MODULATOR2, CARRIER1, CARRIER2 };

bool FurnaceFUI::detectInst(const QString &filePath, char *magic)
{
    Q_UNUSED(filePath);
    return memcmp(magic, "FINS", 4) == 0;
}

FfmtErrCode FurnaceFUI::loadFileInst(QString filePath, FmBank::Instrument &inst, bool *isDrum)
{
    QFile file(filePath);
    if(!file.open(QIODevice::ReadOnly))
        return FfmtErrCode::ERR_NOFILE;

    QByteArray data = file.readAll();
    file.close();

    const int size = data.size();
    const uint8_t *d = reinterpret_cast<const uint8_t *>(data.constData());

    // Header: "FINS" + version(2) + instrument type(2)
    if(size < 8 || memcmp(d, "FINS", 4) != 0)
        return FfmtErrCode::ERR_BADFORMAT;

    uint16_t insType = toUint16LE(d + 6);
    if(insType != DIV_INS_OPL && insType != DIV_INS_OPL_DRUMS)
        return FfmtErrCode::ERR_UNSUPPORTED_FORMAT;

    if(isDrum)
        *isDrum = (insType == DIV_INS_OPL_DRUMS);

    inst = FmBank::emptyInst();
    if(insType == DIV_INS_OPL_DRUMS)
        inst.rhythm_drum_type = 6; // Bass drum by default; exact type isn't stored in the FM feature

    bool gotFM = false;
    int pos = 8;

    // Feature stream: [code(2)][length(2)][payload(length)]
    while(pos + 4 <= size)
    {
        char code[2] = { char(d[pos]), char(d[pos + 1]) };
        uint16_t blockLen = toUint16LE(d + pos + 2);
        int payload = pos + 4;

        if(memcmp(code, "EN", 2) == 0)
            break;

        if(payload + blockLen > size)
            return FfmtErrCode::ERR_BADFORMAT;

        if(memcmp(code, "NA", 2) == 0)
        {
            int n = 0;
            for(; n < blockLen && n < 32 && d[payload + n] != 0; ++n)
                inst.name[n] = char(d[payload + n]);
            inst.name[n] = '\0';
        }
        else if(memcmp(code, "FM", 2) == 0)
        {
            if(blockLen < 5) // flags + 4 base bytes
                return FfmtErrCode::ERR_BADFORMAT;

            const uint8_t *fm = d + payload;
            uint8_t opCount = fm[0] & 0x0F;
            bool is4op = (opCount >= 4);

            uint8_t baseAlgFb = fm[1];        // |x|ALG|x|FB|
            uint8_t alg = (baseAlgFb >> 4) & 0x07;
            uint8_t fb  = baseAlgFb & 0x07;

            inst.en_4op = is4op;
            inst.en_pseudo4op = false;

            if(is4op)
            {
                // OPL3 4-op: two connection bits, feedback on the first pair only
                inst.feedback1 = fb;
                inst.connection1 = (alg & 0x01) != 0;
                inst.feedback2 = 0;
                inst.connection2 = (alg & 0x02) != 0;
            }
            else
            {
                inst.feedback1 = fb;
                inst.connection1 = (alg & 0x01) != 0;
                inst.feedback2 = 0;
                inst.connection2 = false;
            }

            const int *order = is4op ? s_fuiOrder4op : s_fuiOrder2op;
            int nOps = is4op ? 4 : 2;

            // 4 base-data bytes precede the per-operator data
            const uint8_t *op = fm + 5;
            if(payload + 5 + nOps * 8 > payload + blockLen)
                return FfmtErrCode::ERR_BADFORMAT;

            for(int i = 0; i < nOps; ++i, op += 8)
            {
                int e = order[i]; // editor operator index

                uint8_t ksr  = (op[0] >> 7) & 0x01;
                uint8_t mult =  op[0] & 0x0F;
                uint8_t tl   =  op[1] & 0x3F;
                uint8_t vib  = (op[2] >> 5) & 0x01;
                uint8_t ar   =  op[2] & 0x0F; // OPL attack: 4-bit
                uint8_t am   = (op[3] >> 7) & 0x01;
                uint8_t ksl  = (op[3] >> 5) & 0x03;
                uint8_t dr   =  op[3] & 0x0F; // OPL decay: 4-bit
                uint8_t egt  = (op[4] >> 7) & 0x01;
                uint8_t srel =  op[5];        // |SL|RR|, already the native 0x80 byte
                uint8_t ws   =  op[7] & 0x07;

                inst.setAVEKM(e, uint8_t((am << 7) | (vib << 6) | (egt << 5) | (ksr << 4) | mult));
                inst.setKSLL(e, uint8_t((ksl << 6) | tl));
                inst.setAtDec(e, uint8_t((ar << 4) | dr));
                inst.setSusRel(e, srel);
                inst.setWaveForm(e, ws);
            }

            gotFM = true;
        }
        // Unknown/other features are skipped

        pos = payload + blockLen;
    }

    if(!gotFM)
        return FfmtErrCode::ERR_BADFORMAT;

    inst.is_blank = false;
    return FfmtErrCode::ERR_OK;
}

//! Append a feature block, back-patching its length once its payload is written
static void writeFeature(QByteArray &out, const char *code, const QByteArray &payload)
{
    out.append(code, 2);
    uint16_t len = uint16_t(payload.size());
    out.append(char(len & 0xFF));
    out.append(char((len >> 8) & 0xFF));
    out.append(payload);
}

FfmtErrCode FurnaceFUI::saveFileInst(QString filePath, FmBank::Instrument &inst, bool isDrum)
{
    QFile file(filePath);
    if(!file.open(QIODevice::WriteOnly))
        return FfmtErrCode::ERR_NOFILE;

    QByteArray out;

    // Header
    out.append("FINS", 4);
    out.append(char(FUI_ENGINE_VERSION & 0xFF));
    out.append(char((FUI_ENGINE_VERSION >> 8) & 0xFF));
    uint16_t insType = isDrum ? DIV_INS_OPL_DRUMS : DIV_INS_OPL;
    out.append(char(insType & 0xFF));
    out.append(char((insType >> 8) & 0xFF));

    // Name feature. Fall back to the file's base name when the instrument has
    // no name of its own, so it doesn't show up unnamed in Furnace.
    {
        char nm[33];
        memcpy(nm, inst.name, 32);
        nm[32] = '\0';
        QString name = QString::fromUtf8(nm).trimmed();
        if(name.isEmpty())
            name = QFileInfo(filePath).completeBaseName();

        QByteArray na = name.toUtf8();
        na.append('\0'); // zero-terminated string
        writeFeature(out, "NA", na);
    }

    // FM feature
    {
        bool is4op = inst.en_4op || inst.en_pseudo4op;
        int nOps = is4op ? 4 : 2;

        QByteArray fm;
        // flags: op-enable bits (4..7) + op count (0..3)
        fm.append(char((is4op ? 0xF0 : 0x30) | (nOps & 0x0F)));

        // base data
        uint8_t alg, fb;
        if(is4op)
        {
            alg = uint8_t((inst.connection1 ? 0x01 : 0x00) | (inst.connection2 ? 0x02 : 0x00));
            fb  = inst.feedback1 & 0x07;
        }
        else
        {
            alg = inst.connection1 ? 0x01 : 0x00;
            fb  = inst.feedback1 & 0x07;
        }
        fm.append(char(((alg & 0x07) << 4) | (fb & 0x07))); // |x|ALG|x|FB|
        fm.append(char(0));                                  // FMS2/AMS/FMS (unused for OPL)
        fm.append(char(is4op ? 0x20 : 0x00));                // AM2/4/LLPatch: bit5 = 4-op mode
        fm.append(char(0));                                  // Block (>=224): no editor equivalent

        const int *order = is4op ? s_fuiOrder4op : s_fuiOrder2op;
        for(int i = 0; i < nOps; ++i)
        {
            int e = order[i];
            uint8_t avekm  = inst.getAVEKM(e);   // |AM|VIB|EG|KSR|MULT|
            uint8_t ksll   = inst.getKSLL(e);    // |KSL|Level(native)|
            uint8_t atdec  = inst.getAtDec(e);   // |AR|DR|
            uint8_t susrel = inst.getSusRel(e);  // |SL|RR| (native)
            uint8_t ws     = inst.getWaveForm(e) & 0x07;

            uint8_t ksr  = (avekm >> 4) & 0x01;
            uint8_t mult =  avekm & 0x0F;
            uint8_t vib  = (avekm >> 6) & 0x01;
            uint8_t egt  = (avekm >> 5) & 0x01;
            uint8_t am   = (avekm >> 7) & 0x01;
            uint8_t ksl  = (ksll >> 6) & 0x03;
            uint8_t tl   =  ksll & 0x3F;
            uint8_t ar   = (atdec >> 4) & 0x0F;
            uint8_t dr   =  atdec & 0x0F;

            fm.append(char((ksr << 7) | mult));         // |KSR|DT|MULT|
            fm.append(char(tl & 0x3F));                 // |SUS|TL|
            fm.append(char((vib << 5) | (ar & 0x1F)));  // |RS|VIB|AR|
            fm.append(char((am << 7) | (ksl << 5) | (dr & 0x1F))); // |AM|KSL|DR|
            fm.append(char(egt << 7));                  // |EGT|KVS|D2R|
            fm.append(char(susrel));                    // |SL|RR|
            fm.append(char(0));                         // |DVB|SSG|
            fm.append(char(ws));                        // |DAM|DT2|WS|
        }

        writeFeature(out, "FM", fm);
    }

    // End of features
    out.append("EN", 2);

    if(file.write(out) != out.size())
    {
        file.close();
        return FfmtErrCode::ERR_UNKNOWN;
    }

    file.close();
    return FfmtErrCode::ERR_OK;
}

int FurnaceFUI::formatInstCaps() const
{
    return (int)FormatCaps::FORMAT_CAPS_OPEN |
           (int)FormatCaps::FORMAT_CAPS_SAVE |
           (int)FormatCaps::FORMAT_CAPS_IMPORT;
}

QString FurnaceFUI::formatInstName() const
{
    return "Furnace instrument";
}

QString FurnaceFUI::formatInstModuleName() const
{
    return "Furnace Tracker instrument";
}

QString FurnaceFUI::formatInstExtensionMask() const
{
    return "*.fui";
}

QString FurnaceFUI::formatInstDefaultExtension() const
{
    return "fui";
}

InstFormats FurnaceFUI::formatInstId() const
{
    return InstFormats::FORMAT_INST_FUI;
}
