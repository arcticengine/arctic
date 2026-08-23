
cmake_minimum_required(VERSION 3.16 FATAL_ERROR)
################### Variables. ####################
# Change if you want modify path or other values. #
###################################################


set(CMAKE_MACOSX_BUNDLE 1)
# Define Release by default.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}.")

if (NOT $ENV{TEST_NAME} STREQUAL "")
  set(PROJECT_NAME $ENV{TEST_NAME})
else ()
  set(PROJECT_NAME fuzz_harness)
endif ()

message(STATUS "Project name: ${PROJECT_NAME}.")


# Output Variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Folders files
set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CPP_DIR_1 ${CMAKE_CURRENT_SOURCE_DIR}/../engine)
set(HEADER_DIR_1 ${CMAKE_CURRENT_SOURCE_DIR}/../engine)

file(GLOB_RECURSE RES_SOURCES "${DATA_DIR}/data/*")

# libFuzzer comes with clang, and on macOS only with the one from Homebrew, since
# AppleClang has no -fsanitize=fuzzer; the hints below are where brew puts it. A
# compiler chosen from outside is left alone, and a machine with no clang at all
# is told so here instead of failing halfway through the build.
if (NOT CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
  find_program(ARCTIC_CLANGXX NAMES clang++ clang++-21 clang++-20 clang++-19
    clang++-18 clang++-17 clang++-16 clang++-15 clang++-14
    HINTS /opt/homebrew/opt/llvm/bin /usr/local/opt/llvm/bin)
  if (ARCTIC_CLANGXX)
    set(CMAKE_CXX_COMPILER "${ARCTIC_CLANGXX}")
    message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
  else ()
    message(FATAL_ERROR "No clang++ found. The fuzz tests need clang with"
      " libFuzzer in it, gcc will not do: install clang (apt install clang, or"
      " brew install llvm on macOS), or point CMAKE_CXX_COMPILER at it.")
  endif ()
endif ()

set(CMAKE_CXX_STANDARD 14)
set(THREADS_PREFER_PTHREAD_FLAG ON)
############## Define Project. ###############
# ---- This the main options of project ---- #
##############################################

project(${PROJECT_NAME} CXX)
ENABLE_LANGUAGE(C)

IF (APPLE)
  FIND_LIBRARY(AUDIOTOOLBOX AudioToolbox)
  FIND_LIBRARY(COREAUDIO CoreAudio)
  FIND_LIBRARY(COREFOUNDATION CoreFoundation)
  FIND_LIBRARY(COCOA Cocoa)
  FIND_LIBRARY(GAMECONTROLLER GameController)
  FIND_LIBRARY(OPENGL OpenGL)
  FIND_LIBRARY(AVFOUNDATION AVFoundation)
  FIND_LIBRARY(COREVIDEO CoreVideo)
  FIND_LIBRARY(COREMEDIA CoreMedia)
ELSE (APPLE)
  # Sound is optional: a machine without the ALSA headers, a container, a build
  # bot, all of them build and run, only without a sound of any kind. That has to
  # be said out loud here and at run time, or silence looks like a bug in the
  # program.
  find_package(ALSA)
  if (NOT ALSA_FOUND)
    add_definitions(-DARCTIC_NO_ALSA)
    message(WARNING "ALSA was not found: the sound player is built as a stub and"
      " nothing will be audible. Install the ALSA headers to get sound"
      " (apt install libasound2-dev).")
  endif ()

  # Two ways to draw on Linux: desktop OpenGL through GLX, and OpenGL ES through
  # EGL. Both draw into an X11 window, so X11 is needed either way. The choice used
  # to fall to ES/EGL as soon as the GLES headers happened to be installed, which
  # sent a desktop with libgles2-mesa-dev on it down the Raspberry Pi path; now GLX
  # wins unless the machine is a Pi or has no desktop OpenGL at all.
  # -DARCTIC_GRAPHICS=glx or =gles settles it by hand.
  set(ARCTIC_GRAPHICS "auto" CACHE STRING "Graphics backend: auto, glx or gles")
  set_property(CACHE ARCTIC_GRAPHICS PROPERTY STRINGS auto glx gles)

  find_library(EGL_LIBRARY NAMES EGL)
  find_path(EGL_INCLUDE_DIR EGL/egl.h)
  find_library(GLES_LIBRARY NAMES GLESv2)
  find_path(GLES_INCLUDE_DIR GLES/gl.h)
  find_library(ARCTIC_DESKTOP_GL_LIBRARY NAMES GL)

  set(ARCTIC_HAS_GLES FALSE)
  if (EGL_LIBRARY AND EGL_INCLUDE_DIR AND GLES_LIBRARY AND GLES_INCLUDE_DIR)
    set(ARCTIC_HAS_GLES TRUE)
  endif ()

  set(ARCTIC_IS_RASPBERRY_PI FALSE)
  if (EXISTS "/proc/device-tree/model")
    file(READ "/proc/device-tree/model" ARCTIC_DEVICE_TREE_MODEL)
    if (ARCTIC_DEVICE_TREE_MODEL MATCHES "Raspberry Pi")
      set(ARCTIC_IS_RASPBERRY_PI TRUE)
    endif ()
  endif ()

  if (EMSCRIPTEN)
    set(EGL_MODE "EGL")
    message(STATUS "Graphics: GLES through EGL, the browser has nothing else.")
  elseif (ARCTIC_GRAPHICS STREQUAL "gles")
    if (NOT ARCTIC_HAS_GLES)
      message(FATAL_ERROR "ARCTIC_GRAPHICS=gles, but the GLES or EGL headers and"
        " libraries are missing (apt install libgles2-mesa-dev libegl1-mesa-dev).")
    endif ()
    set(EGL_MODE "EGL")
    message(STATUS "Graphics: GLES through EGL, asked for by ARCTIC_GRAPHICS=gles.")
  elseif (ARCTIC_GRAPHICS STREQUAL "glx")
    message(STATUS "Graphics: desktop OpenGL through GLX, asked for by"
      " ARCTIC_GRAPHICS=glx.")
  elseif (NOT ARCTIC_GRAPHICS STREQUAL "auto")
    message(FATAL_ERROR "ARCTIC_GRAPHICS is \"${ARCTIC_GRAPHICS}\": it takes"
      " auto, glx or gles.")
  elseif (ARCTIC_IS_RASPBERRY_PI AND ARCTIC_HAS_GLES)
    set(EGL_MODE "EGL")
    message(STATUS "Graphics: GLES through EGL, this is a Raspberry Pi."
      " -DARCTIC_GRAPHICS=glx draws through GLX instead.")
  elseif (ARCTIC_DESKTOP_GL_LIBRARY)
    message(STATUS "Graphics: desktop OpenGL through GLX."
      " -DARCTIC_GRAPHICS=gles draws through GLES and EGL instead.")
  elseif (ARCTIC_HAS_GLES)
    set(EGL_MODE "EGL")
    message(STATUS "Graphics: GLES through EGL, no desktop OpenGL library here.")
  else ()
    message(FATAL_ERROR "Neither desktop OpenGL nor GLES with EGL was found:"
      " install one of them (apt install libgl1-mesa-dev, or libgles2-mesa-dev"
      " libegl1-mesa-dev).")
  endif ()

  IF (NOT EGL_MODE)
    #only for opengl glx
    set (OpenGL_GL_PREFERENCE "LEGACY")
    find_package(OpenGL REQUIRED)
  ENDIF (NOT EGL_MODE)

  find_package(X11 REQUIRED)
  find_package(Threads REQUIRED)
  find_package(PkgConfig QUIET)
  if (PkgConfig_FOUND)
    pkg_check_modules(GSTREAMER QUIET
      gstreamer-1.0
      gstreamer-app-1.0
      gstreamer-video-1.0)
  endif()
ENDIF (APPLE)


# Definition of Macros

#-D_DEBUG 
IF (APPLE)
  add_definitions(
    -DGL_SILENCE_DEPRECATION
  )
ELSE (APPLE)
	IF (EGL_MODE)
    #only for es egl
    add_definitions(
       -DPLATFORM_RPI 
    )
  ELSE (EGL_MODE)
    #only for opengl glx
    add_definitions(
       -DPLATFORM_LINUX
    )
  ENDIF (EGL_MODE)
  add_definitions(
   -DGLX
   -DGL_GLEXT_PROTOTYPES
  )
  if (GSTREAMER_FOUND)
    add_definitions(-DARCTIC_HAS_GSTREAMER)
    include_directories(${GSTREAMER_INCLUDE_DIRS})
  endif()
ENDIF (APPLE)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)

