#include "Parser.h"
#include "GameRes.h"
#include "Spawner.h"

#include <fstream>
#include <string>
#include <utility>

#include <assert.h>

using namespace std;

sf::Vector2f parse_vec(const Json::Value& value,sf::Vector2f def=sf::Vector2f(0,0)) {
	if(value==Json::nullValue) {
		return def;
	}
	return sf::Vector2f(
			value.get("x",def.x).asInt(),
			value.get("y",def.y).asInt());
}

std::vector<std::string> parse_string_array(const Json::Value& value) {
	std::vector<std::string> arr;
	if(value.isArray()) {
		for(int i=0;i<value.size();i++) {
			arr.push_back(value[i].asString());
		}
	}
	return arr;
}

void parse_range(const Json::Value& value,Range& range) {
	if(value==Json::nullValue) return;
	if(value.isObject()) {
		range.low=value.get("low",range.low).asFloat();
		range.high=value.get("high",range.high).asFloat();
	}
	else {
		if(value.isNumeric()) {
			range.set_fixed(value.asFloat());
		}
	}
}
void parse_range_locked(const Json::Value& value,RangeLocked& range) {
	if(value==Json::nullValue) return;
	parse_range(value,range);
	if(value.isObject()) {
		range.locked=value.get("locked",range.locked).asBool();
	}
}
Range parse_range(const Json::Value& value,float low=0,float high=0) {
	Range r(low,high);
	parse_range(value,r);
	return r;
}
RangeLocked parse_range_locked(const Json::Value& value,float low=0,float high=0,bool locked=false) {
	RangeLocked r(low,high);
	r.locked=locked;
	parse_range_locked(value,r);
	return r;
}

ShipGunAimType parse_aim_type(const Json::Value& value) {
	string t=value.asString();
	if(t=="fixed") return GUN_AIM_TYPE_FIXED;
	if(t=="spiral") return GUN_AIM_TYPE_SPIRAL;
	return GUN_AIM_TYPE_AIM;
}
SpawnWave::Pattern parse_spawn_pattern(const Json::Value& value) {
	string t=value.asString();
	return SpawnWave::RANDOM;
}

Json::Value ParserJSON::read_file(string filename) {

	ifstream file(filename.c_str());
	if(!file.is_open()) {
		printf("Can't open file %s\n",filename.c_str());
		assert(0);
		return Json::nullValue;
	}
	Json::Reader r;
	Json::Value root;
	if(!r.parse(file,root)) {
		file.close();
		printf("Can't parse JSON file %s\n",filename.c_str());
		printf("%s\n",r.getFormattedErrorMessages().c_str());
		return Json::nullValue;
	}
	file.close();
	return root;
}

bool ParserJSON::parse_anim_info(const Json::Value& root,int &frame_w,int &frame_h,int &delay_ms,std::vector<int>& sections) {
	if(root.isNull()) return false;
	if(root["frame_w"].isInt() &&
		root["frame_h"].isInt() &&
		root["delay_ms"].isInt()) {

		frame_w=root["frame_w"].asInt();
		frame_h=root["frame_h"].asInt();
		delay_ms=root["delay_ms"].asInt();

		Json::Value j_sections=root["sections"];
		if(j_sections.isArray()) {
			for(int i=0;i<j_sections.size();i++) {
				sections.push_back(j_sections[i].asInt());
			}
		}

		return true;
	}
	return false;
}
bool ParserJSON::parse_anim_info(std::string filename,int &frame_w,int &frame_h,int &delay_ms,std::vector<int>& sections) {
	Json::Value root=read_file(filename);
	return parse_anim_info(root,frame_w,frame_h,delay_ms,sections);
}
Atlas* ParserJSON::parse_atlas(const Json::Value& root) {
	if(!root.isArray()) {
		return NULL;
	}
	Atlas* a=new Atlas();
	for(int i=0;i<root.size();i++) {
		try {
			Json::Value val=root[i];
			AtlasItem item;
			item.pos.x=val["x"].asInt();
			item.pos.y=val["y"].asInt();
			item.size.x=val["w"].asInt();
			item.size.y=val["h"].asInt();
			item.map_pos.x=val["map_x"].asInt();
			item.map_pos.y=val["map_y"].asInt();
			a->items.push_back(item);
		}
		catch(...) { }
	}
	return a;
}


