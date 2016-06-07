#include "bank_editor.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    a.setStyle("Windows");
    BankEditor w;
    w.show();

    return a.exec();
}
