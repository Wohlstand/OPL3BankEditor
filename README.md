# OPL3BankEditor
A small cross-platform editor of the OPL3 FM banks of different formats

Currently supported Junglevision patch, DMX OPL-2, Apogee Sound System timbre formats, SoundBlaster IBK files. Other formats (*.bnk, *.opl, *.ad, *.adlraw) are planned

# How to build
You need a Qt 5 to build this project.

Run next commands from project directory:
```
qmake CONFIG+=release CONFIG-=debug FMBankEdit.pro
make
```

As alternate way you can open FMBankEdit.pro in the Qt Creator and build it.

# Folders
* ***Bank_Examples*** - example bank files which you can edit and preview them
* ***src*** - source code of this tool
* ***_Misc*** - Various stuff (test scripts, dummy banks, documents, etc.) which was been used in development of this tool

