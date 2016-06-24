#include "TerrainLoader.h"
#include "Terrain.h"

void TerrainLoader::entry() {
	while(true) {
		mutex.lock();
		if(tasks.size()==0) {
			mutex.unlock();
			sf::sleep(sf::milliseconds(10));
			continue;
		}
		TerrainLoaderTask* task=tasks.front();
		tasks.erase(tasks.begin());
		mutex.unlock();

		//printf("loader task..\n");

		//task.island->update_area(task.update_rect);
		task->execute();
		delete(task);
	}
}
