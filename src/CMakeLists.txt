#-------------------------------------------------------------------------------
# CMake configuration
#-------------------------------------------------------------------------------
cmake_minimum_required(VERSION 2.6)
SET(CMAKE_VERBOSE_MAKEFILE ON)	
SET(CMAKE_BUILD_TYPE Debug)

#-------------------------------------------------------------------------------
# Macro
#-------------------------------------------------------------------------------
MACRO(addExternalPackageGTC NAME DIRECTORY)
	SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/external/${DIRECTORY}/util)
	SET(${NAME}_PACKAGE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external/${DIRECTORY})
	FIND_PACKAGE(${NAME})
	IF(NOT ${NAME}_FOUND)
		MESSAGE("ERROR: ${NAME} not found")
	ENDIF(NOT ${NAME}_FOUND)
ENDMACRO(addExternalPackageGTC)
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
	SET(CMAKE_CXX_FLAGS "-O0 -g -pg")
	#SET(CMAKE_LD_FLAGS  "-msse2 -mfmath=sse")
ENDIF(MSVC)

#-------------------------------------------------------------------------------
# Extra directories
#-------------------------------------------------------------------------------
FIND_PACKAGE(OpenGL)
FIND_PACKAGE(GLUT)
IF(NOT OPENGL_FOUND)
	MESSAGE("ERROR: OpenGL not found")
ENDIF(NOT OPENGL_FOUND)

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR})

IF(MSVC)
	addExternalPackageGTC("GLEW" "glew-1.6.0")
	addExternalPackageGTC("FREEGLUT" "freeglut-2.6.0")
	addExternalPackageGTC("GLM" "glm-0.9.2.4")
ELSE(MSVC)
	INCLUDE_DIRECTORIES(/usr/include/nvidia-current/)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glm-0.9.2.4/)
	INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glew-1.6.0/include/)
	LINK_DIRECTORIES(${CMAKE_SOURCE_DIR}/external/glew-1.5.7/lib/)
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
TARGET_LINK_LIBRARIES(BOKEH ${OPENGL_LIBRARY} ${GLEW_LIBRARY} ${FREEGLUT_LIBRARY})