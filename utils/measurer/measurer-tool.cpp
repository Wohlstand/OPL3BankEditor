/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2026 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <FileFormats/format_wohlstand_opl3.h>
#include <opl/measurer.h>
#include <QApplication>
#include <cstring>

static bool doMeasureInstrument(Measurer &measurer, FmBank::Instrument &ins)
{
    for(uint8_t release = 0; release < 16; ++release)
    {
        for(uint8_t decay = 0; decay < 16; ++decay)
        {
            for(uint8_t sustain = 0; sustain < 16; ++sustain)
            {
                for(uint8_t attack = 0; attack < 16; ++attack)
                {
                    ins.OP[CARRIER1].release = release;
                    ins.OP[CARRIER1].sustain = sustain;
                    ins.OP[CARRIER1].attack = attack;
                    ins.OP[CARRIER1].decay = decay;

                    if(attack == 0 && release == 0)
                    {
                        ins.ms_sound_kon = 6;
                        ins.ms_sound_koff = 0;
                    }
                    else if(attack > 0 && release == 0)
                    {
                        ins.ms_sound_kon = 40000;
                        ins.ms_sound_koff = 0;
                    }
                    else
                    {
                        measurer.doMeasurement(ins, true);
                        // fprintf(stderr, "Measurement was interrupted.\n");
                        // return false;
                    }

                    fprintf(stderr, "\n\n\nEG=%u, KSR=%u, KSL=%u, A=%2u, D=%2u, S=%2u, R=%2u    ON=%5u OFF=%5u\n\n",
                                    ins.OP[CARRIER1].eg,
                                    ins.OP[CARRIER1].ksr,
                                    ins.OP[CARRIER1].ksl,
                                    attack, decay, sustain, release, ins.ms_sound_kon, ins.ms_sound_koff);
                    fflush(stderr);

                    measurer.doEstimation(ins);
                    fflush(stderr);
                }
            }
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    bool doTableBuild = false;

    if(argc == 2 && !strcmp("--build-table", argv[1]))
    {
        doTableBuild = true;
    }
    else if(argc != 3)
    {
        fprintf(stderr, "%s <wopl-file-input> <wopl-file-output>\n", argv[0]);
        return 1;
    }

    QApplication app(argc, argv);
    Q_UNUSED(app);

    if(!doTableBuild)
    {
        WohlstandOPL3 format;
        QString woplFileInput = argv[1];

        FmBank bank;
        FfmtErrCode errLoad = format.loadFile(woplFileInput, bank);

        if(errLoad != FfmtErrCode::ERR_OK)
        {
            fprintf(stderr, "Could not load the WOPL file.\n");
            return 1;
        }

        Measurer measurer;
        FmBank bankBackup = bank;

        if(!measurer.doMeasurement(bank, bankBackup, true))
        {
            fprintf(stderr, "Measurement was interrupted.\n");
            return 1;
        }

        QString woplFileOutput = argv[2];

        FfmtErrCode errSave = format.saveFile(woplFileOutput, bank);
        if(errSave != FfmtErrCode::ERR_OK)
        {
            fprintf(stderr, "Could not save the WOPL file.\n");
            return 1;
        }
    }
    else // Pass through possible values and print the table of results
    {
        FmBank::Instrument ins;
        Measurer measurer;

        ins = FmBank::blankInst(false);

        ins.connection1 = FmBank::Instrument::AM;

        ins.OP[CARRIER1].eg = true;
        ins.OP[CARRIER1].ksr = false;
        if(!doMeasureInstrument(measurer, ins))
            return 0;

        ins.OP[CARRIER1].eg = true;
        ins.OP[CARRIER1].ksr = true;
        if(!doMeasureInstrument(measurer, ins))
            return 0;

        ins.OP[CARRIER1].eg = false;
        ins.OP[CARRIER1].ksr = false;
        if(!doMeasureInstrument(measurer, ins))
            return 0;

        ins.OP[CARRIER1].eg = false;
        ins.OP[CARRIER1].ksr = true;
        if(!doMeasureInstrument(measurer, ins))
            return 0;

    }

    return 0;
}
