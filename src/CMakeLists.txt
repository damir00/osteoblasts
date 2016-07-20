add_library(lbga
	#framework
	Framework.cpp
	Loader.cpp
	Utils.cpp
	Node.cpp
	Menu.cpp

	#BGA
	SpaceBackground.cpp
	MenuMain.cpp
	Terrain.cpp
	TerrainLoader.cpp
	Game.cpp
)
target_link_libraries(lbga
	noise
	${LOCAL_LIBS}
)

add_executable(bga bga.cpp)
target_link_libraries(bga lbga)

