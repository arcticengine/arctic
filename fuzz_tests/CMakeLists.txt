
cmake_minimum_required(VERSION 3.0.2 FATAL_ERROR)
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
set(DATA_DIR .)
set(CPP_DIR_1 ../engine)
set(HEADER_DIR_1 ../engine)

file(GLOB_RECURSE RES_SOURCES "${DATA_DIR}/data/*")

IF (APPLE)
  # Install clang with 'brew install llvm' for this to work
  SET(CMAKE_CXX_COMPILER             "/usr/local/opt/llvm/bin/clang++")
ELSE (APPLE)
  SET(CMAKE_CXX_COMPILER             "/usr/bin/clang++")
ENDIF (APPLE)

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
