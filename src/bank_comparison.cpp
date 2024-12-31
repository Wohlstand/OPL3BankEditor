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

#include "bank_comparison.h"
#include "ui_bank_comparison.h"
#include "metaparameter.h"
#include <QDebug>
#include <string.h>

#if QT_VERSION >= 0x050000
namespace Qt {
    static inline QString escape(const QString &s) { return s.toHtmlEscaped(); }
}
#endif

BankCompareDialog::BankCompareDialog(
    unsigned midiSpec, const FmBank &bankA, const FmBank &bankB, QWidget *parent)
    : QDialog(parent), m_midiSpec(midiSpec), m_bankA(bankA), m_bankB(bankB), m_ui(new Ui::BankCompareDialog)
{
    m_ui->setupUi(this);

    updateComparison();
}

BankCompareDialog::~BankCompareDialog()
{
}

void BankCompareDialog::on_chkIgnoreMeasurement_clicked(bool)
{
    updateComparison();
}

void BankCompareDialog::updateComparison()
{
    QString text;
    QTextBrowser *br = m_ui->textBrowser;

    const FmBank &A = m_bankA;
    const FmBank &B = m_bankB;

    text += checkOnlyIn(m_midiSpec, tr("Current"), "compare-A", A, B);
    text += checkOnlyIn(m_midiSpec, tr("Other"), "compare-B", B, A);

    const std::set<uint32_t> idsA = collectIds(A);
    const std::set<uint32_t> idsB = collectIds(B);

    unsigned diffOptions = 0;

    if (m_ui->chkIgnoreMeasurement->isChecked())
        diffOptions |= DiffOpt_IgnoreMeasurement;

    for(uint32_t id : idsA)
    {
        if(idsB.find(id) != idsB.end())
        {
            text += checkDifferences(
                m_midiSpec, id, *instrumentOfId(A, id), *instrumentOfId(B, id), diffOptions);
        }
    }

    br->document()->setDefaultStyleSheet(
        "h1 { text-decoration: underline; }"
        "table { border-style: solid; border-width: 1px; }\n"
        ".name { }\n"
        ".name-fallback { color: gray; }\n"
        ".compare-A { color: blue; }"
        ".compare-B { color: red; }"
        );

    if(text.isEmpty())
        br->setHtml(tr("<h1>Banks are identical.<h1>"));
    else
        br->setHtml(text);
}

std::set<uint32_t> BankCompareDialog::collectIds(const FmBank &fmb)
{
    std::set<uint32_t> ids;

    unsigned melo = fmb.Banks_Melodic.size();
    unsigned drum = fmb.Banks_Percussion.size();

    for(unsigned i = 0, n = melo + drum; i < n; ++i)
    {
        const FmBank::MidiBank &mb = (i < melo) ?
            fmb.Banks_Melodic[i] : fmb.Banks_Percussion[i - melo];
        const FmBank::Instrument *ins = (i < melo) ?
            &fmb.Ins_Melodic[i * 128] : &fmb.Ins_Percussion[(i - melo) * 128];

        for(unsigned j = 0; j < 128; ++j)
        {
            if(!ins[j].is_blank)
            {
                uint32_t id = ((i >= melo) << 24) | (mb.msb << 16) | (mb.lsb << 8) | j;
                ids.insert(id);
            }
        }
    }

    return ids;
}

