SOURCES+= \
    $$PWD/dosbox_opl3.cpp \
    $$PWD/java_opl3.cpp \
    $$PWD/nuked_opl3.cpp \
    $$PWD/opal_opl3.cpp \
    $$PWD/nuked/nukedopl3.c \
    $$PWD/dosbox/dbopl.cpp \
    $$PWD/nuked_opl3_v174.cpp \
    $$PWD/nuked/nukedopl3_174.c

HEADERS+= \
    $$PWD/opl_chip_base.h \
    $$PWD/opl_chip_base.tcc \
    $$PWD/dosbox_opl3.h \
    $$PWD/java_opl3.h \
    $$PWD/nuked_opl3.h \
    $$PWD/opal_opl3.h \
    $$PWD/opl_chip_base.h \
    $$PWD/java/JavaOPL3.hpp \
    $$PWD/nuked/nukedopl3.h \
    $$PWD/opal/opal.hpp \
    $$PWD/dosbox/dbopl.h \
    $$PWD/nuked_opl3_v174.h \
    $$PWD/nuked/nukedopl3_174.h

# Available when C++14 is supported
enable_ymfm: {
DEFINES += ENABLE_YMFM_EMULATOR

SOURCES+= \
    $$PWD/ymfm_opl3.cpp \
    $$PWD/ymfm/ymfm_adpcm.cpp \
    $$PWD/ymfm/ymfm_misc.cpp \
    $$PWD/ymfm/ymfm_opl.cpp \
    $$PWD/ymfm/ymfm_pcm.cpp \
    $$PWD/ymfm/ymfm_ssg.cpp

HEADERS+= \
    $$PWD/ymfm_opl3.h \
    $$PWD/ymfm/ymfm.h \
    $$PWD/ymfm/ymfm_adpcm.h \
    $$PWD/ymfm/ymfm_fm.h \
    $$PWD/ymfm/ymfm_fm.ipp \
    $$PWD/ymfm/ymfm_misc.h \
    $$PWD/ymfm/ymfm_opl.h \
    $$PWD/ymfm/ymfm_pcm.h \
    $$PWD/ymfm/ymfm_ssg.h
}
