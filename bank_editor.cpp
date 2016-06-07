#include <QFileDialog>
#include <QMessageBox>

#include "bank_editor.h"
#include "ui_bank_editor.h"
#include "ins_names.h"
#include "FileFormats/junlevizion.h"
#include "version.h"

BankEditor::BankEditor(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BankEditor)
{
    ui->setupUi(this);
    ui->version->setText(QString("%1, v.%2").arg(PROGRAM_NAME).arg(VERSION));
    setMelodic();
    connect(ui->melodic,    SIGNAL(clicked(bool)),  this,   SLOT(setMelodic()));
    connect(ui->percussion, SIGNAL(clicked(bool)),  this,   SLOT(setDrums()));
    m_curInst = NULL;
    m_lock = false;
    loadInstrument();
}

BankEditor::~BankEditor()
{
    delete ui;
}

void BankEditor::on_instruments_currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if(!current)
    {
        ui->curInsInfo->setText("<Not Selected>");
        m_curInst = NULL;
    } else  {
        ui->curInsInfo->setText(QString("%1 - %2").arg(current->data(Qt::UserRole).toInt()).arg(current->text()));
        setCurrentInstrument(current->data(Qt::UserRole).toInt(), ui->percussion->isChecked() );
    }
    loadInstrument();
}

void BankEditor::setCurrentInstrument(int num, bool isPerc)
{
    m_curInst = isPerc ? &m_bank.Ins_Percussion[num] : &m_bank.Ins_Melodic[num];
}

void BankEditor::loadInstrument()
{
    if(!m_curInst)
    {
        ui->editzone->setEnabled(false);
        return;
    }
    ui->editzone->setEnabled(true);

    m_lock = true;
    ui->perc_noteNum->setValue( m_curInst->percNoteNum );
    ui->op4mode->setChecked( m_curInst->en_4op );
    ui->op3->setEnabled( m_curInst->en_4op );
    ui->op4->setEnabled( m_curInst->en_4op );
    ui->feedback2->setEnabled( m_curInst->en_4op );
    ui->connect2->setEnabled( m_curInst->en_4op );
    ui->feedback2label->setEnabled( m_curInst->en_4op );

    ui->feedback1->setValue( m_curInst->feedback1 );
    ui->feedback2->setValue( m_curInst->feedback2 );

    ui->am1->setChecked( m_curInst->connection1==FmBank::Instrument::Connections::AM );
    ui->fm1->setChecked( m_curInst->connection1==FmBank::Instrument::Connections::FM );

    ui->am2->setChecked( m_curInst->connection2==FmBank::Instrument::Connections::AM );
    ui->fm2->setChecked( m_curInst->connection2==FmBank::Instrument::Connections::FM );


    ui->op1_attack->setValue(m_curInst->OP[CARRIER1].attack);
    ui->op1_decay->setValue(m_curInst->OP[CARRIER1].decay);
    ui->op1_sustain->setValue(m_curInst->OP[CARRIER1].sustain);
    ui->op1_release->setValue(m_curInst->OP[CARRIER1].release);
    ui->op1_waveform->setCurrentIndex(m_curInst->OP[CARRIER1].waveform);
    ui->op1_freqmult->setValue(m_curInst->OP[CARRIER1].fmult);
    ui->op1_level->setValue(m_curInst->OP[CARRIER1].level);
    ui->op1_ksl->setValue(m_curInst->OP[CARRIER1].ksl);
    ui->op1_vib->setChecked(m_curInst->OP[CARRIER1].vib);
    ui->op1_am->setChecked(m_curInst->OP[CARRIER1].am);
    ui->op1_eg->setChecked(m_curInst->OP[CARRIER1].eg);
    ui->op1_ksr->setChecked(m_curInst->OP[CARRIER1].ksr);

    ui->op2_attack->setValue(m_curInst->OP[MODULATOR1].attack);
    ui->op2_decay->setValue(m_curInst->OP[MODULATOR1].decay);
    ui->op2_sustain->setValue(m_curInst->OP[MODULATOR1].sustain);
    ui->op2_release->setValue(m_curInst->OP[MODULATOR1].release);
    ui->op2_waveform->setCurrentIndex(m_curInst->OP[MODULATOR1].waveform);
    ui->op2_freqmult->setValue(m_curInst->OP[MODULATOR1].fmult);
    ui->op2_level->setValue(m_curInst->OP[MODULATOR1].level);
    ui->op2_ksl->setValue(m_curInst->OP[MODULATOR1].ksl);
    ui->op2_vib->setChecked(m_curInst->OP[MODULATOR1].vib);
    ui->op2_am->setChecked(m_curInst->OP[MODULATOR1].am);
    ui->op2_eg->setChecked(m_curInst->OP[MODULATOR1].eg);
    ui->op2_ksr->setChecked(m_curInst->OP[MODULATOR1].ksr);

    ui->op3_attack->setValue(m_curInst->OP[CARRIER2].attack);
    ui->op3_decay->setValue(m_curInst->OP[CARRIER2].decay);
    ui->op3_sustain->setValue(m_curInst->OP[CARRIER2].sustain);
    ui->op3_release->setValue(m_curInst->OP[CARRIER2].release);
    ui->op3_waveform->setCurrentIndex(m_curInst->OP[CARRIER2].waveform);
    ui->op3_freqmult->setValue(m_curInst->OP[CARRIER2].fmult);
    ui->op3_level->setValue(m_curInst->OP[CARRIER2].level);
    ui->op3_ksl->setValue(m_curInst->OP[CARRIER2].ksl);
    ui->op3_vib->setChecked(m_curInst->OP[CARRIER2].vib);
    ui->op3_am->setChecked(m_curInst->OP[CARRIER2].am);
    ui->op3_eg->setChecked(m_curInst->OP[CARRIER2].eg);
    ui->op3_ksr->setChecked(m_curInst->OP[CARRIER2].ksr);

    ui->op4_attack->setValue(m_curInst->OP[MODULATOR2].attack);
    ui->op4_decay->setValue(m_curInst->OP[MODULATOR2].decay);
    ui->op4_sustain->setValue(m_curInst->OP[MODULATOR2].sustain);
    ui->op4_release->setValue(m_curInst->OP[MODULATOR2].release);
    ui->op4_waveform->setCurrentIndex(m_curInst->OP[MODULATOR2].waveform);
    ui->op4_freqmult->setValue(m_curInst->OP[MODULATOR2].fmult);
    ui->op4_level->setValue(m_curInst->OP[MODULATOR2].level);
    ui->op4_ksl->setValue(m_curInst->OP[MODULATOR2].ksl);
    ui->op4_vib->setChecked(m_curInst->OP[MODULATOR2].vib);
    ui->op4_am->setChecked(m_curInst->OP[MODULATOR2].am);
    ui->op4_eg->setChecked(m_curInst->OP[MODULATOR2].eg);
    ui->op4_ksr->setChecked(m_curInst->OP[MODULATOR2].ksr);

    m_lock = false;
}

