This is a toolset to build Qt 4.4.3 under MinGW 5.3.

It's required to build Windows 98-compatible assembly of OPL3 Bank Editor
with support for a testing on real OPL3 chip.

Why MinGW 5.3 and not 3.2 (which officially supported by Qt 4.4.3)?
- We need for C++11 which becoming with G++ 4.8, G++ 3.2 has no C++11 support

============= How to build: ============= 

- Install MinGW (from http://mingw.org), you must also install libz-dev and mingwex-dev
	(you can find them in the packages in the installer)
- Download old Qt 4.4.3 tarball and unpack it's contents into C:\Qt\4.4.3 (custom paths 
	are not allowed or it will fail to find self while configure and make).
	You can download it here (from the official site):

	http://download.qt.io/archive/qt/4.4/qt-all-opensource-src-4.4.3.zip

- Unpack content of "Qt 4.4.3 patch for MinGW 5.3.zip" into C:\Qt\4.4.3 with file replacing.
	This patch contains: QMake from 4.5.3 (original QMake is incompatible with latest 
	Qt Creator, and can't be added as "Qt toolchain"), fixed sources where are was
	errors which are preventing build, and disabled unneeded 
	(or broken/unbuildable) parts by editing of pro/pri files

- Put "make-qt-4-4-3.bat" into C:\Qt\4.4.3\ and run it.

- Confirm license by typing "y" with hiting Enter

- After project was configured, hit any key and wait when it will be built

- After project successfully built, hit any key again, and statical debug+release assembly
	is ready for usage! ;-)
