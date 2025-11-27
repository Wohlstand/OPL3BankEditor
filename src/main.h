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

#include <QApplication>
#include <QTranslator>

#ifdef Q_OS_MACX
#   include <QQueue>
#   include <QStringList>
#endif

class Application : public QApplication
{
    Q_OBJECT

#ifdef Q_OS_MACX
    //! Queue used before slot will be connected to collect file paths received via QFileOpenEvent
    QQueue<QString> m_openFileRequests;
    //! Mark means to don't collect file paths and send them via signals
    bool            m_connected;
#endif

public:
    Application(int &argc, char **argv);
    ~Application();

    static Application *instance()
        { return static_cast<Application *>(qApp); }

    QString getQtTranslationDir() const;
    QString getAppTranslationDir() const;

#ifdef Q_OS_MACX
    /**
     * @brief Disable collecting of the file paths via queue and send any new-received paths via signal
     */
    void    setConnected();
    /**
     * @brief Input event
     * @param event Event descriptor
     * @return is event successfully processed
     */
    bool    event(QEvent *event);
    /**
     * @brief Get all collected file paths and clear internal queue
     * @return String list of all collected file paths
     */
    QStringList getOpenFileChain();
signals:
    /**
     * @brief Signal emiting on receiving a file path via QFileOpenEvent
     * @param filePath full path to open
     */
    void openFileRequested(QString filePath);
#endif

public slots:
    void translate(const QString &language = QString());

private:
    QTranslator m_qtTranslator;
    QTranslator m_appTranslator;
};
