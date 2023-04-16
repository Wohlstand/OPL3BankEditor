set(CHIPS_SOURCES
    "src/opl/chips/dosbox_opl3.cpp"
    "src/opl/chips/java_opl3.cpp"
    "src/opl/chips/nuked_opl3.cpp"
    "src/opl/chips/opal_opl3.cpp"
    "src/opl/chips/nuked/nukedopl3.c"
    "src/opl/chips/dosbox/dbopl.cpp"
    "src/opl/chips/nuked_opl3_v174.cpp"
    "src/opl/chips/nuked/nukedopl3_174.c")

if(COMPILER_SUPPORTS_CXX14) # YMFM can be built in only condition when C++14 and newer were available
  set(YMFM_SOURCES
      "src/opl/chips/ymfm_opl3.cpp"
      "src/opl/chips/ymfm/ymfm_opl.cpp"
      "src/opl/chips/ymfm/ymfm_misc.cpp"
      "src/opl/chips/ymfm/ymfm_pcm.cpp"
      "src/opl/chips/ymfm/ymfm_adpcm.cpp"
      "src/opl/chips/ymfm/ymfm_ssg.cpp"
  )
  if(DEFINED FLAG_CPP14)
    set_source_files_properties(${YMFM_SOURCES} COMPILE_FLAGS ${FLAG_CPP14})
  endif()
  list(APPEND CHIPS_SOURCES ${YMFM_SOURCES})
endif()

if(ENABLE_OPL3_PROXY)
  list(APPEND CHIPS_SOURCES "src/opl/chips/win9x_opl_proxy.cpp")
endif()

if(ENABLE_SERIAL_PORT)
  list(APPEND CHIPS_SOURCES "src/opl/chips/opl_serial_port.cpp")
endif()
