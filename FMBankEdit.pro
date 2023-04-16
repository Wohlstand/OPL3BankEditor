#----------------------------------------------------------------------------
# OPL Bank Editor by Wohlstand, a free tool for music bank editing
# Copyright (c) 2016-2023 Vitaly Novichkov <admin@wohlnet.ru>
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
    load(configure)
    if(qtCompileTest(cpp14)) {
        CONFIG += c++14
        CONFIG += enable_ymfm
        message("TEST: С++14 support presented, YMFM will be ENABLED!")
    } else {
        CONFIG += c++11
        message("TEST: С++14 was not found, YMFM will be DISABLED!")
    }
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
LIBS += -lz

!macx:{
    QMAKE_CXXFLAGS += -fopenmp
}

TEMPLATE = app
TARGET = opl3_bank_editor
INCLUDEPATH += $$PWD/src

# Better name look for an app bundle
macx: TARGET = "OPL3 Bank Editor"

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

win32||oplproxy: {
    include("src/opl/chips/win9x_opl_proxy.pri")
    DEFINES += ENABLE_HW_OPL_PROXY
}
greaterThan(QT_MAJOR_VERSION, 4):lessThan(QT_MAJOR_VERSION, 6) {
    include("src/opl/chips/opl_serial_port.pri")
    DEFINES += ENABLE_HW_OPL_SERIAL_PORT
}

win32 {
    lessThan(QT_MAJOR_VERSION, 4):{
        DEFINES += ENABLE_WIN9X_OPL_PROXY
    }
}

include(src/opl/chips/chipset.pri)

SOURCES += \
    src/FileFormats/format_smaf_importer.cpp \
    src/audio.cpp \
    src/bank.cpp \
    src/bank_editor.cpp \
    src/operator_editor.cpp \
    src/bank_comparison.cpp \
    src/common.cpp \
    src/controlls.cpp \
    src/proxystyle.cpp \
    src/FileFormats/ffmt_base.cpp \
    src/FileFormats/ffmt_enums.cpp \
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
    src/FileFormats/format_dro_importer.cpp \
    src/FileFormats/format_vgm_import.cpp \
    src/FileFormats/format_misc_sgi.cpp \
    src/FileFormats/format_misc_cif.cpp \
    src/FileFormats/format_misc_hsc.cpp \
    src/FileFormats/format_wohlstand_opl3.cpp \
    src/FileFormats/format_flatbuffer_opl3.cpp \
    src/FileFormats/ymf262_to_wopi.cpp \
    src/formats_sup.cpp \
    src/importer.cpp \
    src/audio_config.cpp \
    src/hardware.cpp \
    src/ins_names.cpp \
    src/main.cpp \
    src/opl/generator.cpp \
    src/opl/generator_realtime.cpp \
    src/opl/realtime/ring_buffer.cpp \
    src/piano.cpp \
    src/opl/measurer.cpp \
    src/FileFormats/wopl/wopl_file.c

HEADERS += \
    src/FileFormats/format_smaf_importer.h \
    src/bank_editor.h \
    src/operator_editor.h \
    src/bank_comparison.h \
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
    src/FileFormats/format_dro_importer.h \
    src/FileFormats/format_vgm_import.h \
    src/FileFormats/format_misc_sgi.h \
    src/FileFormats/format_misc_cif.h \
    src/FileFormats/format_misc_hsc.h \
    src/FileFormats/format_wohlstand_opl3.h \
    src/FileFormats/format_flatbuffer_opl3.h \
    src/FileFormats/ymf262_to_wopi.h \
    src/formats_sup.h \
    src/importer.h \
    src/audio_config.h \
    src/hardware.h \
    src/ins_names.h \
    src/ins_names_data.h \
    src/main.h \
    src/opl/generator.h \
    src/opl/generator_realtime.h \
    src/opl/nukedopl3.h \
    src/opl/realtime/ring_buffer.h \
    src/opl/realtime/ring_buffer.tcc \
    src/piano.h \
    src/version.h \
    src/opl/measurer.h \
    src/FileFormats/wopl/wopl_file.h

FORMS += \
    src/bank_editor.ui \
    src/operator_editor.ui \
    src/bank_comparison.ui \
    src/formats_sup.ui \
    src/importer.ui \
    src/audio_config.ui \
    src/hardware.ui

RESOURCES += \
    src/resources/resources.qrc

TRANSLATIONS += \
    src/translations/opl3bankeditor_fr_FR.ts \
    src/translations/opl3bankeditor_ru_RU.ts \
    src/translations/opl3bankeditor_pl_PL.ts

plots {
    SOURCES += src/delay_analysis.cpp
    HEADERS += src/delay_analysis.h
    FORMS += src/delay_analysis.ui
    CONFIG += qwt
    DEFINES += ENABLE_PLOTS
}
