# OPL3BankEditor
A small cross-platform editor of the OPL3 FM banks of different formats

## Currently supported bank formats
* Junglevision patch (*.OP3)
* DMX OPL-2 (OP2)
* Apogee Sound System timbre formats (*.TMB)
* SoundBlaster IBK files (*.IBK)
* AdLib/HMI BNK files (*.BNK)
* Global Timbre Library files for Audio Interface Library (*.AD, *.OPL) **(Read-only yet)**

## Comming soon
* *.adlraw - bank format created by Bisquit - author of ADLMIDI utiltiy
* SB and O3 bank formats (a set of the concoctated SBI files) used with Linux drivers
* Own bank format which supports all parameters provided by editor, and also implement support for GS and XG standard into ADLMIDI

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

