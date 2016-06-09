#include <QPainter>
#include <QLayout>
#include "piano.h"

//! Octave map of black keys
const bool isHalfToneKey[] = {
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
const double offsets[] = {
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
    memset(m_highlightNotes, 0, sizeof(bool)*128);
    m_recentNote = 0;
}

Piano::~Piano()
{}

void Piano::mousePressEvent(QMouseEvent *evt)
{
    double  x       = double(evt->pos().x());
    double  width   = double(this->width());
    int     note    = int(floor(128.0*(x/width)));
    if(note>127) note=127;
    m_recentNote = note;
    m_highlightNotes[note] = true;
    emit    gotNote(note);
    emit    pressed();
    repaint();
}

void Piano::mouseReleaseEvent(QMouseEvent *)
{
    m_highlightNotes[m_recentNote] = false;
    emit released();
    repaint();
}

void Piano::paintEvent(QPaintEvent *evt)
{
    QPainter paint(this);
    QPen pen;
    pen.setBrush(this->isEnabled() ? Qt::black : Qt::lightGray );

    pen.setWidth(1);
    paint.setPen(pen);
    double widthOfCell = double(this->width())/128.0;
    for(int i=0; i<=128; i++)
    {
        double Offset = ((i%12==0)) ? 0.0 : offsets[(i-1)%12];
        //double halfOffset = widthOfCell/2.0;
        bool curHalf  = isHalfToneKey[(i)%12];
        //bool nextHalf = !isHalfToneKey[(i+1)%12];
        double posX = i*widthOfCell + widthOfCell*Offset;

        QLineF line(posX, 0, posX, this->height());
        pen.setWidth( ((i%12)==0) ? 2 : 1 );
        paint.setPen(pen);
        paint.drawLine(line);
        if( curHalf )
        {
            QLine line(i*widthOfCell+widthOfCell/2, 0, i*widthOfCell+widthOfCell/2, this->height()/2);
            pen.setWidth(widthOfCell);
            paint.setPen(pen);
            paint.drawLine(line);
        }
        if(i<128)
        {
            if(m_highlightNotes[i])
            {
                QRectF rect(i*widthOfCell, 0.0, widthOfCell, this->height());
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

