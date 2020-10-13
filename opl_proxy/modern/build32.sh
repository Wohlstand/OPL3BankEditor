#!/bin/bash

i686-w64-mingw32-gcc -m32 oplproxy.c InpOut32Helper.c -nostdlib -nodefaultlibs -shared -s -o liboplproxy.dll -lkernel32 -luser32 -e_DllMain@12

