#----------------------------------------------------------------------------
# OPL Bank Editor by Wohlstand, a free tool for music bank editing
# Copyright (c) 2016-2017 Vitaly Novichkov <admin@wohlnet.ru>
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

QT += core gui
greaterThan(QT_MAJOR_VERSION, 4):{
    QT += widgets multimedia
    DEFINES += ENABLE_AUDIO_TESTING
    CONFIG += c++11
} else {
    QMAKE_CXXFLAGS += -std=c++11
    win32: {
        CONFIG += static
        QMAKE_LFLAGS += -static-libgcc -static-libstdc++ -static
        #DEFINES += snprintf=_snprintf
        DEFINES += NO_NATIVE_OPEN_DIALOGS
    }
}

TEMPLATE = app
TARGET = opl3_bank_editor
INCLUDEPATH += $$PWD/

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
macx: ICON = $$PWD/src/resources/opl3.icns

SOURCES += \
    src/audio.cpp \
    src/bank.cpp \
    src/bank_editor.cpp \
    src/common.cpp \
    src/controlls.cpp \
    src/FileFormats/ffmt_base.cpp \
    src/FileFormats/ffmt_factory.cpp \
    src/FileFormats/format_adlib_bnk.cpp \
    src/FileFormats/format_adlib_tim.cpp \
    src/FileFormats/format_ail2_gtl.cpp \
    src/FileFormats/format_apogeetmb.cpp \
    src/FileFormats/format_bisqwit.cpp \
    src/FileFormats/format_cmf_importer.cpp \
    src/FileFormats/format_dmxopl2.cpp \
    src/FileFormats/format_imf_importer.cpp \
    src/FileFormats/format_junlevizion.cpp \
    src/FileFormats/format_rad_importer.cpp \
    src/FileFormats/format_sb_ibk.cpp \
    src/FileFormats/format_wohlstand_opl3.cpp \
    src/importer.cpp \
    src/ins_names.cpp \
    src/main.cpp \
    src/opl/generator.cpp \
    src/opl/nukedopl3.c \
    src/piano.cpp

HEADERS += \
    src/bank_editor.h \
    src/bank.h \
    src/common.h \
    src/FileFormats/ffmt_base.h \
    src/FileFormats/ffmt_enums.h \
    src/FileFormats/ffmt_factory.h \
    src/FileFormats/format_adlib_bnk.h \
    src/FileFormats/format_adlib_tim.h \
    src/FileFormats/format_ail2_gtl.h \
    src/FileFormats/format_apogeetmb.h \
    src/FileFormats/format_bisqwit.h \
    src/FileFormats/format_cmf_importer.h \
    src/FileFormats/format_dmxopl2.h \
    src/FileFormats/format_imf_importer.h \
    src/FileFormats/format_junlevizion.h \
    src/FileFormats/format_rad_importer.h \
    src/FileFormats/format_sb_ibk.h \
    src/FileFormats/format_wohlstand_opl3.h \
    src/importer.h \
    src/ins_names.h \
    src/opl/generator.h \
    src/opl/nukedopl3.h \
    src/piano.h \
    src/version.h

FORMS += \
    src/bank_editor.ui \
    src/importer.ui

RESOURCES += \
    src/resources/resources.qrc