void BankEditor::setMelodic()
{
    ui->instruments->clear();
    for(int i=0; i<128; i++)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(MidiInsName[i]);
        item->setData(Qt::UserRole, i);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        ui->instruments->addItem(item);
    }
}

void BankEditor::setDrums()
{
    ui->instruments->clear();
    for(int i=0; i<128; i++)
    {
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(MidiPercName[i]);
        item->setData(Qt::UserRole, i);
        item->setFlags(Qt::ItemIsSelectable|Qt::ItemIsEnabled);
        ui->instruments->addItem(item);
    }
}

void BankEditor::on_openBank_clicked()
{
    QString fileToOpen;
    fileToOpen = QFileDialog::getOpenFileName(this, "Open bank file", m_recentPath, "JunleVision bank (*.op3)");
    if(fileToOpen.isEmpty())
        return;

    int err = JunleVizion::loadFile(fileToOpen, m_bank);
    if(err != JunleVizion::ERR_OK)
    {
        QString errText;
        switch(err)
        {
            case JunleVizion::ERR_BADFORMAT: errText = "Bad file format"; break;
            case JunleVizion::ERR_NOFILE:    errText = "Can't open file"; break;
        }
        QMessageBox::warning(this, "Can't open bank file!", "Can't open bank file because "+errText, QMessageBox::Ok);
    } else {
        m_recentPath = fileToOpen;
        if(!ui->instruments->selectedItems().isEmpty())
            on_instruments_currentItemChanged(ui->instruments->selectedItems().first(), NULL);
        else
            on_instruments_currentItemChanged(NULL, NULL);
    }
}

void BankEditor::on_SaveBank_clicked()
{
    QString fileToOpen;
    fileToOpen = QFileDialog::getSaveFileName(this, "Save bank file", m_recentPath, "JunleVision bank (*.op3)");
    if(fileToOpen.isEmpty())
        return;

    int err = JunleVizion::saveFile(fileToOpen, m_bank);
    if(err != JunleVizion::ERR_OK)
    {
        QString errText;
        switch(err)
        {
            case JunleVizion::ERR_NOFILE:    errText = "Can't open file for write!"; break;
        }
        QMessageBox::warning(this, "Can't save bank file!", "Can't save bank file because "+errText, QMessageBox::Ok);
    }
}









