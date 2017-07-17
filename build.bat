@echo off
call _paths.bat

PATH=%QtDir%;%MinGW%;%GitDir%;%PATH%
SET SEVENZIP=C:\Program Files\7-Zip

IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles(x86)%\7-Zip
IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles%\7-Zip

qmake FMBankEdit.pro CONFIG+=release CONFIG-=debug
IF ERRORLEVEL 1 goto error

mingw32-make
IF ERRORLEVEL 1 goto error

md opl3-bank-editor
cd bin-release
windeployqt opl3_bank_editor.exe
IF ERRORLEVEL 1 goto error
cd ..

"%SEVENZIP%\7z" a -tzip "opl3-bank-editor\opl3-bank-editor-dev-win32.zip" .\formats_info.htm .\license.txt .\changelog.txt .\bin-release\* Bank_Examples

goto quit
:error
echo ==============BUILD ERRORED!===============
:quit

