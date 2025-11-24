set(BUILD_MAJOR_QT 5 CACHE STRING "Major version of Qt to prefer (Supported 4, 5, and 6)")

set(FIND_QT_MAJOR ${BUILD_MAJOR_QT})

if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
    set(OpenGL_GL_PREFERENCE GLVND)
endif()

if(BUILD_MAJOR_QT GREATER_EQUAL 6)
    set(FIND_QT_MAJOR 6)
    find_package(Qt6 COMPONENTS Core QUIET) # Check is Qt available
    set(QT_ANY_DIR ${Qt6_DIR})
endif()

if(BUILD_MAJOR_QT GREATER_EQUAL 5 AND NOT Qt6_FOUND)
    set(FIND_QT_MAJOR 5)
    find_package(Qt5 COMPONENTS Core QUIET) # Check is Qt available
    set(QT_ANY_DIR ${Qt5_DIR})
endif()

if(BUILD_MAJOR_QT GREATER_EQUAL 4 AND NOT (Qt5_FOUND OR Qt6_FOUND))
    set(FIND_QT_MAJOR 4)
    find_package(Qt4 COMPONENTS Core QUIET) # Check is Qt available
    set(QT_ANY_DIR ${Qt4_DIR})
endif()

if(NOT Qt6_FOUND AND NOT Qt5_FOUND AND NOT Qt4_FOUND)
    message(FATAL_ERROR "No Qt 4, 5, or 6 was found! This project reques Qt to build!")
endif()

message("Found Qt ${FIND_QT_MAJOR}!")

if(FIND_QT_MAJOR EQUAL 6)
    set(QWT_NAMES qwt-qt6) # find versions for Qt6 only
elseif(FIND_QT_MAJOR EQUAL 5)
    set(QWT_NAMES qwt-qt5 qwt) # find versions for Qt5 only
else()
    set(QWT_NAMES qwt-qt4 qwt) # find versions for Qt4 only
endif()


message("== Qt has been found in ${QT_ANY_DIR}")
if(FIND_QT_MAJOR EQUAL 6)
    get_filename_component(QT6_BINARY_DIRS "${QT_ANY_DIR}/../../bin" ABSOLUTE)
    if(UNIX AND NOT APPLE)
        list(APPEND QT6_BINARY_DIRS "/usr/lib/qt6/bin")
    endif()
elseif(FIND_QT_MAJOR EQUAL 5)
    get_filename_component(QT5_BINARY_DIRS "${QT_ANY_DIR}/../../bin" ABSOLUTE)
    if(UNIX AND NOT APPLE)
        list(APPEND QT5_BINARY_DIRS "/usr/lib/qt5/bin")
    endif()
elseif(FIND_QT_MAJOR EQUAL 4)
    get_filename_component(QT4_BINARY_DIRS "${QT_ANY_DIR}/../../bin" ABSOLUTE)
    if(UNIX AND NOT APPLE)
        list(APPEND QT4_BINARY_DIRS "/usr/lib/qt4/bin")
    endif()
endif()


# Tools

if(Qt6_FOUND)
    find_program(_QT_LRELEASE_PROGRAM NAMES lrelease lrelease-qt6 PATHS ${QT6_BINARY_DIRS})
elseif(Qt5_FOUND)
    find_program(_QT_LRELEASE_PROGRAM NAMES lrelease lrelease-qt5 PATHS ${QT6_BINARY_DIRS})
else()
    find_program(_QT_LRELEASE_PROGRAM NAMES lrelease lrelease-qt4 PATHS ${QT5_BINARY_DIRS})
endif()

if(_QT_LRELEASE_PROGRAM)
    message(STATUS "Found ${_QT_LRELEASE_PROGRAM}, locales will be compiled!")
else()
    message(WARNING "Unable to find lrelease, locales will NOT be built!")
endif()


if(Qt6_FOUND)
    find_program(_QT_LUPDATE_PROGRAM NAMES lupdate lupdate-qt6 PATHS ${QT6_BINARY_DIRS})
else()
    find_program(_QT_LUPDATE_PROGRAM NAMES lupdate lupdate-qt5 PATHS ${QT5_BINARY_DIRS})