Ship* ParserJSON::parse_ship(string id) {

	Json::Value root=read_file(id+".json");
	if(root.isNull()) {
		printf("Can't load ship %s\n",id.c_str());
		return NULL;
	}
	return parse_ship(root);
}
Ship* ParserJSON::parse_ship(Json::Value& root) {
	string ship_type=root.get("type","").asString();

	static pair<string,Ship*> ship_types[]={
			make_pair("player",new Player()),
			make_pair("drone",new Drone()),
			make_pair("kamikaze",new EnemyKamikaze()),
			make_pair("melee",new EnemyMelee()),
			make_pair("boss_platypus",new BossPlatypus()),
			make_pair("boss_walrus",new BossWalrus()),
			make_pair("boss_luna",new BossLuna()),
			make_pair("boss_octopuss",new BossOctopuss()),
			make_pair("null",(Ship*)NULL)
	};

	Ship* ship=NULL;
	for(int i=0;ship_types[i].second;i++) {
		if(ship_type==ship_types[i].first) {
			ship=ship_types[i].second->clone();
		}
	}
	if(!ship) {
		ship=new Ship();
		ship->set_controller(new ControllerShooter());
	}

	/*
	if(ship_type=="player") {
		ship=new Player();
	}
	else if(ship_type=="drone") {
		ship=new Drone();
	}
	else if(ship_type=="kamikaze") {
		ship=new EnemyKamikaze();
	}
	else if(ship_type=="melee") {
		ship=new EnemyMelee();
	}
	else if(ship_type=="boss_platypus") {
		ship=new BossPlatypus();
	}
	else if(ship_type=="boss_walrus") {
		ship=new BossWalrus();
	}
	else {
		ship=new Ship();
		ship->set_controller(new ControllerShooter());
	}
	*/

	//generic props

	//hull
	Json::Value hull=root["hull"];
	if(!hull.isNull()) {
		string hull_type=hull.get("type","").asString();
		string hull_id=hull["id"].asString();

		if(hull_type=="anim") {
			ship->set_anim(GameRes::get_anim(hull_id),false);
		}
		else if(hull_type=="anim_fire") {
			ship->set_anim(GameRes::get_anim(hull_id),true);
			ship->switch_anim_fire.init(false,0,40);
		}
		else {
			ship->set_texture(GameRes::get_texture(hull_id));
		}
	}

	Json::Value fire_pattern=root["fire_pattern"];
	if(!fire_pattern.isNull()) {
		float burst_pause=fire_pattern.get("pause1",100).asFloat();
		float bullet_pause=fire_pattern.get("pause2",100).asFloat();
		float bullet_count=fire_pattern.get("count",1).asInt();
		ship->fire_timeout.set(burst_pause,bullet_pause,bullet_count);
	}


	ship->size=parse_vec(root["size"],sf::Vector2f(50,50));
	ship->health=root.get("health",10).asInt();
	ship->max_acceleration=root.get("acceleration",0.002).asFloat();
	ship->max_speed=root.get("speed",5).asFloat();

	Json::Value guns=root["guns"];
	if(guns.isArray()) {
		for(int i=0;i<guns.size();i++) {
			ShipGun gun;
			if(parse_ship_gun(guns[i],gun)) {
				ship->guns.push_back(gun);
			}
		}
	}

	return ship;
}
bool ParserJSON::parse_ship_gun(Json::Value &root,ShipGun& gun) {

	gun.pos=parse_vec(root["position"]);
	gun.lat_spread=root.get("lat_spread",0).asFloat();
	gun.angle=Utils::deg_to_rad(root.get("angle",0).asFloat());
	gun.angle_spread=Utils::deg_to_rad(root.get("angle_spread",0).asFloat());
	gun.aim_type=parse_aim_type(root["type"]);
	gun.spiral_speed=Utils::deg_to_rad(root.get("spiral_speed",0).asFloat());
	gun.use_master_timer=root.get("use_master_timer",true).asBool();
	gun.bullet_speed=root.get("bullet_speed",1).asFloat();
	gun.bullet_texture=GameRes::get_texture("hero/projectile_2.png");

	return true;
}


bool ParserJSON::parse_ship_groups(std::string filename,std::tr1::unordered_map<std::string, std::vector<Ship*> >& map) {
	Json::Value root=read_file(filename);
	if(root.isNull()) {
		return false;
	}

	Json::Value defs=root["defs"];

	for(int i=0;i<defs.getMemberNames().size();i++) {
		Ship* ship=parse_ship(defs[defs.getMemberNames()[i]]);
		if(ship) {
			GameRes::insert_ship(defs.getMemberNames()[i],ship);
		}
	}
	/*
	Json::Value anim_defs=root["anims"];
	for(int i=0;i<anim_defs.getMemberNames().size();i++) {
		SpriteAnimData data;
		parse_anim_data()
	}
	*/


	Json::Value groups=root["groups"];

	for(int i=0;i<groups.getMemberNames().size();i++) {
		std::string name=groups.getMemberNames()[i];

		std::vector<Ship*>& vec=map[name];

		Json::Value arr=groups[name];
		if(arr.isArray()) {
			for(int a=0;a<arr.size();a++) {
				Ship* s=GameRes::get_ship(arr[a].asString());
				if(s) {
					vec.push_back(s);
				}
			}
		}
	}

	return true;
}

GameConf ParserJSON::parse_game_conf(std::string filename) {
	GameConf conf;

	Json::Value root=read_file(filename);
	if(root.isNull()) {
		return conf;
	}

	Json::Value json_segments=root["segments"];
	if(json_segments.isArray()) {
		for(int i=0;i<json_segments.size();i++) {
			Json::Value json_segment=json_segments[i];

			SpawnSegment segment;
			segment.start_ts=json_segment.get("start",0).asInt();
			segment.end_ts=json_segment.get("end",0).asInt();

			Json::Value json_waves=json_segment["waves"];
			if(json_waves.isArray()) {
				for(int w=0;w<json_waves.size();w++) {

					Json::Value json_wave=json_waves[w];

					SpawnWave wave;

					wave.groups=parse_string_array(json_wave["groups"]);
					wave.ships=parse_string_array(json_wave["ships"]);

					wave.mult_enemy_damage=parse_range_locked(json_wave["mult_enemy_damage"],1,1,false);
					wave.mult_enemy_fire_speed=parse_range_locked(json_wave["mult_enemy_fire_speed"],1,1,false);
					wave.mult_enemy_hp=parse_range_locked(json_wave["mult_enemy_hp"],1,1,false);
					wave.mult_enemy_speed=parse_range_locked(json_wave["mult_enemy_speed"],1,1,false);
					wave.pattern=parse_spawn_pattern(json_wave["pattern"]);
					wave.spawn_delay=parse_range_locked(json_wave["spawn_delay"],500,500,false);
					wave.ship_count=parse_range(json_wave["ship_count"],0,0);
					wave.start=parse_range(json_wave["start"],0,0);

					segment.waves.push_back(wave);
				}
			}

			conf.segments.segments.push_back(segment);

		}
	}

	return conf;
}




