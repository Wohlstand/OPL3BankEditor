cmake_minimum_required(VERSION 3.2)
project(OPL3BankEditor LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)

if(CMAKE_C_COMPILER_ID MATCHES "^(GNU|Clang)$")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
endif()
if(CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang)$")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
endif()

set(USE_RTMIDI "ON" CACHE STRING "Use RtMidi")
set(USE_RTAUDIO "ON" CACHE STRING "Use RtAudio")

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)
find_package(Threads REQUIRED)

message("!! Optional feature summary:")
message("!!   RtMidi: ${USE_RTMIDI}")
message("!!   RtAudio: ${USE_RTAUDIO}")

include_directories("src")

set(SOURCES
  "src/audio.cpp"
  "src/bank.cpp"
  "src/bank_editor.cpp"
  "src/common.cpp"
  "src/controlls.cpp"
  "src/FileFormats/ffmt_base.cpp"
  "src/FileFormats/ffmt_factory.cpp"
  "src/FileFormats/format_adlib_bnk.cpp"
  "src/FileFormats/format_adlib_tim.cpp"
  "src/FileFormats/format_ail2_gtl.cpp"
  "src/FileFormats/format_apogeetmb.cpp"
  "src/FileFormats/format_bisqwit.cpp"
  "src/FileFormats/format_cmf_importer.cpp"
  "src/FileFormats/format_dmxopl2.cpp"
  "src/FileFormats/format_imf_importer.cpp"
  "src/FileFormats/format_junlevizion.cpp"
  "src/FileFormats/format_rad_importer.cpp"
  "src/FileFormats/format_sb_ibk.cpp"
  "src/FileFormats/format_wohlstand_opl3.cpp"
  "src/FileFormats/format_flatbuffer_opl3.cpp"
  "src/formats_sup.cpp"
  "src/importer.cpp"
  "src/latency.cpp"
  "src/ins_names.cpp"
  "src/main.cpp"
  "src/opl/generator.cpp"
  "src/opl/generator_realtime.cpp"
  "src/opl/realtime/ring_buffer.cpp"
  "src/piano.cpp"
  "src/opl/measurer.cpp"
  "src/opl/chips/dosbox_opl3.cpp"
  "src/opl/chips/nuked_opl3.cpp"
  "src/opl/chips/opl_chip_base.cpp"
  "src/opl/chips/nuked/nukedopl3.c"
  "src/opl/chips/dosbox/dbopl.cpp"
  "src/FileFormats/wopl/wopl_file.c"
  "src/opl/chips/nuked_opl3_v174.cpp"
  "src/opl/chips/nuked/nukedopl3_174.c")

qt5_wrap_ui(FORMS
  "src/bank_editor.ui"
  "src/formats_sup.ui"
  "src/importer.ui")

qt5_add_resources(RESOURCES
  "src/resources/resources.qrc")

add_executable(OPL3BankEditor WIN32 MACOSX_BUNDLE ${SOURCES} ${FORMS} ${RESOURCES})
target_link_libraries(OPL3BankEditor PRIVATE Qt5::Widgets Qt5::Concurrent ${CMAKE_THREAD_LIBS_INIT})

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/res.rc")
  target_sources(OPL3BankEditor PRIVATE "${ICON}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(MACOSX_BUNDLE_ICON_FILE "opl3.icns")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/opl3.icns")
  set_source_files_properties("${ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(OPL3BankEditor PRIVATE "${ICON}")
endif()

if(USE_RTMIDI)
  add_library(RtMidi STATIC "src/midi/external/rtmidi/RtMidi.cpp")
  target_include_directories(RtMidi PUBLIC "src/midi/external/rtmidi")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(RtMidi PUBLIC "__LINUX_ALSA__")
    target_link_libraries(RtMidi PUBLIC "asound")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(RtMidi PUBLIC "__WINDOWS_MM__")
    target_link_libraries(RtMidi PUBLIC "winmm")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(RtMidi PUBLIC "__MACOSX_CORE__")
    find_library(COREMIDI_LIBRARY "CoreMidi")
    target_link_libraries(RtMidi PUBLIC "${COREMIDI_LIBRARY}")
  endif()
  target_sources(OPL3BankEditor PRIVATE "src/midi/midi_rtmidi.cpp")
  target_compile_definitions(OPL3BankEditor PRIVATE "ENABLE_MIDI")
  target_link_libraries(OPL3BankEditor PRIVATE RtMidi)
endif()

if(USE_RTAUDIO)
  add_library(RtAudio STATIC "src/audio/external/rtaudio/RtAudio.cpp")
  target_include_directories(RtAudio PUBLIC "src/audio/external/rtaudio")
  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_compile_definitions(RtAudio PUBLIC "__LINUX_ALSA__")
    target_link_libraries(RtAudio PUBLIC "asound")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_compile_definitions(RtAudio PUBLIC "__WINDOWS_DS__")
    target_link_libraries(RtAudio PUBLIC "dsound" "ole32")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(RtAudio PUBLIC "__MACOSX_CORE__")
    find_library(COREAUDIO_LIBRARY "CoreAudio")
    target_link_libraries(RtAudio PUBLIC "${COREAUDIO_LIBRARY}")
  endif()
  target_sources(OPL3BankEditor PRIVATE "src/audio/ao_rtaudio.cpp")
  target_compile_definitions(OPL3BankEditor PRIVATE "ENABLE_AUDIO_TESTING")
  target_link_libraries(OPL3BankEditor PRIVATE RtAudio)
endif()
