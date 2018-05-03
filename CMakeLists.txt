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

set(USE_ALSA "" CACHE STRING "Use ALSA")
set(USE_QTMM "" CACHE STRING "Use QtMultimedia")

if(USE_ALSA STREQUAL "")
  find_package(ALSA)
  set(USE_ALSA "${ALSA_FOUND}" CACHE STRING "Use ALSA" FORCE)
endif()
if(USE_QTMM STREQUAL "")
  find_package(Qt5Multimedia)
  set(USE_QTMM "${Qt5Multimedia_FOUND}" CACHE STRING "Use QtMultimedia" FORCE)
endif()

if(USE_ALSA)
  find_package(ALSA REQUIRED)
  find_package(Threads REQUIRED)
endif()
if(USE_QTMM)
  find_package(Qt5Multimedia REQUIRED)
endif()

find_package(Qt5Widgets REQUIRED)
find_package(Qt5Concurrent REQUIRED)

message("!! Optional feature summary:")
message("!!   ALSA: ${USE_ALSA}")
message("!!   QtMultimedia: ${USE_QTMM}")

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
  "src/ins_names.cpp"
  "src/main.cpp"
  "src/opl/generator.cpp"
  "src/piano.cpp"
  "src/opl/measurer.cpp"
  "src/audio/ao_base.cpp"
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
target_link_libraries(OPL3BankEditor PRIVATE Qt5::Widgets Qt5::Concurrent)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/res.rc")
  target_sources(OPL3BankEditor PRIVATE "${ICON}")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(MACOSX_BUNDLE_ICON_FILE "opl3.icns")
  set(ICON "${CMAKE_SOURCE_DIR}/src/resources/opl3.icns")
  set_source_files_properties("${ICON}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
  target_sources(OPL3BankEditor PRIVATE "${ICON}")
endif()

if(USE_ALSA)
  target_sources(OPL3BankEditor PRIVATE "${CMAKE_SOURCE_DIR}/src/audio/ao_alsa.cpp")
  target_compile_definitions(OPL3BankEditor PRIVATE "ENABLE_AUDIO_TESTING" "USE_AUDIO_ALSA")
  target_include_directories(OPL3BankEditor PRIVATE ${ALSA_INCLUDE_DIRS})
  target_link_libraries(OPL3BankEditor PRIVATE ${ALSA_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})
endif()

if(USE_QTMM)
  target_sources(OPL3BankEditor PRIVATE "${CMAKE_SOURCE_DIR}/src/audio/ao_qtmm.cpp")
  target_compile_definitions(OPL3BankEditor PRIVATE "ENABLE_AUDIO_TESTING" "USE_AUDIO_QTMM")
  target_link_libraries(OPL3BankEditor PRIVATE Qt5::Multimedia)
endif()
