#include "Spawner.h"
#include "Game.h"
#include "Ship.h"
#include "Utils.h"
#include "Parser.h"
/*
void dbg_print_string_vec(int pad,std::string label,std::vector<std::string> vec) {
	for(int i=0;i<pad;i++) printf("\t");
	printf("%s:",label.c_str());
	for(int i=0;i<vec.size();i++) {
		printf(" %s",vec[i].c_str());
		if(i!=vec.size()-1) {
			printf(",");
		}
	}
	printf("\n");
}
void dbg_print_range(int pad,std::string label,Range range) {
	for(int i=0;i<pad;i++) printf("\t");
	printf("%s:",label.c_str());
	printf("%f-%f\n",range.low,range.high);
}
*/

Spawner::Spawner(Game* _game) {
	game=_game;
	current_segment_i=-1;

	data=ParserJSON::parse_game_conf(game->current_level).segments;
	/*
	printf("Spawner:\n");

	printf("%d segments: \n",data.segments.size());
	for(int i=0;i<data.segments.size();i++) {
		printf("\tstart: %ld\n",data.segments[i].start_ts);
		printf("\tend: %ld\n",data.segments[i].end_ts);
		printf("\t%d waves:\n",data.segments[i].waves.size());
		for(int w=0;w<data.segments[i].waves.size();w++) {
			printf("\t\tcount: %f-%f\n",data.segments[i].waves[w].ship_count.low,data.segments[i].waves[w].ship_count.high);
			dbg_print_string_vec(2,"ships",data.segments[i].waves[w].ships);
			dbg_print_string_vec(2,"groups",data.segments[i].waves[w].groups);
			dbg_print_range(2,"spawn_delay",data.segments[i].waves[w].spawn_delay);
		}
	}
	*/

}
void Spawner::start() {
	ts=0;
}

void Spawner::wave_start(SpawnWave &wave) {

	SpawnerWave spawner_wave;

	spawner_wave.wave=wave;

	spawner_wave.wave.mult_enemy_damage.check_lock();
	spawner_wave.wave.mult_enemy_fire_speed.check_lock();
	spawner_wave.wave.mult_enemy_hp.check_lock();
	spawner_wave.wave.mult_enemy_speed.check_lock();
	spawner_wave.wave.spawn_delay.check_lock();

	spawner_wave.num_ships=wave.ship_count.get_value_int();
	spawner_wave.next_ship_ts=ts+wave.start.get_value();

	printf("Started wave, %d ships\n",spawner_wave.num_ships);

	current_waves.push_back(spawner_wave);
}
void Spawner::update_waves() {

	for(int i=0;i<current_waves.size();i++) {

		SpawnerWave& w=current_waves[i];

		if(w.num_ships<=0) {
			current_waves.erase(current_waves.begin()+i);
			i--;
			continue;
		}
		if(w.next_ship_ts>ts) continue;

		Ship* ship=NULL;
		if(w.wave.ships.size()==0) {

			if(w.wave.groups.size()==0) {
				const char* types[]={"shooters","kamikaze","melee",0};
				int type=Utils::rand_range_i(0,2);
				ship=GameRes::ship_group_get_rand(types[type]);
			}
			else {
				ship=GameRes::ship_group_get_rand(Utils::vector_rand(w.wave.groups));
			}
		}
		else {
			ship=GameRes::get_ship(Utils::vector_rand(w.wave.ships));
		}

		if(ship) {
			//wave modifiers
			float speed_mult=w.wave.mult_enemy_speed.get_value();
			float fire_mult=w.wave.mult_enemy_fire_speed.get_value();

			ship->max_acceleration*=speed_mult;
			ship->max_speed*=speed_mult;
			ship->fire_timeout.fire_cooldown.start*=fire_mult;
			ship->fire_timeout.fire_delay.start*=fire_mult;
			ship->health*=w.wave.mult_enemy_hp.get_value();
			ship->damage_mult=w.wave.mult_enemy_damage.get_value();

			//position and spawn
			sf::Vector2f pos=game->player->pos+Utils::rand_angle_vec()*1000.0f;
			ship->pos=pos;
			ship->team=TEAM_ENEMY;
			game->spawn_ship(ship);
		}
		else {
			printf("No ship to spawn!\n");
		}
		w.num_ships--;
		w.next_ship_ts=ts+w.wave.spawn_delay.get_value();

	}
}

void Spawner::update(long delta) {

	ts+=delta;

	update_waves();

	if((current_segment_i<0 && data.segments.size()>0) || current_segment_i<data.segments.size()-1) {
		if(data.segments[current_segment_i+1].start_ts<ts) {
			current_segment_i++;
			SpawnSegment& segment=data.segments[current_segment_i];

			for(int i=0;i<segment.waves.size();i++) {
				wave_start(segment.waves[i]);
			}
		}

	}

	/*
	timeout-=delta;
	if(timeout<=0) {
		timeout+=1000;

		const char* types[]={"shooters","kamikaze","melee",0};

		int type=Utils::rand_range_i(0,2);

		Ship* ship=GameRes::ship_group_get_rand(types[type]);
		if(ship) {
			sf::Vector2f pos=game->player->pos+Utils::rand_angle_vec()*1000.0f;
			//sf::Vector2f pos=game->player->pos+sf::Vector2f(0,-100);
			ship->pos=pos;
			ship->team=TEAM_ENEMY;
			game->spawn_ship(ship);
		}
	}
	*/
}

