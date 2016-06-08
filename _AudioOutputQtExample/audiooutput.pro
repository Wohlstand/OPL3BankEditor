TEMPLATE = app
TARGET = audiooutput

QT += multimedia widgets

CONFIG += c++11

HEADERS       = audiooutput.h \
    dbopl.h

SOURCES       = audiooutput.cpp \
                main.cpp \
    dbopl.cpp

target.path = $$[QT_INSTALL_EXAMPLES]/multimedia/audiooutput
INSTALLS += target
