/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2018 Vitaly Novichkov <admin@wohlnet.ru>
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

class Application : public QApplication
{
    Q_OBJECT

public:
    Application(int &argc, char **argv);

    static Application *instance()
        { return static_cast<Application *>(qApp); }

    QString getQtTranslationDir() const;
    QString getAppTranslationDir() const;

public slots:
    void translate(const QString &language = QString());

private:
    QTranslator m_qtTranslator;
    QTranslator m_appTranslator;
};
