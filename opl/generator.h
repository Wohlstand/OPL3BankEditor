#ifndef GENERATOR_H
#define GENERATOR_H

#include "dbopl.h"

#include <QIODevice>
#include <QObject>

class Generator : public QIODevice
{
    Q_OBJECT

public:
    Generator(int sampleRate, QObject *parent);
    ~Generator();

    void start();
    void stop();

    qint64 readData(char *data, qint64 maxlen);
    qint64 writeData(const char *data, qint64 len);
    qint64 bytesAvailable() const;

    void NoteOn(unsigned c, double hertz);
    void NoteOff(unsigned c);
    void Touch_Real(unsigned c, unsigned volume);
    void Touch(unsigned c, unsigned volume);
    void Patch(unsigned c, unsigned i);
    void Pan(unsigned c, unsigned value);
    void PlayNoteF(int noteID, int patch, int chan2op1 = 8, int chan2op2 = 7, int chan4op1 = 1, int chan4op2 = 4);
public slots:
    void MuteNote();
    void PlayNote();
    void PlayMajorChord();
    void PlayMinorChord();
    void changePatch(int patch);
    void changeNote(int newnote);

signals:
    void saySomething(QString text);

private:
    int note;
    int metainstr;
    DBOPL::Handler chip;
    qint64 m_pos;
    QByteArray m_buffer;
};

#endif // GENERATOR_H
