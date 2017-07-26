PATH=C:\MinGW\bin;%PATH%

IF -%1-==-make- goto DoMake
IF -%1-==-makeone- goto DoMakeOne

configure.exe ^
 -platform win32-g++ ^
 -static -debug-and-release -fast ^
 -no-qt3support -no-webkit ^
 -qt-zlib -qt-gif -no-libtiff -qt-libpng -no-libmng -no-libjpeg ^
 -no-dbus -no-openssl ^
 -no-accessibility -no-opengl -no-dsp -no-vcproj ^
 -no-mmx -no-3dnow -no-sse -no-sse2 ^
 -no-phonon -no-phonon-backend ^
 -prefix "C:/Qt/4.4.3"
rem -no-phonon -no-phonon-backend
 
pause

:DoMake
mingw32-make -j 4
goto Qit

:DoMakeOne
mingw32-make
goto Qit

:Qit
pause