QString BankCompareDialog::checkOnlyIn(unsigned spec, const QString &nameA, const QString &styleClassA, const FmBank &A, const FmBank &B)
{
    QString text;
    unsigned index = 0;

    const std::set<uint32_t> idsA = collectIds(A);
    const std::set<uint32_t> idsB = collectIds(B);

    for(uint32_t id : idsA)
    {
        if(idsB.find(id) != idsB.end())
            continue;

        if(index == 0)
        {
            text += tr("<h1>Only in bank <span class='%1'>%2</span></h1>")
                .arg(styleClassA)
                .arg(Qt::escape(nameA));
            text += "<table>";
        }

        const FmBank::MidiBank *mb = midiBankOfId(A, id);
        const FmBank::Instrument *ins = instrumentOfId(A, id);

        bool isFallback = false;
        QString mbName = nameOfMidiBank(spec, *mb, id);
        QString insName = nameOfInstrument(spec, *ins, id, &isFallback);

        text += QString("<tr><td>%1</td><td>%2</td><td class='name name%3'>%4</td></tr>")
            .arg(Qt::escape(stringOfId(id)))
            .arg(Qt::escape(mbName))
            .arg(isFallback ? " name-fallback" : "")
            .arg(Qt::escape(insName));

        ++index;
    }

    if(index > 0)
    {
        text += "</table>";
    }

    return text;
}

QString BankCompareDialog::checkDifferences(unsigned spec, uint32_t id, const FmBank::Instrument &A, const FmBank::Instrument &B, unsigned options)
{
    QString text;
    unsigned index = 0;

    auto insDiffHeader = [spec, id, &A]() -> QString
    {
        QString text;
        text += QString("<h1>%1 <span class='compare-A'>%2</span></h1>")
            .arg(Qt::escape(stringOfId(id)))
            .arg(Qt::escape(nameOfInstrument(spec, A, id)));
        text += "<div>";
        return text;
    };

    // Name
    if(strcmp(A.name, B.name) != 0)
    {
        if(index == 0)
            text += insDiffHeader();

        text += tr("<p>Renamed to <span class='compare-B'>%1</span></p>")
            .arg(Qt::escape(((B.name[0] != '\0') ? QString::fromUtf8(B.name) : tr("(empty string)"))));
        ++index;
    }

    bool samealg;
    if(!A.en_4op)
        samealg = !B.en_4op && A.connection1 == B.connection1;
    else
        samealg = B.en_4op && A.en_pseudo4op == B.en_pseudo4op && A.connection1 == B.connection1 && A.connection2 == B.connection2;

    if(!samealg)
    {
        if(index == 0)
            text += insDiffHeader();

        text += tr("<p>Uses <span class='compare-B'>different algorithm</span></p>");

        std::vector<DiffElement> elts;
        elts.emplace_back("4op", A.en_4op, B.en_4op);
        if(A.en_4op || B.en_4op)
            elts.emplace_back("ps4op", A.en_pseudo4op, B.en_pseudo4op);
        elts.emplace_back("con1", A.connection1, B.connection1);
        if(A.en_4op || B.en_4op)
            elts.emplace_back("con2", A.connection2, B.connection2);

        text += tabulateDiffElements(elts);

        ++index;
    }

    // Parameters
    if(samealg)
    {
        std::vector<DiffElement> elts;

        // Pre-filter parameter set
        std::vector<const MetaParameter *> mps;
        for(const MetaParameter &mp : MP_instrument)
        {
            if(!A.en_4op && (mp.flags & MP_4OpOnly) != 0) continue;
            if(A.en_4op && !A.en_pseudo4op && (mp.flags & MP_Pseudo4OpOnly) != 0) continue;
            if((options & DiffOpt_IgnoreMeasurement) != 0 && (mp.flags & MP_Measure) != 0) continue;
            mps.push_back(&mp);
        }

        // Not part of operator
        elts.clear();
        for(const MetaParameter *mp : mps)
        {
            if((mp->flags & MP_OperatorMask) != 0) continue;

            int valA = mp->get(A);
            int valB = mp->get(B);
            if(valA == valB) continue;

            if(index == 0)
                text += insDiffHeader();

            elts.emplace_back(mp->name, valA, valB);
            ++index;
        }
        if(!elts.empty())
            text += tabulateDiffElements(elts);

        // Part of operator
        for(unsigned op = 0; op < (A.en_4op ? 4 : 2); ++op)
        {
            elts.clear();
            unsigned opDiffIndex = 0;

            for(const MetaParameter *mp : mps)
            {
                bool isOfOperator = false;
                switch (op) {
                case 0:
                    isOfOperator = (mp->flags & MP_OperatorMask) == MP_Operator1; break;
                case 1:
                    isOfOperator = (mp->flags & MP_OperatorMask) == MP_Operator2; break;
                case 2:
                    isOfOperator = (mp->flags & MP_OperatorMask) == MP_Operator3; break;
                case 3:
                    isOfOperator = (mp->flags & MP_OperatorMask) == MP_Operator4; break;
               };

                if(!isOfOperator)
                    continue;

                int valA = mp->get(A);
                int valB = mp->get(B);
                if(valA == valB) continue;

                if(index == 0)
                    text += insDiffHeader();

                if(opDiffIndex == 0)
                    text += tr("<h2>Operator %1</h2>").arg(op + 1);

                elts.emplace_back(mp->name, valA, valB);
                ++index;
                ++opDiffIndex;
            }
            if(!elts.empty())
                text += tabulateDiffElements(elts);
        }
    }

    if(index > 0)
    {
        text += "</div>";
    }

    return text;
}

