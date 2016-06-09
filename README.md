# OPL3BankEditor
A small cross-platform editor of the OPL3 FM banks of different formats

Currently supported Junglevision format, planned to add TMB, DMX-OP2 and other formats

# How to build
You need a Qt 5 to build this project.

Run next commands from project directory:
```
qmake CONFIG+=release CONFIG-=debug FMBankEdit.pro
make
```

As alternate way you can open FMBankEdit.pro in the Qt Creator and build it.
