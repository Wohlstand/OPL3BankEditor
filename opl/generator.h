#ifndef GENERATOR_H
#define GENERATOR_H

#include "dbopl.h"
#include "../bank.h"

#include <QIODevice>
#include <QObject>

struct adldata
{
    unsigned int  modulator_E862, carrier_E862;  // See below
    unsigned char modulator_40, carrier_40; // KSL/attenuation settings
    unsigned char feedconn; // Feedback/connection bits for the channel
    signed char   finetune;
};

struct OPL_PatchSetup
{
    enum { Flag_True4op = 0x04, Flag_Pseudo4op = 0x01, Flag_NoSound = 0x02 };
    adldata OPS[2];
    unsigned char  tone;
    unsigned char  flags;
    unsigned short ms_sound_kon;  // Number of milliseconds it produces sound;
    unsigned short ms_sound_koff;
    double         voice2_fine_tune;//Pseudo-4op second voice only
};

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
    void PlayNoteF(int noteID, int chan2op1 = 8, int chan2op2 = 7, int chan4op1 = 1, int chan4op2 = 4);
public slots:
    void MuteNote();
    void PlayNote();
    void PlayMajorChord();
    void PlayMinorChord();
    void changePatch(const FmBank::Instrument &instrument, bool isDrum=false);
    void changeNote(int newnote);
    void changeDeepTremolo(bool enabled);
    void changeDeepVibrato(bool enabled);

signals:
    void saySomething(QString text);

private:
    int note;
    unsigned char DeepTremoloMode;
    unsigned char DeepVibratoMode;
    DBOPL::Handler chip;
    OPL_PatchSetup m_patch;
};

#endif // GENERATOR_H
