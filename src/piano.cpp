/*
 * OPL Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2016-2021 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * OPN2 Bank Editor by Wohlstand, a free tool for music bank editing
 * Copyright (c) 2017-2021 Vitaly Novichkov <admin@wohlnet.ru>
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

#include <QPainter>
#include <QLayout>
#include <cmath>
#include "piano.h"

//! Octave map of black keys
const bool isHalfToneKey[] =
{
    false,  //C
    true,   //C#
    false,  //D
    true,   //D#
    false,  //E
    false,  //F
    true,   //F#
    false,  //G
    true,   //G#
    false,  //A
    true,   //A#
    false,  //H
};

//! Octave map of inter-key lines offsets
const double offsets[] =
{
    0.5,  //C
    -0.5,  //C#
    0.5,  //D
    -0.5,  //D#
    0.0,  //E
    0.5,  //F
    -0.5,  //F#
    0.5,  //G
    -0.5,  //G#
    0.5,  //A
    -0.5,  //A#
    0.0,  //H
};


Piano::Piano(QWidget *parent) : QFrame(parent)
{
    memset(m_highlightNotes, 0, sizeof(bool) * 128);
    m_recentNote = 0;
    m_held = false;
}

Piano::~Piano()
{}

void Piano::setNoteHighlighted(int note, bool hl)
{
    if(note < 0 || note >= 128 || m_highlightNotes[note] == hl)
        return;

    m_highlightNotes[note] = hl;
    repaint();
}

void Piano::findNote(QMouseEvent *evt, int &note)
{
    double  x       = double(evt->pos().x());
    double  width   = double(this->width());
    note    = int(floor(128.0 * (x / width)));
    if(note < 0)
        note = 0;
    if(note > 127)
        note = 127;
}

void Piano::mousePressEvent(QMouseEvent *evt)
{
    int note = -1;
    findNote(evt, note);
    m_held = true;
    m_recentNote            = note;
    m_highlightNotes[note]  = true;
    emit    gotNote(note);
    emit    pressed();
    repaint();
}

void Piano::mouseReleaseEvent(QMouseEvent *evt)
{
    int note = -1;
    findNote(evt, note);
    m_held = false;
    m_recentNote = note;
    m_highlightNotes[m_recentNote] = false;
    emit    gotNote(m_recentNote);
    emit    released();
    repaint();
}

void Piano::mouseMoveEvent(QMouseEvent *evt)
{
    if(!m_held)
        return;
    int note = -1;
    findNote(evt, note);
    if(note != m_recentNote)
    {
        m_highlightNotes[m_recentNote] = false;
        emit    gotNote(m_recentNote);
        emit    released();
        m_recentNote = note;
        m_highlightNotes[note] = true;
        emit    gotNote(note);
        emit    pressed();
        repaint();
    }
}

void Piano::paintEvent(QPaintEvent *evt)
{
    QPainter paint(this);
    QPen pen;
    pen.setBrush(this->isEnabled() ? Qt::black : Qt::lightGray);
    pen.setWidth(1);
    paint.setPen(pen);
    double widthOfCell = double(this->width()) / 128.0;

    for(int i = 0; i <= 128; i++)
    {
        double  Offset  = ((i % 12 == 0)) ? 0.0 : offsets[(i - 1) % 12];
        bool    curHalf = isHalfToneKey[(i) % 12];
        double  posX    = i * widthOfCell + widthOfCell * Offset;
        QLineF line(posX, 0, posX, this->height());
        pen.setWidth(((i % 12) == 0) ? 2 : 1);
        paint.setPen(pen);
        paint.drawLine(line);

        if(curHalf)
        {
            QLine line(i * widthOfCell + widthOfCell / 2.0, 0, i * widthOfCell + widthOfCell / 2.0, this->height() / 2);
            pen.setWidth(widthOfCell);
            paint.setPen(pen);
            paint.drawLine(line);
        }

        if(i < 128)
        {
            if(m_highlightNotes[i])
            {
                QRectF rect(i * widthOfCell, 0.0, widthOfCell, this->height());
                paint.setPen(Qt::NoPen);
                paint.setBrush(Qt::red);
                paint.setOpacity(0.5);
                paint.drawRect(rect);
                paint.setBrush(Qt::NoBrush);
                paint.setOpacity(1.0);
            }
        }
    }

    paint.end();
    QFrame::paintEvent(evt);
}

