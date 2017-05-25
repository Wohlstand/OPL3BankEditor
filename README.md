# OPL3BankEditor
A small cross-platform editor of the OPL3 FM banks of different formats

[![Build status](https://ci.appveyor.com/api/projects/status/llbyd0blk0i7amih?svg=true)](https://ci.appveyor.com/project/Wohlstand/opl3bankeditor)

## Currently supported bank formats
* Junglevision patch (*.OP3)
* DMX OPL-2 (OP2)
* Apogee Sound System timbre formats (*.TMB)
* SoundBlaster IBK files (*.IBK)
* AdLib/HMI BNK files (*.BNK)
* Global Timbre Library files for Audio Interface Library (*.AD, *.OPL)
* SB and O3 bank formats (a set of the concoctated SBI files) used with Linux drivers **(Read-only yet)**

## Comming soon
* *.adlraw - bank format created by Bisquit - author of ADLMIDI utiltiy
* Own bank format which supports all parameters provided by editor, and also implement support for GS and XG standard into ADLMIDI

## Download
* **Stable builds:** https://github.com/Wohlstand/OPL3BankEditor/releases
* **Fresh dev builds:**
  * [Download for Windows x86](http://wohlsoft.ru/docs/_laboratory/_Builds/win32/opl3-bank-editor/opl3-bank-editor-dev-win32.zip) (built by [AppVeyor](https://ci.appveyor.com/project/Wohlstand/opl3bankeditor))
  * CIs for other operating systems are coming soon... (However, it's easy to build it by yourself :wink:)

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

