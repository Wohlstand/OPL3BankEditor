PATH=C:\Qt\Tools\mingw810_64\bin\;%PATH%
x86_64-w64-mingw32-gcc -m64 oplproxy.c InpOut32Helper.c liboplproxy.def -nostdlib -nodefaultlibs -shared -s -o liboplproxy64.dll -lkernel32 -luser32 -eDllMain@12 -Wl,--enable-stdcall-fixup
pause