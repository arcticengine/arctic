
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


set(PROJECT_NAME tests)
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

project(${PROJECT_NAME} CXX)
ENABLE_LANGUAGE(C)

IF (APPLE)
  FIND_LIBRARY(AUDIOTOOLBOX AudioToolbox)
  FIND_LIBRARY(COREAUDIO CoreAudio)
  FIND_LIBRARY(COREFOUNDATION CoreFoundation)
  FIND_LIBRARY(COCOA Cocoa)
  FIND_LIBRARY(OPENGL OpenGL)
ELSE (APPLE)
  find_package(ALSA REQUIRED)
  find_package(OpenGL REQUIRED)
  find_package(X11 REQUIRED)
  find_package(Threads REQUIRED)
ENDIF (APPLE)


# Definition of Macros

#-D_DEBUG 
IF (APPLE)
ELSE (APPLE)
add_definitions(
   -DGLX
   -DPLATFORM_RPI 
)
ENDIF (APPLE)

include_directories(${CMAKE_SOURCE_DIR}/..)

################# Flags ################
# Defines Flags for Windows and Linux. #
########################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O2")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang++")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

################ Files ################
#   --   Add files to project.   --   #
#######################################

IF (APPLE)
file(GLOB SRC_FILES
#    ${CPP_DIR_1}/*.cpp
#    ${CPP_DIR_1}/*.mm
    ${CPP_DIR_2}/*.cpp
    ${HEADER_DIR_1}/*.h
    ${HEADER_DIR_2}/*.h
)
ELSE (APPLE)
file(GLOB SRC_FILES
#    ${CPP_DIR_1}/*.cpp
    ${CPP_DIR_2}/*.cpp
    ${HEADER_DIR_1}/*.h
    ${HEADER_DIR_2}/*.h
)
ENDIF (APPLE)


# Add executable to build.
add_executable(${PROJECT_NAME} MACOSX_BUNDLE
   ${SRC_FILES}
   ${RES_SOURCES}
)
SET_SOURCE_FILES_PROPERTIES(${RES_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/data)

IF (APPLE)
target_link_libraries(
  ${PROJECT_NAME}
  ${AUDIOTOOLBOX}
  ${COREAUDIO}
  ${COREFOUNDATION}
  ${COCOA}
  ${OPENGL}
)
ELSE (APPLE)
target_link_libraries(
  ${PROJECT_NAME}
  ${OPENGL_gl_LIBRARY}
  ${X11_LIBRARIES}
  ${CMAKE_THREAD_LIBS_INIT}
  ${ALSA_LIBRARY}
)
ENDIF (APPLE)
