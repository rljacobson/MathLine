cmake_minimum_required (VERSION 2.6)
project (MathLine)

#### FindMathematica ####
set (CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/CMake/Mathematica" ${CMAKE_MODULE_PATH})
# Dynamically linking to MathLink introduces a nondeterministic malloc error upon
# exit. Statically linking seems to solve this problem. 
set (Mathematica_USE_STATIC_LIBRARIES ON)
find_package(Mathematica COMPONENTS WSTP MathLink)
# The following is only relevant for linking dynamically to MathLink on Mac OS X.
# We record it here for future reference.
# Fix MathLink shared library references under Mac OS X:
# Mathematica_ABSOLUTIZE_LIBRARY_DEPENDENCIES(MathLine)

#### Boost ####
set(Boost_USE_STATIC_LIBS ON) 
set(Boost_USE_STATIC_RUNTIME ON) 
find_package(Boost 1.45.0 COMPONENTS program_options) 

#### Mac OS X Specific Stuff ####
if(APPLE)
    # MathLink requires we link to CoreFoundation
	find_library(COREFOUNDATION_LIBRARY CoreFoundation)
endif()

if(Boost_FOUND AND Mathematica_WSTP_FOUND)
	# Use the WSTP version of mlbridge.
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/WSTP/mlbridge.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/mlbridge.cpp COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/WSTP/mlbridge.h ${CMAKE_CURRENT_SOURCE_DIR}/src/mlbridge.h COPYONLY)
	include_directories(${Boost_INCLUDE_DIRS} ${Mathematica_WSTP_INCLUDE_DIR}) 
    add_executable(MathLine src/main.cpp src/mlbridge.cpp src/linenoise.c)
    target_link_libraries(MathLine ${Boost_LIBRARIES} ${Mathematica_WSTP_LIBRARY})
    if (APPLE)
    	# MathLink requires we link to CoreFoundation
		target_link_libraries(MathLine ${COREFOUNDATION_LIBRARY})
	endif()
elseif(Boost_FOUND AND Mathematica_MathLink_FOUND)
	# Use the MathLink version of mlbridge.
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/MathLink/mlbridge.cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/mlbridge.cpp COPYONLY)
	configure_file(${CMAKE_CURRENT_SOURCE_DIR}/src/MathLink/mlbridge.h ${CMAKE_CURRENT_SOURCE_DIR}/src/mlbridge.h COPYONLY)
	include_directories(${Boost_INCLUDE_DIRS} ${Mathematica_MathLink_INCLUDE_DIR}) 
    add_executable(MathLine src/main.cpp src/mlbridge.cpp src/linenoise.c)
    target_link_libraries(MathLine ${Boost_LIBRARIES} ${Mathematica_MathLink_LIBRARY})
    if(APPLE)
    	# MathLink requires we link to CoreFoundation
		target_link_libraries(MathLine ${COREFOUNDATION_LIBRARY})
	endif()
endif()
