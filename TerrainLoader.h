#ifndef _TerrainIsland_H_
#define _TerrainIsland_H_

#include <SFML/Graphics.hpp>
#include <SFML/System.hpp>

class TerrainIsland;

class TerrainLoaderTask {
public:
	virtual void execute()=0;
	virtual ~TerrainLoaderTask() {}
};

/*
class TerrainLoaderTask {
public:
	TerrainIsland* island;
	sf::IntRect update_rect;

	TerrainLoaderTask();
	TerrainLoaderTask(TerrainIsland* _island,sf::IntRect _rect) {
		island=_island;
		update_rect=_rect;
	}
};
*/

class TerrainLoader {

	sf::Thread thread;
	sf::Mutex mutex;
	std::vector<TerrainLoaderTask*> tasks;

	void entry();

public:

	TerrainLoader() : thread(&TerrainLoader::entry,this) {
		thread.launch();
	}
	void add_task(TerrainLoaderTask *task) {
		mutex.lock();
		tasks.push_back(task);
		mutex.unlock();
	}

};

#endif
