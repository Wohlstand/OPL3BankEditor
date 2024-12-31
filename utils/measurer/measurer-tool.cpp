/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018-2025 Vitaly Novichkov <admin@wohlnet.ru>
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
#include <atomic>
#include <memory>
#include <cstring>

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "%s <wopl-file-input> <wopl-file-output>\n", argv[0]);
        return 1;
    }

    QApplication app(argc, argv);
    Q_UNUSED(app);

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

    return 0;
}
