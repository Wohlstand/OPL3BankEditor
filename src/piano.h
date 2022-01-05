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

#ifndef PIANO_H
#define PIANO_H

#include <QFrame>
#include <QMouseEvent>

/**
 * @brief A piano keyboard for mouse touching to play a notes
 */
class Piano : public QFrame
{
    Q_OBJECT

public:
    explicit Piano(QWidget* parent = 0);
    ~Piano();
    void setNoteHighlighted(int note, bool hl);

private:
    void findNote(QMouseEvent *evt, int &note);

protected:
    virtual void mousePressEvent(QMouseEvent* evt);
    virtual void mouseReleaseEvent(QMouseEvent* evt);
    virtual void mouseMoveEvent(QMouseEvent* evt);
    virtual void paintEvent(QPaintEvent*evt);
signals:
    /**
     * @brief Piano key pressed and note has been detected
     * @param note Note ID from 0 to 127
     */
    void gotNote(int note);
    /**
     * @brief Piano key pressed
     */
    void pressed();
    /**
     * @brief Piano key released
     */
    void released();

private:
    bool m_held = false;
    //! Table of highlighted notes
    bool m_highlightNotes[128];
    //! Recently played notes
    int  m_recentNote;
};

#endif // PIANO_H
