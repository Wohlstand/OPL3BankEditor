set(CHIPS_SOURCES
    "src/opl/chips/dosbox_opl3.cpp"
    "src/opl/chips/dosbox_opl3.h"
    "src/opl/chips/java_opl3.cpp"
    "src/opl/chips/java_opl3.h"
    "src/opl/chips/nuked_opl3.cpp"
    "src/opl/chips/nuked_opl3.h"
    "src/opl/chips/opal_opl3.cpp"
    "src/opl/chips/opal_opl3.h"
    "src/opl/chips/opal/opal.c"
    "src/opl/chips/opal/opal.h"
    "src/opl/chips/nuked/nukedopl3.c"
    "src/opl/chips/nuked/nukedopl3.h"
    "src/opl/chips/dosbox/dbopl.cpp"
    "src/opl/chips/dosbox/dbopl.h"
    "src/opl/chips/nuked_opl3_v174.cpp"
    "src/opl/chips/nuked_opl3_v174.h"
    "src/opl/chips/nuked/nukedopl3_174.c"
    "src/opl/chips/nuked/nukedopl3_174.h"
    "src/opl/chips/ymf262_lle.cpp"
    "src/opl/chips/ymf262_lle.h"
    "src/opl/chips/ymf262_lle/nuked_fmopl3.c"
    "src/opl/chips/ymf262_lle/nuked_fmopl3.h"
    "src/opl/chips/ymf262_lle/nopl3.c"
    "src/opl/chips/ymf262_lle/nopl3.h"
)

if(COMPILER_SUPPORTS_CXX14) # YMFM can be built in only condition when C++14 and newer were available
  set(YMFM_SOURCES
      "src/opl/chips/ymfm_opl3.cpp"
      "src/opl/chips/ymfm_opl3.h"
      "src/opl/chips/ymfm/ymfm.h"
      "src/opl/chips/ymfm/ymfm_opl.cpp"
      "src/opl/chips/ymfm/ymfm_opl.h"
      "src/opl/chips/ymfm/ymfm_misc.cpp"
      "src/opl/chips/ymfm/ymfm_misc.h"
      "src/opl/chips/ymfm/ymfm_pcm.cpp"
      "src/opl/chips/ymfm/ymfm_pcm.h"
      "src/opl/chips/ymfm/ymfm_adpcm.cpp"
      "src/opl/chips/ymfm/ymfm_adpcm.h"
      "src/opl/chips/ymfm/ymfm_ssg.cpp"
      "src/opl/chips/ymfm/ymfm_ssg.h"
  )
  if(DEFINED FLAG_CPP14)
    set_source_files_properties(${YMFM_SOURCES} COMPILE_FLAGS ${FLAG_CPP14})
  endif()
  list(APPEND CHIPS_SOURCES ${YMFM_SOURCES})
endif()

if(ENABLE_OPL3_PROXY)
  list(APPEND CHIPS_SOURCES
    "src/opl/chips/win9x_opl_proxy.cpp"
    "src/opl/chips/win9x_opl_proxy.h"
  )
endif()

if(ENABLE_SERIAL_PORT)
  list(APPEND CHIPS_SOURCES
    "src/opl/chips/opl_serial_port.cpp"
    "src/opl/chips/opl_serial_port.h"
  )
endif()
