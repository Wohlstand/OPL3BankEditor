/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2025 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2025 Vitaly Novichkov <admin@wohlnet.ru>
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

#if !defined(IS_QT_4)

#include <QDockWidget>
#include <QAbstractItemView>
#include <QComboBox>

#include "proxystyle.h"

BankEditor_ProxyStyle::BankEditor_ProxyStyle(QStyle *style) : QProxyStyle(style) {}

BankEditor_ProxyStyle::BankEditor_ProxyStyle(const QString &key) : QProxyStyle(key) {}

int BankEditor_ProxyStyle::styleHint(QStyle::StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const
{
    switch(hint)
    {
        //Combobox fix by Guilherme Nascimento:
        //http://stackoverflow.com/questions/20554940/qcombobox-pop-up-expanding-and-qtwebkit
        case QStyle::SH_ComboBox_Popup:
        {
            const QComboBox *combo = (QComboBox *) widget;//convert qwidget in QComboBox
            const QObjectList a = combo->children();
            const int j = a.count();
            QAbstractItemView *view = 0;
            QString className = "";
            bool hasView = false;

            /*
                at this point I have not used combo->view() because he "crash" the application without explanation
                so I had to make a loop to find the "view"
                */
            for(int i = 0; i < j; ++i)
            {
                const QObjectList b = a.at(i)->children();
                const int y = b.count();

                for(int x = 0; x < y; ++x)
                {
                    className = b.at(x)->metaObject()->className();
                    if(className == "QComboBoxListView") {
                        view = (QAbstractItemView *) b.at(x);
                        hasView = true;
                        break;
                    }
                }

                if (hasView == true)
                {
                    break;
                }
            }

            if(hasView == true)
            {
                const int iconSize = combo->iconSize().width();
                const QFontMetrics fontMetrics1 = view->fontMetrics();
                const QFontMetrics fontMetrics2 = combo->fontMetrics();
                const int j = combo->count();
                int width = combo->width(); //default width

                for(int i=0; i < j; ++i)
                {
#if QT_VERSION >= QT_VERSION_CHECK(5, 11, 0)
                    const int textWidth = qMax(
                                fontMetrics1.horizontalAdvance(combo->itemText(i) + "WW"),
                                fontMetrics2.horizontalAdvance(combo->itemText(i) + "WW")
                                );
#else
                    const int textWidth = qMax(
                                fontMetrics1.width(combo->itemText(i) + "WW"),
                                fontMetrics2.width(combo->itemText(i) + "WW")
                                );
#endif
                    if(combo->itemIcon(i).isNull())
                        width = qMax(width, textWidth);
                    else
                        width = qMax(width, textWidth + iconSize);
                }

                view->setMinimumWidth(width);
            }
        } /*fallthrough*/
        //Combobox fix end

        default:
            break;
    }
    return QProxyStyle::styleHint(hint, option, widget, returnData);
}

#endif  // !defined(IS_QT_4)
