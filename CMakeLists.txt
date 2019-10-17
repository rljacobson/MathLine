# Create the project.
cmake_minimum_required(VERSION 2.6)
project(mathline)
add_executable(mathline ${CMAKE_SOURCE_DIR}/src/main.cpp ${CMAKE_SOURCE_DIR}/src/mlbridge.cpp)
include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/dep/linenoise-ng/include" "${CMAKE_SOURCE_DIR}/build")

# Enable C++11 extensions.
target_compile_features(mathline PRIVATE cxx_constexpr)

#### FindMathematica ####
cmake_policy(SET CMP0012 OLD) # Silences warnings from FindMathematica
cmake_policy(SET CMP0011 NEW) # Silences warnings from FindMathematica
set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Mathematica" ${CMAKE_MODULE_PATH})
# Dynamically linking to MathLink introduces a nondeterministic malloc error upon
# exit. Statically linking seems to solve this problem.
set(Mathematica_USE_LIBCXX_LIBRARIES OFF)
set(Mathematica_USE_STATIC_LIBRARIES ON)
find_package(Mathematica COMPONENTS WSTP MathLink)
# The following is only relevant for linking dynamically to MathLink on Mac OS X.
# We record it here for future reference.
# Fix MathLink shared library references under Mac OS X:
# Mathematica_ABSOLUTIZE_LIBRARY_DEPENDENCIES(mathline)

# Choose between WSTP and MathLink.
if(Mathematica_WSTP_FOUND)
	set(WSTP true)
	set(ML_PREFIX WS)
	include_directories(${Mathematica_WSTP_INCLUDE_DIR})
	target_link_libraries(mathline ${Mathematica_WSTP_LIBRARY})
	message("WSTP library is: ${Mathematica_WSTP_LIBRARY}")
elseif(Mathematica_MathLink_FOUND)
	set(WSTP false)
	set(ML_PREFIX ML)
	include_directories(${Mathematica_MathLink_INCLUDE_DIR})
	target_link_libraries(mathline ${Mathematica_MathLink_LIBRARY})
endif()

# Compile linenose-ng library and link to it
add_subdirectory(dep/linenoise-ng)
target_link_libraries(mathline linenoise)

# Link to the correct stdlib.
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	# We're on macOS X.
	# TODO: Prior to version 10.4, use -lstdc++ in EXTRA_LIBS to link against libstdc++ instead of libc++.
	# MathLink requires we link to CoreFoundation
	find_library(COREFOUNDATION_LIBRARY CoreFoundation)
	target_link_libraries(mathline ${COREFOUNDATION_LIBRARY})
	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		# Using GCC. MathLink requires additional libraries.
		target_link_libraries(mathline m pthread c++ dl)
	endif()
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	# We're on Microsoft Windows.

else()
	# The libuuid library is special. On (at least) Ubuntu libuuid.so isn't
	# symlinked to libuuid.so.1 if libuuid-dev isn't installed, so ld can't
	# find -luuid.
	find_library(UUID_LIBRARY REQUIRED NAMES uuid libuuid.so.1)
	target_link_libraries(mathline m pthread rt stdc++ dl ${UUID_LIBRARY})
endif()

# Configure a header file to pass some of the CMake settings
# to the source code
configure_file("${CMAKE_SOURCE_DIR}/src/config.h.in" "${CMAKE_SOURCE_DIR}/build/config.h")

# Installation
install(TARGETS mathline DESTINATION bin)