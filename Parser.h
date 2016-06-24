#ifndef _PARSER_H_
#define _PARSER_H_

#include "Ship.h"
#include "Utils.h"

#include <string>
#include <stdio.h>
#include <json/reader.h>

#include <tr1/unordered_map>

class ParserJSON {

	static bool parse_ship_gun(Json::Value &root,ShipGun& gun);
	static Ship* parse_ship(Json::Value& root);
public:
	static Json::Value read_file(std::string filename);

	static Ship* parse_ship(std::string id);
	static bool parse_anim_info(const Json::Value& root,int &frame_w,int &frame_h,int &delay_ms,std::vector<int>& sections);
	static bool parse_anim_info(std::string filename,int &frame_w,int &frame_h,int &delay_ms,std::vector<int>& sections);
	static Atlas* parse_atlas(const Json::Value& root);
	static bool parse_ship_groups(std::string filename,std::tr1::unordered_map<std::string, std::vector<Ship*> >& map);
	static GameConf parse_game_conf(std::string filename);
};

#endif
