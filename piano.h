#ifndef PIANO_H
#define PIANO_H

#include <QFrame>
#include <QMouseEvent>

class Piano : public QFrame
{
    Q_OBJECT

public:
    explicit Piano(QWidget* parent = 0);
    ~Piano();

protected:
    virtual void mousePressEvent(QMouseEvent* evt);
    virtual void mouseReleaseEvent(QMouseEvent*);
    virtual void paintEvent(QPaintEvent*evt);
signals:
    void gotNote(int note);
    void pressed();
    void released();

private:
    bool m_highlightNotes[128];
    int  m_recentNote;
};

#endif // PIANO_H
