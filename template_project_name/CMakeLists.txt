
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

set(PROJECT_NAME template_project_name)
# Output Variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
# Folders files
set(DATA_DIR .)
set(CPP_DIR_1 ../arctic/engine)
set(CPP_DIR_2 .)
set(HEADER_DIR_1 ../arctic/engine)
set(HEADER_DIR_2 .)

file(GLOB_RECURSE RES_SOURCES "${DATA_DIR}/data/*")

SET(CMAKE_CXX_COMPILER             "/usr/bin/clang++")
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
ELSE (APPLE)
  find_package(ALSA REQUIRED)

  find_library(EGL_LIBRARY NAMES EGL)
  find_path(EGL_INCLUDE_DIR EGL/egl.h)
  find_library(GLES_LIBRARY NAMES GLESv2)
  find_path(GLES_INCLUDE_DIR GLES/gl.h)
  IF (EGL_LIBRARY AND EGL_INCLUDE_DIR AND GLES_LIBRARY AND GLES_INCLUDE_DIR)
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

  find_package(X11 REQUIRED)
  find_package(Threads REQUIRED)
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
ENDIF (APPLE)

include_directories(${CMAKE_SOURCE_DIR}/..)
include_directories(${CMAKE_SOURCE_DIR}/../arctic)

################# Flags ################
# Defines Flags for Windows and Linux. #
########################################

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
target_link_libraries(
  ${PROJECT_NAME}
  ${OPENGL_gl_LIBRARY}
  ${X11_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${ALSA_LIBRARY}
  #  ${EGL_LIBRARY}
  #  ${GLES_LIBRARY}
)
ENDIF (APPLE)
