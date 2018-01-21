LIBS += -lasound

DEFINES += USE_AUDIO_ALSA

HEADERS += \
    $$PWD/ao_alsa.h

SOURCES += \
    $$PWD/ao_alsa.cpp

