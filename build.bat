@echo off
call _paths.bat

PATH=%QtDir%;%MinGW%;%GitDir%;%PATH%
SET SEVENZIP=C:\Program Files\7-Zip

IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles(x86)%\7-Zip
IF NOT EXIST "%SEVENZIP%\7z.exe" SET SEVENZIP=%ProgramFiles%\7-Zip

lrelease FMBankEdit.pro

qmake FMBankEdit.pro CONFIG+=release CONFIG-=debug
IF ERRORLEVEL 1 goto error

mingw32-make
IF ERRORLEVEL 1 goto error

md opl3-bank-editor
IF NOT -%1-==-win9x- (
    cd bin-release
    md translations
    copy "..\src\translations\*.qm" translations
    windeployqt opl3_bank_editor.exe
    IF ERRORLEVEL 1 goto error
    cd ..
) ELSE (
    cd bin-release
    md translations
    copy "..\src\translations\*.qm" translations
    cd ..
)

SET DEST_ARCHIVE=opl3-bank-editor-dev-win32.zip
SET DEPLOY_FILES=.\bin-release\*
IF -%1-==-win9x- (
    SET DEPLOY_FILES=%DEPLOY_FILES% .\opl_proxy\win9x\liboplproxy.dll
    SET DEST_ARCHIVE=opl3-bank-editor-dev-win9x.zip
) ELSE (
    SET DEPLOY_FILES=%DEPLOY_FILES% .\opl_proxy\modern\liboplproxy.dll .\opl_proxy\modern\inpout32.dll
)
SET DEPLOY_FILES=%DEPLOY_FILES% Bank_Examples
SET DEPLOY_FILES=%DEPLOY_FILES% .\formats_info.htm .\license.txt .\changelog.txt

"%SEVENZIP%\7z" a -tzip "opl3-bank-editor\%DEST_ARCHIVE%" %DEPLOY_FILES%

goto quit
:error
echo ==============BUILD ERRORED!===============
:quit

