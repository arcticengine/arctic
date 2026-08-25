# =============================================================================
# How to build (read first)
# =============================================================================
# This file uses paths relative to the project folder (e.g. ../arctic/engine,
# ./data). Configure and build from the directory that contains THIS file so
# those paths resolve as intended. A separate build/ subdirectory will result
# in a failed build.
#
# Always build from this directory:
#   cmake . && make -j 8
# =============================================================================


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

set(PROJECT_NAME template_project_name)
# Output Variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Folders files
set(DATA_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(CPP_DIR_1 ${CMAKE_CURRENT_SOURCE_DIR}/../arctic/engine)
set(CPP_DIR_2 ${CMAKE_CURRENT_SOURCE_DIR})
set(HEADER_DIR_1 ${CMAKE_CURRENT_SOURCE_DIR}/../arctic/engine)
set(HEADER_DIR_2 ${CMAKE_CURRENT_SOURCE_DIR})

file(GLOB_RECURSE RES_SOURCES "${DATA_DIR}/data/*")

# The engine is built with clang, and only with clang: it leans on libc++ and on
# flags that gcc spells differently, so a gcc build fails in ways that say nothing
# about the real cause. A compiler chosen from outside (-DCMAKE_CXX_COMPILER=...,
# the CXX environment variable, a toolchain file) is left alone; otherwise clang++
# is looked up in the PATH, versioned names included, and a machine that has none
# is told so here instead of failing halfway through the build.
if (NOT CMAKE_CXX_COMPILER AND NOT DEFINED ENV{CXX})
  find_program(ARCTIC_CLANGXX NAMES clang++ clang++-21 clang++-20 clang++-19
    clang++-18 clang++-17 clang++-16 clang++-15 clang++-14)
  if (ARCTIC_CLANGXX)
    set(CMAKE_CXX_COMPILER "${ARCTIC_CLANGXX}")
    message(STATUS "C++ compiler: ${CMAKE_CXX_COMPILER}")
  else ()
    message(FATAL_ERROR "No clang++ found. Arctic Engine needs clang, gcc is not"
      " supported: install it (apt install clang, or xcode-select --install on"
      " macOS), or point CMAKE_CXX_COMPILER at the clang++ you have.")
  endif ()
endif ()
set(CMAKE_CXX_STANDARD 14)
set(THREADS_PREFER_PTHREAD_FLAG ON)
############## Define Project. ###############
# ---- This the main options of project ---- #
##############################################
IF (EMSCRIPTEN)
  unset(APPLE)
ELSE (EMSCRIPTEN)
ENDIF (EMSCRIPTEN)

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
  # Keychain access: engine/httplib.h reads the trusted roots from there once
  # HTTPS is enabled below.
  FIND_LIBRARY(SECURITY Security)
ELSE (APPLE)

  IF (EMSCRIPTEN)
  ELSE (EMSCRIPTEN)
    # Sound is optional: a machine without the ALSA headers, a container, a build
    # bot, all of them build and run, only without a sound of any kind.
    find_package(ALSA)
    if (NOT ALSA_FOUND)
      add_definitions(-DARCTIC_NO_ALSA)
      message(WARNING "ALSA was not found: the sound player is built as a stub and"
        " nothing will be audible. Install the ALSA headers to get sound"
        " (apt install libasound2-dev).")
    endif ()
    find_package(X11 REQUIRED)
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
      pkg_check_modules(GSTREAMER QUIET
        gstreamer-1.0
        gstreamer-app-1.0
        gstreamer-video-1.0)
    endif()
  ENDIF (EMSCRIPTEN)

  # Two ways to draw on Linux: desktop OpenGL through GLX, and OpenGL ES through
  # EGL. Both draw into an X11 window, so X11 is needed either way. 
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

  find_package(Threads REQUIRED)
ENDIF (APPLE)

# HTTPS for engine/httplib.h is optional, so OpenSSL is looked up but never
# required: with it the HTTP client and server speak TLS, without it they are
# limited to plain HTTP and nothing else in the engine is affected. Emscripten is
# skipped because the browser terminates TLS itself.
IF (NOT EMSCRIPTEN)
  IF (APPLE AND NOT OPENSSL_ROOT_DIR)
    # Homebrew installs OpenSSL outside the default search path, so hint the
    # common locations before find_package.
    foreach(cand /opt/homebrew/opt/openssl@3 /usr/local/opt/openssl@3
                 /opt/homebrew/opt/openssl /usr/local/opt/openssl)
      if(EXISTS "${cand}")
        set(OPENSSL_ROOT_DIR "${cand}")
        break()
      endif()
    endforeach()
  ENDIF ()
  find_package(OpenSSL QUIET)
  if (OPENSSL_FOUND)
    message(STATUS "OpenSSL found: httplib HTTPS enabled.")
    add_definitions(-DCPPHTTPLIB_OPENSSL_SUPPORT)
  else ()
    message(STATUS "OpenSSL not found: httplib is limited to plain HTTP. "
                   "Install OpenSSL (libssl-dev / brew install openssl) to "
                   "enable HTTPS.")
  endif ()
ENDIF (NOT EMSCRIPTEN)



# Definition of Macros

#-D_DEBUG 
IF (APPLE)
  add_definitions(
    -DGL_SILENCE_DEPRECATION
  )
ELSE (APPLE)
  IF (EMSCRIPTEN)
    add_definitions(
      -DPLATFORM_WEB 
    )
  ELSE (EMSCRIPTEN)
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
  ENDIF (EMSCRIPTEN)
  
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
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../arctic)