endif()

if(_QT_LUPDATE_PROGRAM)
    message(STATUS "Found ${_QT_LUPDATE_PROGRAM}, locales can be refreshed!")
else()
    message(WARNING "Unable to find lupdate, locales can't be refreshed!")
endif()

# Libraries
macro(pge_qt_find_and_define_lib QT_COMPONENT_NAME)
    if(FIND_QT_MAJOR EQUAL 6)
        set(XXX_QT_VER Qt6)
    elseif(FIND_QT_MAJOR EQUAL 5)
        set(XXX_QT_VER Qt5)
    elseif(FIND_QT_MAJOR EQUAL 4)
        set(XXX_QT_VER Qt4)
    endif()

    set(extra_args ${ARGN})
    list(LENGTH extra_args extra_count)
    if(${extra_count} GREATER 0)
        list(GET extra_args 0 required_arg)
        set(QT_REQUIRED_OPTION ${required_arg})
    endif()

    message("-- Finding Qt component: ${XXX_QT_VER}${QT_COMPONENT_NAME} ${QT_REQUIRED_OPTION}")

    find_package(${XXX_QT_VER}${QT_COMPONENT_NAME} ${QT_REQUIRED_OPTION})
    set(QT_${QT_COMPONENT_NAME}_TARGET ${XXX_QT_VER}::${QT_COMPONENT_NAME})
    set(QT_${QT_COMPONENT_NAME}_LIBRARIES ${${XXX_QT_VER}${QT_COMPONENT_NAME}_LIBRARIES})
    set(QT_${QT_COMPONENT_NAME}_DEFINITIONS ${${XXX_QT_VER}${QT_COMPONENT_NAME}_DEFINITIONS})
    set(QT_${QT_COMPONENT_NAME}_INCLUDE_DIRS ${${XXX_QT_VER}${QT_COMPONENT_NAME}_INCLUDE_DIRS})
    set(QT_${QT_COMPONENT_NAME}_EXECUTABLE_COMPILE_FLAGS ${${XXX_QT_VER}${QT_COMPONENT_NAME}_EXECUTABLE_COMPILE_FLAGS})
    unset(XXX_QT_VER)
    unset(QT_REQUIRED_OPTION)
    unset(required_arg)
    unset(extra_args)
endmacro()

pge_qt_find_and_define_lib(Core REQUIRED)
pge_qt_find_and_define_lib(Gui REQUIRED)
if(BUILD_MAJOR_QT GREATER_EQUAL 5)
    pge_qt_find_and_define_lib(Widgets REQUIRED)
endif()
pge_qt_find_and_define_lib(Concurrent REQUIRED)
pge_qt_find_and_define_lib(LinguistTools REQUIRED)
pge_qt_find_and_define_lib(SerialPort)
if(BUILD_MAJOR_QT GREATER_EQUAL 5 AND WIN32)
    pge_qt_find_and_define_lib(WinExtras REQUIRED)
endif()


macro(pge_qt_wrap_ui)
    if(FIND_QT_MAJOR EQUAL 6)
        qt6_wrap_ui(${ARGN})
    elseif(FIND_QT_MAJOR EQUAL 5)
        qt5_wrap_ui(${ARGN})
    elseif(FIND_QT_MAJOR EQUAL 4)
        qt4_wrap_ui(${ARGN})
    endif()
endmacro()

macro(pge_qt_add_resources)
    if(FIND_QT_MAJOR EQUAL 6)
        qt6_add_resources(${ARGN})
    elseif(FIND_QT_MAJOR EQUAL 5)
        qt5_add_resources(${ARGN})
    elseif(FIND_QT_MAJOR EQUAL 4)
        qt4_add_resources(${ARGN})
    endif()
endmacro()

macro(pge_qt_add_translation)
    if(FIND_QT_MAJOR EQUAL 6)
        qt6_add_translation(${ARGN})
    elseif(FIND_QT_MAJOR EQUAL 5)
        qt5_add_translation(${ARGN})
    elseif(FIND_QT_MAJOR EQUAL 4)
        qt4_add_translation(${ARGN})
    endif()
endmacro()
