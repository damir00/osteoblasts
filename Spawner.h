#ifndef _Spawner_H_
#define _Spawner_H_

#include "Utils.h"

#include <vector>

class Game;
class Ship;

class SpawnerWave {
public:

	int num_ships;
	long next_ship_ts;
	SpawnWave wave;
};

class Spawner {
	Game* game;

	int current_segment_i;

	std::vector<SpawnerWave> current_waves;

	void wave_start(SpawnWave& wave);
	//void wave_spawn();
	void update_waves();

public:
	long ts;

	SpawnData data;

	Spawner(Game* _game);
	void start();
	void update(long delta);
};

#endif
