#ifndef BANK_EDITOR_H
#define BANK_EDITOR_H

#include <QDialog>
#include <QList>
#include <QListWidgetItem>
#include "bank.h"

namespace Ui {
class BankEditor;
}

class BankEditor : public QDialog
{
    Q_OBJECT

public:
    explicit BankEditor(QWidget *parent = 0);
    ~BankEditor();
    QString m_recentPath;
    FmBank  m_bank;
    FmBank::Instrument* m_curInst;
    void setCurrentInstrument(int num, bool isPerc);
    void loadInstrument();

public slots:
    void setMelodic();
    void setDrums();

private slots:
    void on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *);

    void on_openBank_clicked();
    void on_SaveBank_clicked();

    void on_feedback1_valueChanged(int arg1);
    void on_am1_clicked(bool checked);
    void on_fm1_clicked(bool checked);

    void on_perc_noteNum_valueChanged(int arg1);

    void on_feedback2_valueChanged(int arg1);
    void on_am2_clicked(bool checked);
    void on_fm2_clicked(bool checked);

    void on_op4mode_clicked(bool checked);

    void on_op1_attack_valueChanged(int arg1);
    void on_op1_sustain_valueChanged(int arg1);
    void on_op1_decay_valueChanged(int arg1);
    void on_op1_release_valueChanged(int arg1);
    void on_op1_level_valueChanged(int arg1);
    void on_op1_freqmult_valueChanged(int arg1);
    void on_op1_ksl_valueChanged(int arg1);
    void on_op1_waveform_currentIndexChanged(int index);
    void on_op1_am_toggled(bool checked);
    void on_op1_vib_toggled(bool checked);
    void on_op1_eg_toggled(bool checked);
    void on_op1_ksr_toggled(bool checked);

    void on_op2_attack_valueChanged(int arg1);
    void on_op2_sustain_valueChanged(int arg1);
    void on_op2_decay_valueChanged(int arg1);
    void on_op2_release_valueChanged(int arg1);
    void on_op2_level_valueChanged(int arg1);
    void on_op2_freqmult_valueChanged(int arg1);
    void on_op2_ksl_valueChanged(int arg1);
    void on_op2_waveform_currentIndexChanged(int index);
    void on_op2_am_toggled(bool checked);
    void on_op2_vib_toggled(bool checked);
    void on_op2_eg_toggled(bool checked);
    void on_op2_ksr_toggled(bool checked);

    void on_op3_attack_valueChanged(int arg1);
    void on_op3_sustain_valueChanged(int arg1);
    void on_op3_decay_valueChanged(int arg1);
    void on_op3_release_valueChanged(int arg1);
    void on_op3_level_valueChanged(int arg1);
    void on_op3_freqmult_valueChanged(int arg1);
    void on_op3_ksl_valueChanged(int arg1);
    void on_op3_waveform_currentIndexChanged(int index);
    void on_op3_am_toggled(bool checked);
    void on_op3_vib_toggled(bool checked);
    void on_op3_eg_toggled(bool checked);
    void on_op3_ksr_toggled(bool checked);

    void on_op4_attack_valueChanged(int arg1);
    void on_op4_sustain_valueChanged(int arg1);
    void on_op4_decay_valueChanged(int arg1);
    void on_op4_release_valueChanged(int arg1);
    void on_op4_level_valueChanged(int arg1);
    void on_op4_freqmult_valueChanged(int arg1);
    void on_op4_ksl_valueChanged(int arg1);
    void on_op4_waveform_currentIndexChanged(int index);
    void on_op4_am_toggled(bool checked);
    void on_op4_vib_toggled(bool checked);
    void on_op4_eg_toggled(bool checked);
    void on_op4_ksr_toggled(bool checked);

private:
    Ui::BankEditor *ui;
    bool m_lock;
};

#endif // BANK_EDITOR_H