QString BankCompareDialog::tabulateDiffElements(const std::vector<DiffElement> &elts)
{
    QString text = "<table>";

    text += "<tr>";
    for(const DiffElement &d : elts)
        text += QString("<td>%1</td>").arg(d.name);
    text += "</tr>";

    text += "<tr>";
    for(const DiffElement &d : elts)
        text += QString("<td><span class='compare-A'>%1</span><br /><span class='compare-B'>%2</span></td>")
            .arg(d.valueA).arg(d.valueB);
    text += "</tr>";

    text += "</table>";
    return text;
}

QString BankCompareDialog::stringOfId(uint32_t id)
{
    char idText[128];
    sprintf(idText, "%c%03u:%03u:%03u", "MP"[(id >> 24) != 0],
            (id >> 16) & 127, (id >> 8) & 127, id & 127);
    return QString::fromLatin1(idText);
}

const FmBank::MidiBank *BankCompareDialog::midiBankOfId(const FmBank &bank, uint32_t id)
{
    FmBank::MidiBank *mb = nullptr;
    return const_cast<FmBank &>(bank).getBank((id >> 16) & 127, (id >> 8) & 127, (id >> 24) != 0, &mb, nullptr) ?
        mb : nullptr;
}

const FmBank::Instrument *BankCompareDialog::instrumentOfId(const FmBank &bank, uint32_t id)
{
    FmBank::Instrument *ins = nullptr;
    return const_cast<FmBank &>(bank).getBank((id >> 16) & 127, (id >> 8) & 127, (id >> 24) != 0, nullptr, &ins) ?
        &ins[id & 127] : nullptr;
}

QString BankCompareDialog::nameOfInstrument(unsigned spec, const FmBank::Instrument &ins, uint32_t id, bool *isFallback)
{
    if(ins.name[0] != '\0')
        return QString::fromUtf8(ins.name);

    MidiProgramId pr = MidiProgramId((id >> 24) != 0, (id >> 16) & 127, (id >> 8) & 127, id & 127);

    unsigned specObtained = kMidiSpecXG;
    const MidiProgram *p = getMidiProgram(pr, spec, &specObtained);
    if(isFallback)
        *isFallback = (p == nullptr);

    p = p ? p : getFallbackProgram(pr, spec, &specObtained);
    return p ? p->patchName : tr("<Reserved %1>").arg(id & 127);
}

QString BankCompareDialog::nameOfMidiBank(unsigned spec, const FmBank::MidiBank &mb, uint32_t id)
{
    if(mb.name[0] != '\0')
        return QString::fromUtf8(mb.name);

    MidiProgramId pr((id >> 24) != 0, (id >> 16) & 127, (id >> 8) & 127, 0);
    unsigned specObtained = kMidiSpecXG;
    if(const MidiProgram *p = getMidiBank(pr, spec, &specObtained))
        return QString::fromUtf8(p->bankName);

    return QString();
}
