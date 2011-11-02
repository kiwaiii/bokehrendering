#-------------------------------------------------------------------------------
# CMake configuration
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_VERBOSE_MAKEFILE ON)	
SET(CMAKE_BUILD_TYPE Debug)

#-------------------------------------------------------------------------------
# Macro
#-------------------------------------------------------------------------------
MACRO(addExternalPackage NAME DIRECTORY)
	SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/external/${DIRECTORY}/util)
	SET(${NAME}_PACKAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/${DIRECTORY})
	FIND_PACKAGE(${NAME})
	IF(NOT ${NAME}_FOUND)
		MESSAGE("ERROR: ${NAME} not found")
	ENDIF(NOT ${NAME}_FOUND)
ENDMACRO(addExternalPackage)

#-------------------------------------------------------------------------------
# Project configuration
#-------------------------------------------------------------------------------
PROJECT(BokehRendering)

#-------------------------------------------------------------------------------
# Definitions
#-------------------------------------------------------------------------------
IF(MSVC)
	ADD_DEFINITIONS(-D_CRT_SECURE_NO_WARNINGS)
	ADD_DEFINITIONS(-D_UNICODE)
	ADD_DEFINITIONS(-DUNICODE)
	ADD_DEFINITIONS(-DGLEW_STATIC)
	ADD_DEFINITIONS(-DFREEGLUT_STATIC)
ENDIF(MSVC)

#-------------------------------------------------------------------------------
# Compiler & linker options
#-------------------------------------------------------------------------------
IF(MSVC)
ELSE(MSVC)
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g -pg -Wall")
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wall")
	#SET(CMAKE_LD_FLAGS  "-msse2 -mfmath=sse")
ENDIF(MSVC)

#-------------------------------------------------------------------------------
# Extra directories
#-------------------------------------------------------------------------------
INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

IF(MSVC)
	FIND_PACKAGE(OpenGL)
	IF(NOT OPENGL_FOUND)
		MESSAGE("ERROR: OpenGL not found")
	ENDIF(NOT OPENGL_FOUND)
	addExternalPackage("FREEGLUT" "freeglut-2.6.0")
	addExternalPackage("GLEW" "glew-1.7.0")
	addExternalPackage("GLM" "glm-0.9.2.4")
	addExternalPackage("DEVIL" "devil-1.7.8")
ELSE(MSVC)
	FIND_PACKAGE(GLUT)
	FIND_PACKAGE(DEVIL)
	INCLUDE_DIRECTORIES(/usr/include/nvidia-current/)
	addExternalPackage("GLEW" "glew-1.7.0")
	addExternalPackage("GLM" "glm-0.9.2.4")
ENDIF(MSVC)

#-------------------------------------------------------------------------------
# Sub directories
#-------------------------------------------------------------------------------
ADD_SUBDIRECTORY(glf)
ADD_SUBDIRECTORY(glui)
ADD_SUBDIRECTORY(glf/io)

#-------------------------------------------------------------------------------
# Libraries definitions
#-------------------------------------------------------------------------------
ADD_EXECUTABLE(BOKEH mainbokeh.cpp ${GLF_SRCS} ${GLUI_SRCS})
TARGET_LINK_LIBRARIES(BOKEH ${OPENGL_LIBRARY} ${GLEW_LIBRARY} ${FREEGLUT_LIBRARY} ${GLUT_LIBRARY} ${DEVIL_LIBRARY})
