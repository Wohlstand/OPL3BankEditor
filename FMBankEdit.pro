#-------------------------------------------------
#
# Project created by QtCreator 2016-06-07T13:26:28
#
#-------------------------------------------------

QT       += core gui widgets multimedia

TARGET = FMBankEdit
TEMPLATE = app

SOURCES += main.cpp\
        bank_editor.cpp \
    ins_names.cpp \
    bank.cpp \
    FileFormats/junlevizion.cpp \
    opl/adldata.cpp \
    opl/adlmidi.cpp \
    opl/dbopl.cpp

HEADERS  += bank_editor.h \
    ins_names.h \
    bank.h \
    FileFormats/junlevizion.h \
    version.h \
    opl/adldata.hh \
    opl/adlmidi.h \
    opl/dbopl.h \
    opl/fraction.h

FORMS    += bank_editor.ui
