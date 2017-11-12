
cmake_minimum_required(VERSION 3.0.0 FATAL_ERROR)
################### Variables. ####################
# Change if you want modify path or other values. #
###################################################

# Define Release by default.
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
  message(STATUS "Build type not specified: defaulting to release.")
endif(NOT CMAKE_BUILD_TYPE)

set(PROJECT_NAME demo)
# Output Variables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/demo )
# Folders files
set(CPP_DIR_1 ./engine)
set(CPP_DIR_2 ./demo)
set(HEADER_DIR_1 ./engine)
set(HEADER_DIR_2 ./demo)

SET(CMAKE_CXX_COMPILER             "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 11)
set(THREADS_PREFER_PTHREAD_FLAG ON)
############## Define Project. ###############
# ---- This the main options of project ---- #
##############################################

project(${PROJECT_NAME} CXX)

find_package(ALSA REQUIRED)
find_package(OpenGL REQUIRED)
find_package(X11 REQUIRED)
find_package(Threads REQUIRED)


# Definition of Macros

#-D_DEBUG 
add_definitions(
   -DGLX
   -DPLATFORM_RPI 
)

include_directories(${CMAKE_SOURCE_DIR})

################# Flags ################
# Defines Flags for Windows and Linux. #
########################################

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O2")
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang++")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

################ Files ################
#   --   Add files to project.   --   #
#######################################

file(GLOB SRC_FILES
    ${CPP_DIR_1}/*.cpp
    ${CPP_DIR_2}/*.cpp
    ${HEADER_DIR_1}/*.h
    ${HEADER_DIR_2}/*.h
)


# Add executable to build.
add_executable(${PROJECT_NAME} 
   ${SRC_FILES}
)

target_link_libraries(
    ${PROJECT_NAME}
    ${OPENGL_gl_LIBRARY}
    ${X11_LIBRARIES}
    Threads::Threads
    ${ALSA_LIBRARY}
)

