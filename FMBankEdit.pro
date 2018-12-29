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
    QT += widgets concurrent
    DEFINES += ENABLE_AUDIO_TESTING
    CONFIG += c++11
} else {
    QMAKE_CXXFLAGS += -std=c++11
    DEFINES += IS_QT_4
    win32: {
        CONFIG += static
        QMAKE_LFLAGS += -static-libgcc -static-libstdc++ -static
        #DEFINES += snprintf=_snprintf
        DEFINES += NO_NATIVE_OPEN_DIALOGS
        INCLUDEPATH += $$PWD/src/audio/for-mingw-9x
    }
}
win32 {
    DEFINES += _USE_MATH_DEFINES
}

CONFIG += rtmidi
CONFIG += rtaudio
#CONFIG += plots

!macx:{
QMAKE_CXXFLAGS += -fopenmp
}

TEMPLATE = app
TARGET = opl3_bank_editor
INCLUDEPATH += $$PWD/
INCLUDEPATH += $$PWD/src

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

rtaudio {
    include("src/audio/ao_rtaudio.pri")
}
rtmidi {
    DEFINES += ENABLE_MIDI
    include("src/midi/midi_rtmidi.pri")
}
win32 {
    include("src/opl/chips/win9x_opl_proxy.pri")
    DEFINES += ENABLE_WIN9X_OPL_PROXY
}

SOURCES += \
    src/audio.cpp \
    src/bank.cpp \
    src/bank_editor.cpp \
    src/common.cpp \
    src/controlls.cpp \
    src/proxystyle.cpp \
    src/FileFormats/ffmt_base.cpp \
    src/FileFormats/ffmt_factory.cpp \
    src/FileFormats/format_adlib_bnk.cpp \
    src/FileFormats/format_adlib_tim.cpp \
    src/FileFormats/format_adlibgold_bnk2.cpp \
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
    src/FileFormats/format_flatbuffer_opl3.cpp \
    src/formats_sup.cpp \
    src/importer.cpp \
    src/latency.cpp \
    src/hardware.cpp \
    src/ins_names.cpp \
    src/main.cpp \
    src/opl/generator.cpp \
    src/opl/generator_realtime.cpp \
    src/opl/realtime/ring_buffer.cpp \
    src/piano.cpp \
    src/opl/measurer.cpp \
    src/opl/chips/dosbox_opl3.cpp \
    src/opl/chips/nuked_opl3.cpp \
    src/opl/chips/nuked/nukedopl3.c \
    src/opl/chips/dosbox/dbopl.cpp \
    src/FileFormats/wopl/wopl_file.c \
    src/opl/chips/nuked_opl3_v174.cpp \
    src/opl/chips/nuked/nukedopl3_174.c

HEADERS += \
    src/bank_editor.h \
    src/bank.h \
    src/common.h \
    src/proxystyle.h \
    src/FileFormats/ffmt_base.h \
    src/FileFormats/ffmt_enums.h \
    src/FileFormats/ffmt_factory.h \
    src/FileFormats/format_adlib_bnk.h \
    src/FileFormats/format_adlib_tim.h \
    src/FileFormats/format_adlibgold_bnk2.h \
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
    src/FileFormats/format_flatbuffer_opl3.h \
    src/formats_sup.h \
    src/importer.h \
    src/latency.h \
    src/hardware.h \
    src/ins_names.h \
    src/main.h \
    src/opl/generator.h \
    src/opl/generator_realtime.h \
    src/opl/nukedopl3.h \
    src/opl/realtime/ring_buffer.h \
    src/opl/realtime/ring_buffer.tcc \
    src/piano.h \
    src/version.h \
    src/opl/measurer.h \
    src/opl/chips/opl_chip_base.h \
    src/opl/chips/opl_chip_base.tcc \
    src/opl/chips/dosbox_opl3.h \
    src/opl/chips/nuked_opl3.h \
    src/opl/chips/opl_chip_base.h \
    src/opl/chips/nuked/nukedopl3.h \
    src/opl/chips/dosbox/dbopl.h \
    src/FileFormats/wopl/wopl_file.h \
    src/opl/chips/nuked_opl3_v174.h \
    src/opl/chips/nuked/nukedopl3_174.h

FORMS += \
    src/bank_editor.ui \
    src/formats_sup.ui \
    src/importer.ui

RESOURCES += \
    src/resources/resources.qrc

TRANSLATIONS += \
    src/translations/opl3bankeditor_fr_FR.ts \
    src/translations/opl3bankeditor_ru_RU.ts

plots {
    SOURCES += src/delay_analysis.cpp
    HEADERS += src/delay_analysis.h
    FORMS += src/delay_analysis.ui
    CONFIG += qwt
    DEFINES += ENABLE_PLOTS
}
