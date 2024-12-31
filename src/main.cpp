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

#include "main.h"
#include "bank_editor.h"
#include "proxystyle.h"
#include <QLibraryInfo>
#include <QStringList>
#include <QDebug>
#include <QDir>
#include <QFile>

int main(int argc, char *argv[])
{
    Application a(argc, argv);

#if !defined(IS_QT_4)
    a.setStyle(new BankEditor_ProxyStyle(a.style()));
#endif

    BankEditor w;
    w.show();

    QStringList args = a.arguments();
    if(args.size()>1)
        w.openOrImportFile(args[1]);

    return a.exec();
}

Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    installTranslator(&m_qtTranslator);
    installTranslator(&m_appTranslator);
}

void Application::translate(const QString &language)
{
    if (language.isEmpty())
        return translate(QLocale::system().name());

    QString qtTranslationDir = getQtTranslationDir();
    qDebug() << "Qt translation dir:" << qtTranslationDir;
    m_qtTranslator.load("qt_" + language, qtTranslationDir);

    QString appTranslationDir = getAppTranslationDir();
    qDebug() << "App translation dir:" << appTranslationDir;
    m_appTranslator.load("opl3bankeditor_" + language, appTranslationDir);
}

QString Application::getQtTranslationDir() const
{
#if defined(Q_OS_WIN)
    return QCoreApplication::applicationDirPath() + "/translations";
#elif defined(Q_OS_DARWIN)
    return QCoreApplication::applicationDirPath() + "/../Resources/translations";
#else
    return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#endif
}

QString Application::getAppTranslationDir() const
{
#if defined(Q_OS_WIN) || defined(Q_OS_DARWIN)
    return getQtTranslationDir();
#else
    //QString qtTranslationDir = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
    QDir dir(QCoreApplication::applicationDirPath() + "/../share/opl3_bank_editor/");
    if(dir.exists())
        return QCoreApplication::applicationDirPath() + "/../share/opl3_bank_editor/translations";
    else //For debug purposes, use In-Source translations :-P
        return QCoreApplication::applicationDirPath() + "/../src/translations";
#endif
}
