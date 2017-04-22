#----------------------------------------------------------------------------
# OPL Bank Editor by Wohlstand, a free tool for music bank editing
# Copyright (c) 2016 Vitaly Novichkov <admin@wohlnet.ru>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#----------------------------------------------------------------------------

#-------------------------------------------------
#
# Project created by QtCreator 2016-06-07T13:26:28
#
#-------------------------------------------------

QT += core gui widgets multimedia

TEMPLATE = app

TARGET = opl3_bank_editor

android:{
    ARCH=android_arm
} else {
    !contains(QMAKE_TARGET.arch, x86_64) {
        ARCH=x32
    } else {
        ARCH=x64
    }
}

debug: {
BUILDTP=debug
    DEFINES += DEBUG_BUILD=1
    DESTDIR = $$PWD/bin-debug/
} else: release: {
    BUILDTP=release
    DESTDIR = $$PWD/bin-release/
}

BUILD_OBJ_DIR = $$PWD/_build_data

OBJECTS_DIR = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.obj
MOC_DIR     = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.moc
RCC_DIR     = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.rcc
UI_DIR      = $$BUILD_OBJ_DIR/_build_$$ARCH/$$TARGET/_$$BUILDTP/.ui

win32: RC_FILE = $$PWD/src/resources/res.rc

SOURCES += \
    src/FileFormats/dmxopl2.cpp \
    src/FileFormats/junlevizion.cpp \
    src/audio.cpp \
    src/bank.cpp \
    src/bank_editor.cpp \
    src/common.cpp \
    src/controlls.cpp \
    src/main.cpp \
    src/ins_names.cpp \
    src/opl/nukedopl3.c \
    src/opl/generator.cpp \
    src/piano.cpp \
    src/FileFormats/apogeetmb.cpp \
    src/FileFormats/sb_ibk.cpp \
    src/FileFormats/adlibbnk.cpp \
    src/importer.cpp \
    src/FileFormats/ffmt_base.cpp \
    src/FileFormats/milesopl.cpp


HEADERS += \
    src/FileFormats/dmxopl2.h \
    src/FileFormats/ffmt_base.h \
    src/FileFormats/junlevizion.h \
    src/bank.h \
    src/bank_editor.h \
    src/common.h \
    src/ins_names.h \
    src/opl/nukedopl3.h \
    src/opl/generator.h \
    src/piano.h \
    src/version.h \
    src/FileFormats/apogeetmb.h \
    src/FileFormats/sb_ibk.h \
    src/FileFormats/adlibbnk.h \
    src/importer.h \
    src/FileFormats/milesopl.h


FORMS += \
    src/bank_editor.ui \
    src/importer.ui

RESOURCES += \
    src/resources/resources.qrc

