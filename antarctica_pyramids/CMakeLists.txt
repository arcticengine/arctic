cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
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

set(PROJECT_NAME antarctica_pyramids)
# Output Variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Folders files
set(DATA_DIR .)
set(CPP_DIR_1 ../engine)
set(CPP_DIR_2 .)
set(HEADER_DIR_1 ../engine)
set(HEADER_DIR_2 .)

file(GLOB_RECURSE RES_SOURCES "${DATA_DIR}/data/*")

SET(CMAKE_CXX_COMPILER             "/usr/bin/clang++")
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
ELSE (APPLE)

  IF (EMSCRIPTEN)
  ELSE (EMSCRIPTEN)
    find_package(ALSA REQUIRED)
    find_package(X11 REQUIRED)
  ENDIF (EMSCRIPTEN)

  find_library(EGL_LIBRARY NAMES EGL)
  find_path(EGL_INCLUDE_DIR EGL/egl.h)
  find_library(GLES_LIBRARY NAMES GLESv2)
  find_path(GLES_INCLUDE_DIR GLES/gl.h)
  IF (EMSCRIPTEN OR (EGL_LIBRARY AND EGL_INCLUDE_DIR AND GLES_LIBRARY AND GLES_INCLUDE_DIR))
    message(STATUS "GLES EGL mode")
    set(EGL_MODE "EGL")
  ELSE ()
    message(STATUS "OPENGL GLX mode")
  ENDIF()

  IF (NOT EGL_MODE)
    #only for opengl glx
    set (OpenGL_GL_PREFERENCE "LEGACY")
    find_package(OpenGL REQUIRED)
  ENDIF (NOT EGL_MODE)

  find_package(Threads REQUIRED)
ENDIF (APPLE)



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
ENDIF (APPLE)

include_directories(${CMAKE_SOURCE_DIR}/..)

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
    set (CMAKE_LINKER_FLAGS_DEBUG "${CMAKE_STATIC_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address")
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
  ENDIF (EMSCRIPTEN)
ENDIF (APPLE)

IF (EMSCRIPTEN)
  set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS "--preload-file data")
ENDIF (EMSCRIPTEN)
