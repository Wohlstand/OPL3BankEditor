#include <QString>
#include <QtTest>
#include <QRandomGenerator>

#include <bank.h>
#include <FileFormats/format_wohlstand_opl3.h>
#include <FileFormats/wopl/wopl_file.h>

class Wopl_rwTest : public QObject
{
    Q_OBJECT

    FmBank bank1;
    FmBank bank2;
    QString bankPath;

    QString getRandomString(const int randomStringLength = 12) const
    {
       const QString possibleCharacters("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789 \".,(){}[];:'");
       QString randomString;
       QRandomGenerator gen;
       for(int i=0; i < randomStringLength; ++i)
       {
           int index = gen.bounded(0, possibleCharacters.length());
           QChar nextChar = possibleCharacters.at(index);
           randomString.append(nextChar);
       }
       return randomString;
    }

    void fillRandRaw(uint8_t *dst, size_t maxLen)
    {
        QRandomGenerator gen;
        for(size_t i = 0; i < maxLen; i++)
            *dst++ = (uint8_t)gen.bounded(0, 255);
    }

    void fillRandString(char *dst, int maxLen)
    {
        QString rnd = getRandomString(maxLen);
        QByteArray u = rnd.toLatin1();
        memset(dst, 0, (size_t)maxLen);
        strncpy(dst, u.data(), (size_t)u.size());
    }

    static QByteArray dumpFile(QString path, bool noRemove = false)
    {
        QFile f(path);
        QByteArray gotData;
        if(!f.open(QIODevice::ReadOnly))
            return QByteArray();
        gotData = f.readAll();
        f.close();
        if(!noRemove)
            f.remove();
        return gotData;
    }

    void generateRandomBank(WOPLBank &bank)
    {
        QRandomGenerator gen;
        bank.bank_midi_lsb = (uint8_t)gen.bounded(0, 255);
        bank.bank_midi_msb = (uint8_t)gen.bounded(0, 255);
        memset(bank.bank_name, 0, 33);
        fillRandString(bank.bank_name, 32);
        for(size_t i = 0; i < 128; i++)
        {
            WOPLInstrument &ins = bank.ins[i];
            fillRandRaw((uint8_t*)&ins, sizeof(WOPLInstrument));
            memset(ins.inst_name, 0, 34);
            fillRandString(ins.inst_name, 32);
            ins.inst_flags &= WOPL_Ins_ALL_MASK;
        }
    }

    WOPLFile * getRandomWopl()
    {
        QRandomGenerator gen;
        uint16_t melodics = (uint16_t)gen.bounded(1, 128);
        uint16_t percussions = (uint16_t)gen.bounded(1, 128);

        WOPLFile* probe = WOPL_Init(melodics, percussions);
        probe->version = (uint16_t)gen.bounded(1, 3);
        probe->opl_flags = (uint8_t)gen.bounded(0, 3);
        probe->volume_model = (uint8_t)gen.bounded(0, 6);
        for(uint16_t i = 0; i < melodics; i++)
            generateRandomBank(probe->banks_melodic[i]);
        for(uint16_t i = 0; i < percussions; i++)
            generateRandomBank(probe->banks_percussive[i]);
        return probe;
    }

public:
    Wopl_rwTest();

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void loadBankOld()
    {
        WohlstandOPL3 format;
        QFile getMagic(bankPath);
        char magic[32];
        QVERIFY2(getMagic.open(QIODevice::ReadOnly), "open Get Magic");
        QVERIFY2(getMagic.read(magic, 32) == 32, "read 32 bytes");
        getMagic.close();
        QVERIFY2(format.detect(bankPath, magic), "Detect WOPL");
        QBENCHMARK {
            QVERIFY2(format.loadFileOLD(bankPath, bank1) == FfmtErrCode::ERR_OK, "Load bank data by old parser");
        }
    }
    void loadBankNew()
    {
        WohlstandOPL3 format;
        QFile getMagic(bankPath);
        char magic[32];
        QVERIFY2(getMagic.open(QIODevice::ReadOnly), "open Get Magic");
        QVERIFY2(getMagic.read(magic, 32) == 32, "read 32 bytes");
        getMagic.close();
        QVERIFY2(format.detect(bankPath, magic), "Detect WOPL");
        QBENCHMARK {
            QVERIFY2(format.loadFile(bankPath, bank2) == FfmtErrCode::ERR_OK, "Load bank data by new parser");
        }
    }
    void compareResults()
    {
        QVERIFY2(bank1 == bank2, "Loaded banks are not matching!");
    }

