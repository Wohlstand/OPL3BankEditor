/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2022 Vitaly Novichkov <admin@wohlnet.ru>
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

#ifndef BANK_COMPARISON_H
#define BANK_COMPARISON_H

#include "bank.h"
#include "ins_names.h"
#include <QDialog>
#include <set>
#include <vector>
#include <memory>

class FmBank;
namespace Ui { class BankCompareDialog; }

class BankCompareDialog : public QDialog
{
    Q_OBJECT

public:
    explicit BankCompareDialog(unsigned midiSpec,
                               const FmBank &bankA,
                               const FmBank &bankB,
                               QWidget *parent = nullptr);
    ~BankCompareDialog();

private slots:
    void on_chkIgnoreMeasurement_clicked(bool);

private:
    void updateComparison();
    static std::set<uint32_t> collectIds(const FmBank &fmb);

    static QString checkOnlyIn(unsigned spec,
                               const QString &nameA,
                               const QString &styleClassA,
                               const FmBank &A,
                               const FmBank &B);

    static QString checkDifferences(unsigned spec,
                                    uint32_t id,
                                    const FmBank::Instrument &A,
                                    const FmBank::Instrument &B,
                                    unsigned options);

    enum DiffOption
    {
        DiffOpt_IgnoreMeasurement = 1,
    };

    struct DiffElement
    {
        DiffElement(const char *n, int a, int b) : name(n), valueA(a), valueB(b) {}
        const char *name;
        int valueA;
        int valueB;
    };

    static QString tabulateDiffElements(const std::vector<DiffElement> &elts);

    static QString stringOfId(uint32_t id);
    static const FmBank::MidiBank *midiBankOfId(const FmBank &bank, uint32_t id);
    static const FmBank::Instrument *instrumentOfId(const FmBank &bank, uint32_t id);
    static QString nameOfMidiBank(unsigned spec, const FmBank::MidiBank &mb, uint32_t id);
    static QString nameOfInstrument(unsigned spec, const FmBank::Instrument &ins, uint32_t id, bool *isFallback = nullptr);

    unsigned m_midiSpec = 0;
    FmBank m_bankA;
    FmBank m_bankB;
    std::unique_ptr<Ui::BankCompareDialog> m_ui;
};

#endif // BANK_COMPARISON_H
