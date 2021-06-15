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
    DEFINES += __LINUX_PULSE__
    LIBS += -lpulse-simple -lpulse
}
win32 {
    DEFINES += __WINDOWS_DS__
    LIBS += -ldsound -lole32

    greaterThan(QT_MAJOR_VERSION, 4): !DISABLE_WASAPI:{
        DEFINES += __WINDOWS_WASAPI__
        LIBS += -lole32 -lksguid
        LIBS += -lmfplat -lmfuuid -lwmcodecdspuuid # TODO: Verify will this break Windows XP compatibility or not
        # DEFINES += __WINDOWS_ASIO__
        # INCLUDEPATH += $$PWD/external/rtaudio/include
        # SOURCES += \
        #   $$PWD/external/rtaudio/include/asio.cpp \
        #   $$PWD/external/rtaudio/include/asiodrivers.cpp \
        #   $$PWD/external/rtaudio/include/asiolist.cpp \
        #   $$PWD/external/rtaudio/include/iasiothiscallresolver.cpp
    }
}
macx {
    DEFINES += __MACOSX_CORE__
    LIBS += -framework CoreAudio -framework CoreFoundation
}
