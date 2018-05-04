SOURCES += \
    $$PWD/midi_rtmidi.cpp \
    $$PWD/external/rtmidi/RtMidi.cpp

HEADERS += \
    $$PWD/midi_rtmidi.h \
    $$PWD/external/rtmidi/RtMidi.h

INCLUDEPATH += $$PWD/external/rtmidi

linux {
    DEFINES += __LINUX_ALSA__
    LIBS += -lasound
}
win32 {
    DEFINES += __WINDOWS_MM__
    LIBS += -lwinmm
}
macx {
    DEFINES += __MACOSX_CORE__
    LIBS += -framework CoreMidi
}
