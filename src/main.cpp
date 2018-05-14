/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2018 Vitaly Novichkov <admin@wohlnet.ru>
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

#include "bank_editor.h"
#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include <QStringList>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

#if defined(Q_OS_WIN)
    QString qtTranslationDir = QCoreApplication::applicationDirPath() + "/translations";
    const QString &appTranslationDir = qtTranslationDir;
#elif defined(Q_OS_DARWIN)
    QString qtTranslationDir = QCoreApplication::applicationDirPath() + "/../Resources/translations";
    const QString &appTranslationDir = qtTranslationDir;
#else
    QString qtTranslationDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    QString appTranslationDir =
        QCoreApplication::applicationDirPath() + "/../share/opl3_bank_editor/translations";
#endif

    // install a translator of Qt messages
    QTranslator qtTranslator;
    qtTranslator.load("qt_" + QLocale::system().name(), qtTranslationDir);
    a.installTranslator(&qtTranslator);
    // install a translator of application messages
    QTranslator myappTranslator;
    myappTranslator.load(QLocale::system().name(), appTranslationDir);
    a.installTranslator(&myappTranslator);

    BankEditor w;
    w.show();

    QStringList args = a.arguments();
    if(args.size()>1)
        w.openFile(args[1]);

    return a.exec();
}