################# Flags ################
# Defines Flags for Windows and Linux. #
########################################
IF (APPLE)
ELSE (APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
ENDIF (APPLE)

message(STATUS "CompilerId: ${CMAKE_CXX_COMPILER_ID}.")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O3")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang++" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

IF (EGL_MODE)
  #only for  es egl
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lGLESv2 -lEGL")
ENDIF (EGL_MODE)

################ Files ################
#   --   Add files to project.   --   #
#######################################


IF (APPLE)
file(GLOB SRC_FILES
    ${CPP_DIR_1}/*.cpp
    ${CPP_DIR_1}/*.mm
    ${CPP_DIR_1}/*.c
    ${HEADER_DIR_1}/*.h
    ${HEADER_DIR_1}/*.hpp
)
ELSE (APPLE)
file(GLOB SRC_FILES
    ${CPP_DIR_1}/*.cpp
    ${CPP_DIR_1}/*.c
    ${HEADER_DIR_1}/*.h
    ${HEADER_DIR_1}/*.hpp
)
ENDIF (APPLE)
file(GLOB SRC_FILES_TO_REMOVE
    ${CPP_DIR_1}/arctic_platform_pi.cpp
    ${CPP_DIR_1}/byte_array.cpp
    ${HEADER_DIR_1}/byte_array.h
)
list(REMOVE_ITEM SRC_FILES ${SRC_FILES_TO_REMOVE})

macro(fuzzer name)
    add_executable(${PROJECT_NAME} ${name}.cpp ${SRC_FILES} ${RES_SOURCES})
    set_target_properties(${PROJECT_NAME}
            PROPERTIES
            COMPILE_FLAGS "-fsanitize=fuzzer -DARCTIC_NO_MAIN -DARCTIC_NO_HARD_EXIT -DARCTIC_NO_FATAL_MESSAGES"
            LINK_FLAGS "-fsanitize=fuzzer")
    target_include_directories(${PROJECT_NAME}
            PRIVATE
            ../src)
endmacro()

fuzzer( ${PROJECT_NAME} )

# Add executable to build.
SET_SOURCE_FILES_PROPERTIES(${RES_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/data)

IF (APPLE)
target_link_libraries(
  ${PROJECT_NAME}
  ${AUDIOTOOLBOX}
  ${COREAUDIO}
  ${COREFOUNDATION}
  ${COCOA}
  ${GAMECONTROLLER}
  ${OPENGL}
  ${AVFOUNDATION}
  ${COREVIDEO}
  ${COREMEDIA}
)
ELSE (APPLE)
target_link_libraries(
  ${PROJECT_NAME}
  ${OPENGL_gl_LIBRARY}
  ${X11_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${ALSA_LIBRARY}
  #  ${EGL_LIBRARY}
  #  ${GLES_LIBRARY}
)
if (GSTREAMER_FOUND)
  target_link_libraries(${PROJECT_NAME} ${GSTREAMER_LIBRARIES})
  target_link_directories(${PROJECT_NAME} PUBLIC ${GSTREAMER_LIBRARY_DIRS})
endif()
ENDIF (APPLE)

# An out-of-source build (cmake -B build) leaves the binary in the build
# directory, and the program looks for its data next to itself, so the data is
# copied there after every build. An in-source build has it in place already, and
# a macOS bundle carries it in Resources.
if (EXISTS "${DATA_DIR}/data"
    AND NOT "${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}"
    AND NOT APPLE AND NOT EMSCRIPTEN)
  add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
      "${DATA_DIR}/data" "$<TARGET_FILE_DIR:${PROJECT_NAME}>/data")
endif ()
