PATH=C:\Qt\Tools\mingw810_32\bin\;%PATH%
i686-w64-mingw32-gcc -m32 oplproxy.c InpOut32Helper.c liboplproxy.def -nostdlib -nodefaultlibs -shared -s -o liboplproxy.dll -lkernel32 -luser32 -e_DllMain@12 -Wl,--enable-stdcall-fixup
pause