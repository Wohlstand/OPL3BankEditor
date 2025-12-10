#ifndef BANK_TYPE_DLG_H
#define BANK_TYPE_DLG_H

#include <QDialog>
#include <QObject>

class FmBankFormatBase;

class BankType : public QDialog
{
    Q_OBJECT
    int m_reply = 0;
    bool m_ibkSkipNonRhythm = false;
    FmBankFormatBase *m_dst = nullptr;

public:
    explicit BankType(const QString &filePath, FmBankFormatBase *fmt, QWidget *parent);

    int getAnswer();

    bool ibkSkipNonRhythm();

private slots:
    void on_accept();
    void answer_melodic();
    void answer_drums();
    void set_ibk_nonrhythm_skip(bool checked);
};

#endif // BANK_TYPE_DLG_H