################# Flags ################
# Defines Flags for Windows and Linux. #
########################################
IF (APPLE)
ELSE (APPLE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
ENDIF (APPLE)

message(STATUS "CompilerId: ${CMAKE_CXX_COMPILER_ID}.")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang++" OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "AppleClang")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
    set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
    set (CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
endif()

IF (EGL_MODE)
  #only for  es egl
  set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lGLESv2 -lEGL")
ENDIF (EGL_MODE)

IF (EMSCRIPTEN)
    # Set memory flags in linker flags, not compiler flags
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s INITIAL_MEMORY=1GB -sASSERTIONS -sASYNCIFY")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sEXIT_RUNTIME=1")

    
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -sPTHREAD_POOL_SIZE=4")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s AUDIO_WORKLET=1 -sWASM_WORKERS=1")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sSTACK_SIZE=262144")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -sASYNCIFY_STACK_SIZE=262144")

    # Debug flags for source maps and C++ filenames/line numbers
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g3 -gsource-map")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -O0 -g3 -gsource-map --source-map-base http://127.0.0.1:8000/")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -s WASM=1")
ELSE (EMSCRIPTEN)
ENDIF (EMSCRIPTEN)

################ Files ################
#   --   Add files to project.   --   #
#######################################


IF (APPLE)
file(GLOB SRC_FILES
    ${CPP_DIR_1}/*.cpp
    ${CPP_DIR_1}/*.mm
    ${CPP_DIR_1}/*.c
    ${CPP_DIR_2}/*.cpp
    ${CPP_DIR_2}/*.c
    ${HEADER_DIR_1}/*.h
    ${HEADER_DIR_1}/*.hpp
    ${HEADER_DIR_2}/*.h
    ${HEADER_DIR_2}/*.hpp
)
ELSE (APPLE)
file(GLOB SRC_FILES
    ${CPP_DIR_1}/*.cpp
    ${CPP_DIR_1}/*.c
    ${CPP_DIR_2}/*.cpp
    ${CPP_DIR_2}/*.c
    ${HEADER_DIR_1}/*.h
    ${HEADER_DIR_1}/*.hpp
    ${HEADER_DIR_2}/*.h
    ${HEADER_DIR_2}/*.hpp
)
ENDIF (APPLE)
file(GLOB SRC_FILES_TO_REMOVE
    ${CPP_DIR_1}/arctic_platform_pi.cpp
    ${CPP_DIR_1}/byte_array.cpp
    ${HEADER_DIR_1}/byte_array.h
)
list(REMOVE_ITEM SRC_FILES ${SRC_FILES_TO_REMOVE})


# Add executable to build.
add_executable(${PROJECT_NAME} MACOSX_BUNDLE
   ${SRC_FILES}
   ${RES_SOURCES}
)

foreach(RES_FILE ${RES_SOURCES})
  get_filename_component(ABSOLUTE_PATH "${DATA_DIR}/data" ABSOLUTE)
  file(RELATIVE_PATH RES_PATH "${ABSOLUTE_PATH}" ${RES_FILE})
  get_filename_component(RES_DIR_PATH ${RES_PATH} DIRECTORY)
  set_property(SOURCE ${RES_FILE} PROPERTY MACOSX_PACKAGE_LOCATION "Resources/data/${RES_DIR_PATH}")
endforeach(RES_FILE)

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
  IF (EMSCRIPTEN)
    target_link_libraries(
      ${PROJECT_NAME}
      ${OPENGL_gl_LIBRARY}
      ${CMAKE_THREAD_LIBS_INIT}
    )
  ELSE (EMSCRIPTEN)
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
  ENDIF (EMSCRIPTEN)
ENDIF (APPLE)

if (OPENSSL_FOUND)
  target_link_libraries(${PROJECT_NAME} OpenSSL::SSL OpenSSL::Crypto)
  IF (APPLE)
    target_link_libraries(${PROJECT_NAME} ${SECURITY})
  ENDIF (APPLE)
endif ()

# The browser has no file system to read the assets from, so they are packed into
# a .data file next to the .js and unpacked into a virtual file system at start.
# The packager resolves a relative path against the directory the linker runs in,
# which in a build directory of its own holds no data at all, so the path is
# written in full and mapped to /data, where the program looks for it. The quotes
# reach the link command and keep a space in the path from splitting the argument.
IF (EMSCRIPTEN)
  set_target_properties(${PROJECT_NAME} PROPERTIES
    LINK_FLAGS "--preload-file \"${DATA_DIR}/data\"@/data")
ENDIF (EMSCRIPTEN)

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

# The page that loads the .js and the server that hands it to the browser are in
# the project directory, while a build of its own leaves the .js somewhere else,
# so both are copied next to it and the result is served from a single directory.
if (EMSCRIPTEN
    AND NOT "${CMAKE_CURRENT_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_SOURCE_DIR}")
  foreach (WEB_FILE index.html webserver.py)
    if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${WEB_FILE}")
      add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
          "${CMAKE_CURRENT_SOURCE_DIR}/${WEB_FILE}"
          "$<TARGET_FILE_DIR:${PROJECT_NAME}>/${WEB_FILE}")
    endif ()
  endforeach ()
endif ()
