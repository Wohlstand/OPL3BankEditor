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
#ifdef Q_OS_MACX
#   include <QFileOpenEvent>
#endif



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

#ifdef __APPLE__
    QStringList files = a.getOpenFileChain();

    if(!files.isEmpty())
        w.openOrImportFile(files.first());
#endif

#ifdef __APPLE__
    QObject::connect(&a, SIGNAL(openFileRequested(QString)), &w, SLOT(openFileSlot(QString)));
    a.setConnected();
#endif

    return a.exec();
}




Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
    installTranslator(&m_qtTranslator);
    installTranslator(&m_appTranslator);
}

Application::~Application()
{}

void Application::translate(const QString &language)
{
    if(language.isEmpty())
        return translate(QLocale::system().name());

    QString qtTranslationDir = getQtTranslationDir();
    qDebug() << "Qt translation dir:" << qtTranslationDir;
    if(!m_qtTranslator.load("qt_" + language, qtTranslationDir))
        qWarning() << "Failed to load Qt translation:" << "qt_" + language;

    QString appTranslationDir = getAppTranslationDir();
    qDebug() << "App translation dir:" << appTranslationDir;
    if(!m_appTranslator.load("opl3bankeditor_" + language, appTranslationDir))
        qWarning() << "Failed to load Qt translation:" << "opl3bankeditor_" + language;
}

QString Application::getQtTranslationDir() const
{
#if defined(Q_OS_WIN)
    return QCoreApplication::applicationDirPath() + "/translations";
#elif defined(Q_OS_DARWIN)
    return QCoreApplication::applicationDirPath() + "/../Resources/translations";
#else
#   if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    return QLibraryInfo::path(QLibraryInfo::TranslationsPath);
#   else
    return QLibraryInfo::location(QLibraryInfo::TranslationsPath);
#   endif
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


#ifdef Q_OS_MACX

void Application::setConnected()
{
    m_connected = true;
}

bool Application::event(QEvent *event)
{
    if(event->type() == QEvent::FileOpen)
    {
        QFileOpenEvent *openEvent = static_cast<QFileOpenEvent *>(event);

        if(openEvent)
        {
            if(m_connected)
            {
                QString file = openEvent->file();
                qDebug() << "Opened file " + file + " (signal)";
                emit openFileRequested(file);
            }
            else
            {
                QString file = openEvent->file();
                qDebug() << "Opened file " + file + " (queue)";
                m_openFileRequests.enqueue(file);
            }
        }
        else
        {
            qDebug() << "Failed to process openEvent: pointer is null!";
        }
    }

    return QApplication::event(event);
}

QStringList Application::getOpenFileChain()
{
    QStringList chain;

    while(!m_openFileRequests.isEmpty())
    {
        QString file = m_openFileRequests.dequeue();
        chain.push_back(file);
    }

    return chain;
}

#endif
