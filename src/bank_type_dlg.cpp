#include <QFileInfo>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QCheckBox>

#include "bank_type_dlg.h"
#include "FileFormats/ffmt_base.h"
#include "FileFormats/ffmt_enums.h"

BankType::BankType(const QString &filePath, FmBankFormatBase *fmt, QWidget *parent) :
    QDialog(parent), m_dst(fmt)
{
    Q_ASSERT(fmt);

    QFileInfo f(filePath);
    QGridLayout *l = new QGridLayout(this);
    QLabel *q = new QLabel(tr("Select the slot to load the half of the bank %1:").arg(f.fileName()));
    QPushButton *melo = new QPushButton(tr("Melodic"), this);
    QPushButton *perc = new QPushButton(tr("Percussion"), this);

    setWindowTitle(tr("Bank type selection"));

    l->addWidget(q, 0, 0, 1, 2);
    l->addWidget(melo, 1, 0);
    l->addWidget(perc, 1, 1);

    if(fmt->formatId() == BankFormats::FORMAT_IBK)
    {
        QCheckBox *skipNonRhythm = new QCheckBox(tr("Skip non-rhythm percussion instruments"), this);
        l->addWidget(skipNonRhythm, 2, 0, 1, 2);
        QObject::connect(skipNonRhythm, SIGNAL(toggled(bool)), this, SLOT(set_ibk_nonrhythm_skip(bool)));
    }

    QObject::connect(melo, SIGNAL(clicked()), this, SLOT(answer_melodic()));
    QObject::connect(perc, SIGNAL(clicked()), this, SLOT(answer_drums()));
    QObject::connect(this, SIGNAL(accepted()), this, SLOT(on_accept()));
}

int BankType::getAnswer()
{
    return m_reply;
}

bool BankType::ibkSkipNonRhythm()
{
    return m_ibkSkipNonRhythm;
}

void BankType::on_accept()
{
    m_dst->setLoadAsDrums(m_reply == 2);

    if(m_dst->formatId() == BankFormats::FORMAT_IBK)
        m_dst->setIbkSkipNonRhythm(m_ibkSkipNonRhythm);
}

void BankType::answer_melodic()
{
    m_reply = 1;
    accept();
}

void BankType::answer_drums()
{
    m_reply = 2;
    accept();
}

void BankType::set_ibk_nonrhythm_skip(bool checked)
{
    m_ibkSkipNonRhythm = checked;
}
