# OPL3BankEditor
![OPL3 Editor Logo](src/resources/opl3_48.png)

A small cross-platform editor of the OPL3 FM banks of different formats

# CI Build status
Linux | Windows | macOS
------------ | ------------- | -------------
[![Build Status](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml/badge.svg)](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml) | [![Build status](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/windows-ci.yml/badge.svg)](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/windows-ci.yml) | [![Build Status](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/macos-ci.yml/badge.svg)](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/macos-ci.yml)

## Currently supported bank formats
* Own bank format (.WOPL) (Specification in the **WOPL-and-OPLI-Specification.txt** file)
* Junglevision patch (.OP3)
* DMX OPL-2 (.OP2) ([Specification](http://www.shikadi.net/moddingwiki/OP2_Bank_Format))
* Apogee Sound System timbre formats (.TMB) ([Specification](http://www.shikadi.net/moddingwiki/Apogee_Sound_System_Timbre_Format))
* SoundBlaster IBK files (.IBK) ([Specification](http://www.shikadi.net/moddingwiki/IBK_Format))
* AdLib/HMI BNK files (.BNK) ([Specification](http://www.shikadi.net/moddingwiki/AdLib_Instrument_Bank_Format))
* Adlib Gold files (.BNK) (specification is informal and can only be retrieved from source code)
* AdLib Timbre bank files (.SND, .TIM) ([Specification](http://www.shikadi.net/moddingwiki/AdLib_Timbre_Bank_Format))
* Global Timbre Library files for Audio Interface Library (.AD, .OPL) ([Specification](http://www.shikadi.net/moddingwiki/Global_Timbre_Library))
* SB and O3 bank formats (a set of the concatenated SBI files) used with Linux drivers
* Bisqwit's ADLMIDI bank (.ADLRAW)

## Currently supported instrument formats
* Own 2/4-operator instrument format (.OPLI) (Specification in the **WOPL-and-OPLI-Specification.txt** file)
* 2-operator Sound Blaster instruments for DOS and UNIX (.SBI) ([Specification](http://www.shikadi.net/moddingwiki/SBI_Format))
* 4-operator Sound Blaster instruments for UNIX (.SBI)
* Legacy AdLib instruments (.INS) ([Specification](http://www.shikadi.net/moddingwiki/AdLib_Instrument_Bank_Format))

## Currently supported music formats to import instruments
* Id-Software Music Format (.IMF) ([Specification](http://www.shikadi.net/moddingwiki/IMF_Format))
* Creative Music Format (.CMF) ([Specification](http://www.shikadi.net/moddingwiki/CMF_Format))
* Reality ADlib Tracker Music Format (.RAD) ([Specification](http://hackipedia.org/File%20formats/Music/Sample%20based/text/Reality%20ADlib%20Tracker%20format.cp437.txt.utf-8.txt))

## Download
* **Stable builds:** https://github.com/Wohlstand/OPL3BankEditor/releases
* **Fresh dev auto-builds:**
  * **Windows**:
    * [Download for Windows x86_64 (7/8/8.1/10 with Nuked OPL3 emulator)](https://wohlsoft.ru/docs/_laboratory/_Builds/win32/opl3-bank-editor/opl3-bank-editor-dev-win64.zip) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/windows-ci.yml))
    * [Download for Windows x86 (XP/Vista/7/8/8.1/10 with Nuked OPL3 emulator)](https://wohlsoft.ru/docs/_laboratory/_Builds/win32/opl3-bank-editor/opl3-bank-editor-dev-win32.zip) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/windows-ci.yml))
    * [Download for Windows x86 (98/ME with proxy to real OPL3 chip)](https://wohlsoft.ru/docs/_laboratory/_Builds/win32/opl3-bank-editor/opl3-bank-editor-dev-win9x.zip) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/windows-ci.yml))
  * **macOS**:
    * <s>[Download for macOS x64 (DMG)](https://wohlsoft.ru/docs/_laboratory/_Builds/macosx/opl3-bank-editor/opl3_bank_editor-macos.dmg)</s> (Under construction)
    * <s>[Download for macOS x64 (ZIP)](https://wohlsoft.ru/docs/_laboratory/_Builds/macosx/opl3-bank-editor/opl3_bank_editor-macos.zip)</s> (Under construction)
  * **Ubuntu**:
    * [Download for Ubuntu 24.04 for Qt5 x64 (DEB)](https://builds.wohlsoft.ru/ubuntu-24-04/opl3-bank-editor-qt5-ubuntu-24-04-amd64-master.deb) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml))
    * [Download for Ubuntu 24.04 for Qt6 x64 (DEB) (Plots unsupported)](https://builds.wohlsoft.ru/ubuntu-24-04/opl3-bank-editor-qt6-ubuntu-24-04-amd64-master.deb) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml))
    * [Download for Ubuntu 22.04 x64 (DEB)](https://builds.wohlsoft.ru/ubuntu-22-04/opl3-bank-editor-ubuntu-22-04-amd64-master.deb) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml))
    * [Download for Ubuntu 20.04 x64 (DEB)](https://builds.wohlsoft.ru/ubuntu-20-04/opl3-bank-editor-ubuntu-20-04-amd64-master.deb) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml))
    * [Download for Ubuntu 18.04 x64 (DEB)](https://builds.wohlsoft.ru/ubuntu-18-04/opl3-bank-editor-ubuntu-18-04-amd64-master.deb) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml))
    * [Download for Ubuntu 16.04 x64 (DEB)](https://builds.wohlsoft.ru/ubuntu-16-04/opl3-bank-editor-ubuntu-16-04-amd64-master.deb) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml))
    * [Download for Ubuntu 14.04 x64 (DEB) (Plots unsupported)](https://builds.wohlsoft.ru/ubuntu-14-04/opl3-bank-editor-ubuntu-14-04-amd64-master.deb) (built by [GitHub Actions](https://github.com/Wohlstand/OPL3BankEditor/actions/workflows/ubuntu-ci.yml))

# How to build
Please, see [the wiki](https://github.com/Wohlstand/OPL3BankEditor/wiki).

As alternate way you can open FMBankEdit.pro in the Qt Creator and build it.

# Folders
* ***Bank_Examples*** - example bank files which you can edit and preview them
* ***src*** - source code of this tool
* ***_Misc*** - Various stuff (test scripts, dummy banks, documents, etc.) which was been used in development of this tool
