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

#ifndef MEASURER_H
#define MEASURER_H

#include <QObject>
#include <QWidget>
#include <QVector>
#include <vector>
#include "../bank.h"

class Measurer : public QObject
{
    Q_OBJECT

    QWidget *m_parentWindow;

public:
    explicit Measurer(QWidget *parent = nullptr);
    ~Measurer();

    bool doMeasurement(FmBank &bank, FmBank &bankBackup, bool forceReset = false);
    bool doMeasurement(FmBank::Instrument &instrument);

    struct DurationInfo
    {
        uint64_t    peak_amplitude_time;
        double      peak_amplitude_value;
        double      quarter_amplitude_time;
        double      begin_amplitude;
        double      interval;
        double      keyoff_out_time;
        int64_t     ms_sound_kon;
        int64_t     ms_sound_koff;
        bool        nosound;
#if defined(ENABLE_PLOTS)
        std::vector<double> amps_on;
        std::vector<double> amps_off;
        double amps_timestep;
#endif
    };
    bool doComputation(const FmBank::Instrument &instrument, DurationInfo &result);

    struct BenchmarkResult {
        QString name;
        qint64  elapsed;
    };
    bool runBenchmark(FmBank::Instrument &instrument, QVector<BenchmarkResult> &result);
};



#endif // MEASURER_H
