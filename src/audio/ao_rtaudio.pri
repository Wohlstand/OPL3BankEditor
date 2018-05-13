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
    greaterThan(QT_MAJOR_VERSION, 4):{
        ## Don't turn on WASAPI to support Windows XP!
        ## Alternatively, to support WASAPI, need to try load it through LoadLibrary way
        ## When we using Qt 5.7 or lower, then disable WASAPI to support Windows XP runtime
        isEqual(QT_MAJOR_VERSION, 5):greaterThan(QT_MINOR_VERSION, 7):{
            CONFIG+=Qt_5_7_PLUS
        }
        greaterThan(QT_MAJOR_VERSION, 5):{
            CONFIG+=Qt_5_7_PLUS
        }
        Qt_5_7_PLUS {
            message("RtAudio: WASAPI WILL BE USED")
            DEFINES += __WINDOWS_WASAPI__
            LIBS += -lksguid
        } else {
            message("RtAudio: WASAPI support is disabled!")
        }
        DEFINES += __WINDOWS_ASIO__
        INCLUDEPATH += $$PWD/external/rtaudio/include
        SOURCES += \
          $$PWD/external/rtaudio/include/asio.cpp \
          $$PWD/external/rtaudio/include/asiodrivers.cpp \
          $$PWD/external/rtaudio/include/asiolist.cpp \
          $$PWD/external/rtaudio/include/iasiothiscallresolver.cpp
    }
}
macx {
    DEFINES += __MACOSX_CORE__
    LIBS += -framework CoreAudio
}
