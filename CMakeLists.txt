cmake_minimum_required (VERSION 2.6)
project (MathLine)
add_executable(MathLine ${CMAKE_SOURCE_DIR}/src/main.cpp ${CMAKE_SOURCE_DIR}/src/mlbridge.cpp ${CMAKE_SOURCE_DIR}/src/linenoise.c)
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/build")

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -Wc++11-extensions" )

#### FindMathematica ####
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Mathematica" ${CMAKE_MODULE_PATH})
# Dynamically linking to MathLink introduces a nondeterministic malloc error upon
# exit. Statically linking seems to solve this problem. 
set (Mathematica_USE_LIBCXX_LIBRARIES OFF)
set (Mathematica_USE_STATIC_LIBRARIES ON)
find_package(Mathematica COMPONENTS WSTP MathLink)
# The following is only relevant for linking dynamically to MathLink on Mac OS X.
# We record it here for future reference.
# Fix MathLink shared library references under Mac OS X:
# Mathematica_ABSOLUTIZE_LIBRARY_DEPENDENCIES(MathLine)

#### Boost ####
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_STATIC_RUNTIME ON) 
find_package(Boost COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
target_link_libraries(MathLine ${Boost_LIBRARIES})

# Choose between WSTP and MathLink.
if(Mathematica_WSTP_FOUND)
	set(WSTP true)
	set(ML_PREFIX WS)
	include_directories(${Mathematica_WSTP_INCLUDE_DIR})
	target_link_libraries(MathLine ${Mathematica_WSTP_LIBRARY})
elseif(Mathematica_MathLink_FOUND)
	set(WSTP false)
	set(ML_PREFIX ML)
	include_directories(${Mathematica_MathLink_INCLUDE_DIR})
	target_link_libraries(MathLine ${Mathematica_MathLink_LIBRARY})
endif()

if(APPLE)
	# MathLink requires we link to CoreFoundation
	find_library(COREFOUNDATION_LIBRARY CoreFoundation)
	target_link_libraries(MathLine ${COREFOUNDATION_LIBRARY})
endif()

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file ("${CMAKE_SOURCE_DIR}/src/config.h.in" "${CMAKE_SOURCE_DIR}/build/config.h")

# Installation
install(TARGETS MathLine DESTINATION bin)