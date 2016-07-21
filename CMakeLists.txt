cmake_minimum_required(VERSION 2.6)
project(bga)

set(CMAKE_BUILD_TYPE Debug)
cmake_policy(SET CMP0015 OLD)	# relative link paths

if(WIN32)
	set(PLATFORM "win32")
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-subsystem,windows")
else()
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wreturn-type")	#warn on no return
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")	#warnings
endif()


if(NOT DEFINED PLATFORM)
	#message(FATAL_ERROR "Platform not defined")
	set(PLATFORM "linux64")
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM})

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/${PLATFORM}/lib)
set(LOCAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/${PLATFORM}/include)

#SFML
#set(DEP_SFML_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/${PLATFORM}/SFML-2.3.2" CACHE STRING "SFML root dir")
set(DEP_SFML_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/deps/${PLATFORM}/SFML-2.1" CACHE STRING "SFML root dir")
set(DEP_SFML_INCLUDE_DIR "${DEP_SFML_ROOT}/include")
set(DEP_SFML_LIB_DIR "${DEP_SFML_ROOT}/lib")

if("${PLATFORM}" STREQUAL "linux32")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set(LOCAL_LIBS
		${DEP_SFML_LIB_DIR}/libsfml-audio.so
		${DEP_SFML_LIB_DIR}/libsfml-graphics.so
		${DEP_SFML_LIB_DIR}/libsfml-system.so
		${DEP_SFML_LIB_DIR}/libsfml-window.so

		${LIB_DIR}/libGLEW.so.1.10
	)
elseif("${PLATFORM}" STREQUAL "linux64")
	set(LOCAL_LIBS
		libsfml-audio.so
		libsfml-graphics.so
		libsfml-system.so
		libsfml-window.so

		libGLEW.so
	)
elseif("${PLATFORM}" STREQUAL "win32")
	set(LOCAL_LIBS
		${DEP_SFML_LIB_DIR}/sfml-audio.lib
		${DEP_SFML_LIB_DIR}/sfml-graphics.lib
		${DEP_SFML_LIB_DIR}/sfml-system.lib
		${DEP_SFML_LIB_DIR}/sfml-window.lib

		#${LIB_DIR}/libGLEW.so.1.10
	)
endif()

include_directories(
	src
	${INCLUDE_DIR}
	${LOCAL_INCLUDE_DIR}
	${DEP_SFML_INCLUDE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/ext
)

add_subdirectory(ext)
add_subdirectory(src)
add_subdirectory(test)

#add_executable(ship_viewer ship_viewer.cpp)
#target_link_libraries(ship_viewer osteo)

#add_executable(test test.cpp)
#target_link_libraries(test lbga)



