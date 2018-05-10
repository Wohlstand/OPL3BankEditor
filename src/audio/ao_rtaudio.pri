SOURCES += \
    $$PWD/ao_rtaudio.cpp \
    $$PWD/external/rtaudio/RtAudio.cpp

HEADERS += \
    $$PWD/ao_rtaudio.h \
    $$PWD/external/rtaudio/RtAudio.h

INCLUDEPATH += $$PWD/external/rtaudio

linux {
    DEFINES += __LINUX_ALSA__
    LIBS += -lasound
}
win32 {
    DEFINES += __WINDOWS_DS__
    LIBS += -ldsound -lole32
}
macx {
    DEFINES += __MACOSX_CORE__
    LIBS += -framework CoreAudio
}
