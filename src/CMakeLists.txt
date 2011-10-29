#-------------------------------------------------------------------------------

# CMake configuration

#-------------------------------------------------------------------------------

cmake_minimum_required(VERSION 2.6)

SET(CMAKE_VERBOSE_MAKEFILE OFF)	
#SET(CMAKE_BUILD_TYPE Release)

SET(CMAKE_BUILD_TYPE Debug)



#-------------------------------------------------------------------------------

# Project configuration

#-------------------------------------------------------------------------------

PROJECT(BokehRendering)


#-------------------------------------------------------------------------------

# Definitions

#-------------------------------------------------------------------------------

#ADD_DEFINITIONS(-DNDEBUG)

#ADD_DEFINITIONS(-DDEBUG)



#-------------------------------------------------------------------------------

# Compiler & linker options

#-------------------------------------------------------------------------------

IF(MSVC)

ELSE(MSVC)
	SET(CMAKE_CXX_FLAGS "-O0 -g -pg")

	#SET(CMAKE_LD_FLAGS  "-msse2 -mfmath=sse")



ENDIF(MSVC)

#-------------------------------------------------------------------------------

# Extra directories
#-------------------------------------------------------------------------------

FIND_PACKAGE(OpenGL)
IF(NOT OPENGL_FOUND)
	message("ERROR: OpenGL not found")
ENDIF(NOT OPENGL_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/freeglut-2.6.0/)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glm-0.9.2.4/)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glew-1.6.0/include/)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glew-1.6.0/include/)


IF(MSVC)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/freeglut-2.6.0/lib/win64-vc)
	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glew-1.6.0/lib/win64-vc)
ELSE(MSVC)
	INCLUDE_DIRECTORIES(/usr/include/nvidia-current/)

ENDIF(MSVC)





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

