#-------------------------------------------------
#
# Project created by QtCreator 2016-06-07T13:26:28
#
#-------------------------------------------------

QT       += core gui widgets multimedia

TARGET = FMBankEdit
TEMPLATE = app

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

win32: RC_FILE = $$PWD/resources/res.rc

SOURCES += main.cpp\
        bank_editor.cpp \
    ins_names.cpp \
    bank.cpp \
    FileFormats/junlevizion.cpp \
    opl/dbopl.cpp \
    opl/generator.cpp \
    piano.cpp \
    common.cpp \
    controlls.cpp \
    audio.cpp \
    FileFormats/dmxopl2.cpp

HEADERS  += bank_editor.h \
    ins_names.h \
    bank.h \
    FileFormats/junlevizion.h \
    version.h \
    opl/dbopl.h \
    opl/generator.h \
    piano.h \
    common.h \
    FileFormats/dmxopl2.h

FORMS    += bank_editor.ui

RESOURCES += \
    resources/resources.qrc
