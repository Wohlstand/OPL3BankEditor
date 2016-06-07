#-------------------------------------------------
#
# Project created by QtCreator 2016-06-07T13:26:28
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FMBankEdit
TEMPLATE = app


SOURCES += main.cpp\
        bank_editor.cpp \
    ins_names.cpp \
    bank.cpp \
    FileFormats/junlevizion.cpp

HEADERS  += bank_editor.h \
    ins_names.h \
    bank.h \
    FileFormats/junlevizion.h \
    version.h

FORMS    += bank_editor.ui
