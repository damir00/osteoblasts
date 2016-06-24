cmake_minimum_required(VERSION 2.6)
project(Osteoblasts)

set(CMAKE_BUILD_TYPE Debug)
cmake_policy(SET CMP0015 OLD)	# relative link paths
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wreturn-type")	#warn on no return

if(NOT DEFINED PLATFORM)
	message(FATAL_ERROR "Platform not defined")
endif()


set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM})

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/${PLATFORM}/lib)
set(LOCAL_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/${PLATFORM}/include)

if(${PLATFORM} STREQUAL "linux32")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
	set(LOCAL_LIBS
		${LIB_DIR}/libsfml-audio.so.2
		${LIB_DIR}/libsfml-graphics.so.2
		${LIB_DIR}/libsfml-network.so.2
		${LIB_DIR}/libsfml-system.so.2
		${LIB_DIR}/libsfml-window.so.2
		${LIB_DIR}/libGLEW.so.1.10
		${LIB_DIR}/libnoise.so.0
		${LIB_DIR}/libjsoncpp.a
	)
elseif(${PLATFORM} STREQUAL "win32")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mwindows")
	set(LOCAL_LIBS
		${LIB_DIR}/sfml-audio-2.dll
		${LIB_DIR}/sfml-graphics-2.dll
		${LIB_DIR}/sfml-network-2.dll
		${LIB_DIR}/sfml-system-2.dll
		${LIB_DIR}/sfml-window-2.dll
		#${LIB_DIR}/libGLEW.so.1.10
		${LIB_DIR}/libnoise.dll
		${LIB_DIR}/libjsoncpp.a
	)
endif()

include_directories(
	${INCLUDE_DIR}
	${LOCAL_INCLUDE_DIR}
	${LOCAL_INCLUDE_DIR}/SFML
	${LOCAL_INCLUDE_DIR}/libnoise
	#${LOCAL_INCLUDE_DIR}/json
	#${GLIB_INCLUDE}
)

add_library(lbtga SHARED
	osteoblasts.cpp
	BGManager.cpp
	GameRes.cpp
	Node.cpp
	Utils.cpp
	Terrain.cpp
	TerrainLoader.cpp
	Spawner.cpp
	Game.cpp
	Ship.cpp
	Anim.cpp
	Parser.cpp
	Audio.cpp
	Menu.cpp
)
target_link_libraries(lbtga ${LOCAL_LIBS})

add_executable(bgta osteoblasts.cpp)
target_link_libraries(bgta lbtga)

#add_executable(ship_viewer ship_viewer.cpp)
#target_link_libraries(ship_viewer osteo)

add_executable(test test.cpp)
target_link_libraries(test lbtga)



