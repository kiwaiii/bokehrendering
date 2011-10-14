#-------------------------------------------------------------------------------
# CMake configuration
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_VERBOSE_MAKEFILE OFF)	# Set build verbose
#SET(CMAKE_BUILD_TYPE Release)
SET(CMAKE_BUILD_TYPE Debug)

#-------------------------------------------------------------------------------
# Project configuration
#-------------------------------------------------------------------------------
PROJECT(Demos)

#-------------------------------------------------------------------------------
# Definitions
#-------------------------------------------------------------------------------
#ADD_DEFINITIONS(-DNDEBUG)
#ADD_DEFINITIONS(-DDEBUG)


#-------------------------------------------------------------------------------
# Compiler & linker options
#-------------------------------------------------------------------------------
SET(CMAKE_CXX_FLAGS "-O0 -g -pg")
#SET(CMAKE_LD_FLAGS "-m64")
#SET(CMAKE_LD_FLAGS  "-m32 -msse2 -mfmath=sse")


#-------------------------------------------------------------------------------
# Dependencies 
#-------------------------------------------------------------------------------
SET(EXTRA_MODULES_PATH "$ENV{HOME}/.local/share/CMakeModules/")
SET(CMAKE_MODULE_PATH ${EXTRA_MODULES_PATH})
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glm-0.9.1.1/)
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glew-1.5.7/include/)
INCLUDE_DIRECTORIES(/usr/include/nvidia-current/)
#-------------------------------------------------------------------------------
LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glew-1.5.7/lib/)


#-------------------------------------------------------------------------------
# Sub directories
#-------------------------------------------------------------------------------
ADD_SUBDIRECTORY(glf)
ADD_SUBDIRECTORY(glui)

#-------------------------------------------------------------------------------
# Libraries definitions
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(BOKEH mainbokeh.cpp ${GLF_SRCS} ${GLUI_SRCS})
TARGET_LINK_LIBRARIES(BOKEH -lGL -lGLEW -lglut)

