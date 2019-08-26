# OPL3BankEditor
![OPL3 Editor Logo](src/resources/opl3_48.png)

A small cross-platform editor of the OPL3 FM banks of different formats

# CI Build status
Linux | Windows | macOS
------------ | ------------- | -------------
[![Build Status](https://travis-ci.org/Wohlstand/OPL3BankEditor.svg?branch=master)](https://travis-ci.org/Wohlstand/OPL3BankEditor) | [![Build status](https://ci.appveyor.com/api/projects/status/llbyd0blk0i7amih?svg=true)](https://ci.appveyor.com/project/Wohlstand/opl3bankeditor) | [![Build Status](https://travis-ci.org/Wohlstand/OPL3BankEditor.svg?branch=master)](https://travis-ci.org/Wohlstand/OPL3BankEditor)

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
* **Fresh dev builds:**
  * [Download for Windows x86 (XP/7/8/8.1/10 with Nuked OPL3 emulator)](http://wohlsoft.ru/docs/_laboratory/_Builds/win32/opl3-bank-editor/opl3-bank-editor-dev-win32.zip) (built by [AppVeyor](https://ci.appveyor.com/project/Wohlstand/opl3bankeditor))
  * [Download for Windows x86 (98/ME with proxy to real OPL3 chip)](http://wohlsoft.ru/docs/_laboratory/_Builds/win32/opl3-bank-editor/opl3-bank-editor-dev-win9x.zip) (built by [AppVeyor](https://ci.appveyor.com/project/Wohlstand/opl3bankeditor))
  * [Download for macOS x64 (DMG)](http://wohlsoft.ru/docs/_laboratory/_Builds/macosx/opl3-bank-editor/opl3_bank_editor-macos.dmg) (built by [Travis-CI](https://travis-ci.org/Wohlstand/OPL3BankEditor))
  * [Download for macOS x64 (ZIP)](http://wohlsoft.ru/docs/_laboratory/_Builds/macosx/opl3-bank-editor/opl3_bank_editor-macos.zip) (built by [Travis-CI](https://travis-ci.org/Wohlstand/OPL3BankEditor))
  * CIs for other operating systems are coming soon... (However, it's easy to build it by yourself :wink:)

# How to build
Please, see [the wiki](https://github.com/Wohlstand/OPL3BankEditor/wiki).

As alternate way you can open FMBankEdit.pro in the Qt Creator and build it.

# Folders
* ***Bank_Examples*** - example bank files which you can edit and preview them
* ***src*** - source code of this tool
* ***_Misc*** - Various stuff (test scripts, dummy banks, documents, etc.) which was been used in development of this tool

