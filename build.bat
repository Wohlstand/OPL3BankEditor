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
rem cd bin-release
rem windeployqt opl3_bank_editor.exe
rem IF ERRORLEVEL 1 goto error
rem cd ..

SET DEST_ARCHIVE=opl3-bank-editor-dev-win32.zip
SET DEPLOY_FILES=.\bin-release\*
IF -%1-==-win9x- (
    SET DEPLOY_FILES=%DEPLOY_FILES% .\opl_proxy\liboplproxy.dll
    SET DEST_ARCHIVE=opl3-bank-editor-dev-win9x.zip
)
SET DEPLOY_FILES=%DEPLOY_FILES% Bank_Examples
SET DEPLOY_FILES=%DEPLOY_FILES% .\formats_info.htm .\license.txt .\changelog.txt

"%SEVENZIP%\7z" a -tzip "opl3-bank-editor\%DEST_ARCHIVE%" %DEPLOY_FILES%

goto quit
:error
echo ==============BUILD ERRORED!===============
:quit