void BankEditor::on_feedback1_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->feedback1 = arg1;
}

void BankEditor::on_am1_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection1 = FmBank::Instrument::AM;
}

void BankEditor::on_fm1_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection1 = FmBank::Instrument::FM;
}

void BankEditor::on_perc_noteNum_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->percNoteNum = arg1;
}

void BankEditor::on_feedback2_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->feedback2 = arg1;
}

void BankEditor::on_am2_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection2 = FmBank::Instrument::AM;
}

void BankEditor::on_fm2_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    if(checked)
        m_curInst->connection2 = FmBank::Instrument::FM;
}

void BankEditor::on_op4mode_clicked(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->en_4op = checked;
}




void BankEditor::on_op1_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].attack = uchar(arg1);
}

void BankEditor::on_op1_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].sustain = uchar(arg1);
}

void BankEditor::on_op1_decay_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].decay = uchar(arg1);
}

void BankEditor::on_op1_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].release = uchar(arg1);
}

void BankEditor::on_op1_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].level = uchar(arg1);
}

void BankEditor::on_op1_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].fmult = uchar(arg1);
}

void BankEditor::on_op1_ksl_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].ksl = uchar(arg1);
}

void BankEditor::on_op1_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].waveform = uchar(index);
}

void BankEditor::on_op1_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].am = checked;
}

void BankEditor::on_op1_vib_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].vib = checked;
}

void BankEditor::on_op1_eg_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].eg = checked;
}

void BankEditor::on_op1_ksr_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER1].ksr = checked;
}









void BankEditor::on_op2_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].attack = uchar(arg1);
}

void BankEditor::on_op2_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].sustain = uchar(arg1);
}

void BankEditor::on_op2_decay_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].decay = uchar(arg1);
}

void BankEditor::on_op2_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].release = uchar(arg1);
}

void BankEditor::on_op2_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].level = uchar(arg1);
}

void BankEditor::on_op2_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].fmult = uchar(arg1);
}

void BankEditor::on_op2_ksl_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].ksl = uchar(arg1);
}

void BankEditor::on_op2_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].waveform = uchar(index);
}

void BankEditor::on_op2_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].am = checked;
}

void BankEditor::on_op2_vib_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].vib = checked;
}

void BankEditor::on_op2_eg_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].eg = checked;
}

void BankEditor::on_op2_ksr_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR1].ksr = checked;
}










void BankEditor::on_op3_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].attack = uchar(arg1);
}

void BankEditor::on_op3_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].sustain = uchar(arg1);
}

void BankEditor::on_op3_decay_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].decay = uchar(arg1);
}

void BankEditor::on_op3_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].release = uchar(arg1);
}

void BankEditor::on_op3_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].level = uchar(arg1);
}

void BankEditor::on_op3_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].fmult = uchar(arg1);
}

void BankEditor::on_op3_ksl_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].ksl = uchar(arg1);
}

void BankEditor::on_op3_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].waveform = uchar(index);
}

void BankEditor::on_op3_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].am = checked;
}

void BankEditor::on_op3_vib_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].vib = checked;
}

void BankEditor::on_op3_eg_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].eg = checked;
}

void BankEditor::on_op3_ksr_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[CARRIER2].ksr = checked;
}














void BankEditor::on_op4_attack_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].attack = uchar(arg1);
}

void BankEditor::on_op4_sustain_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].sustain = uchar(arg1);
}

void BankEditor::on_op4_decay_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].decay = uchar(arg1);
}

void BankEditor::on_op4_release_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].release = uchar(arg1);
}

void BankEditor::on_op4_level_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].level = uchar(arg1);
}

void BankEditor::on_op4_freqmult_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].fmult = uchar(arg1);
}

void BankEditor::on_op4_ksl_valueChanged(int arg1)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].ksl = uchar(arg1);
}

void BankEditor::on_op4_waveform_currentIndexChanged(int index)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].waveform = uchar(index);
}

void BankEditor::on_op4_am_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].am = checked;
}

void BankEditor::on_op4_vib_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].vib = checked;
}

void BankEditor::on_op4_eg_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].eg = checked;
}

void BankEditor::on_op4_ksr_toggled(bool checked)
{
    if(m_lock) return;
    if(!m_curInst) return;
    m_curInst->OP[MODULATOR2].ksr = checked;
}