    void saveFileTest()
    {
        WohlstandOPL3 format;
        QVERIFY2(format.saveFileOLD("crap1-old.wopl", bank1) == FfmtErrCode::ERR_OK, "Saving by old algo");
        QVERIFY2(format.saveFileOLD("crap2-old.wopl", bank2) == FfmtErrCode::ERR_OK, "Saving by old algo");
        QVERIFY2(format.saveFile("crap1-new.wopl", bank1) == FfmtErrCode::ERR_OK, "Saving by new algo");
        QVERIFY2(format.saveFile("crap2-new.wopl", bank2) == FfmtErrCode::ERR_OK, "Saving by new algo");
        QByteArray etaloneData = dumpFile(bankPath, true);
        QByteArray crap1old = dumpFile("crap1-old.wopl");
        QByteArray crap2old = dumpFile("crap2-old.wopl");
        QByteArray crap1new = dumpFile("crap1-new.wopl");
        QByteArray crap2new = dumpFile("crap2-new.wopl");
        QVERIFY2(etaloneData.size() == crap1old.size(), "Size difference 1 old");
        QVERIFY2(etaloneData.size() == crap2old.size(), "Size difference 2 old");
        QVERIFY2(etaloneData.size() == crap1new.size(), "Size difference 1 new");
        QVERIFY2(etaloneData.size() == crap2new.size(), "Size difference 2 new");
        QVERIFY2(etaloneData == crap1old, "Data difference 1 old");
        QVERIFY2(etaloneData == crap2old, "Data difference 2 old");
        QVERIFY2(etaloneData == crap1new, "Data difference 1 new");
        QVERIFY2(etaloneData == crap2new, "Data difference 2 new");
    }

    void benchmarkOldWrite()
    {
        WohlstandOPL3 format;
        QBENCHMARK {
            QVERIFY2(format.saveFileOLD("crap1-old.wopl", bank1) == FfmtErrCode::ERR_OK, "Saving by old algo");
        }
        QFile("crap1-old.wopl").remove();
    }

    void benchmarkNewWrite()
    {
        WohlstandOPL3 format;
        QBENCHMARK {
            QVERIFY2(format.saveFile("crap1-old.wopl", bank1) == FfmtErrCode::ERR_OK, "Saving by new algo");
        }
        QFile("crap1-old.wopl").remove();
    }

    void stabilityOverInputShit()
    {
        QRandomGenerator gen;
        uint junkDataBig[101803];
        uint junkDataFew[1500];

        QBENCHMARK {
            gen.fillRange(junkDataBig, 101803);
            gen.fillRange(junkDataFew, 1500);
            int err = 0;
            WOPLFile* probe;
            probe = WOPL_LoadBankFromMem((void*)junkDataBig, sizeof(size_t) * 101803, &err);
            QVERIFY2(probe == nullptr, "Must be null!");
            QVERIFY2(err != WOPL_ERR_OK, "Must be errored!");

            probe = WOPL_LoadBankFromMem((void*)junkDataFew, sizeof(size_t) * 1500, &err);
            QVERIFY2(probe == nullptr, "Must be null!");
            QVERIFY2(err != WOPL_ERR_OK, "Must be errored!");
        }
    }

    void randomizedBankDataTest()
    {
        QBENCHMARK {
            WOPLFile *probe_src = getRandomWopl();
            size_t fileSize = WOPL_CalculateBankFileSize(probe_src, 0);

            QByteArray probeData;
            probeData.resize((int)fileSize);
            QVERIFY2(WOPL_SaveBankToMem(probe_src, probeData.data(), fileSize, 0, 0) == WOPL_ERR_OK, "Write failed");

            int err = 0;
            WOPLFile *probe_dst = WOPL_LoadBankFromMem(probeData.data(), fileSize, &err);
            QVERIFY2(probe_dst != nullptr, "Must NOT be null!");
            QVERIFY2(err == WOPL_ERR_OK, "Must be Ok!");

            // Don't use version in comparison, just make them be equal... to zero!
            probe_src->version = 0;
            probe_dst->version = 0;

            QVERIFY2(WOPL_BanksCmp(probe_src, probe_dst) == 1, "Generated and re-readed banks are DIFFERENT!");

            WOPL_Free(probe_src);
            WOPL_Free(probe_dst);
        }
    }
};

Wopl_rwTest::Wopl_rwTest() :
    bankPath(":/test.wopl")
{}

void Wopl_rwTest::initTestCase()
{
    bank1.reset();
    bank2.reset();
}

void Wopl_rwTest::cleanupTestCase()
{
}

QTEST_APPLESS_MAIN(Wopl_rwTest)

#include <tst_wopl_rwtest.moc>
