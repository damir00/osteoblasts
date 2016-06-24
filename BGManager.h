#ifndef _BGManager_H_
#define _BGManager_H_

#include <SFML/Graphics.hpp>

#include "Node.h"

#include <tr1/unordered_map>

class BGSpawnerItem {
public:
	sf::Vector2f pos;
	sf::Sprite* sprite;
};

class BGSpawner {

	sf::IntRect current_area;

	/*
	var items_arr:Array<BGSpawnerItem>;
	var items:Map<String,DisplayObject>;
	*/
	std::tr1::unordered_map<std::string,BGSpawnerItem*> items_map;

	//var item_count:Int;

	float border;
	float spacing;

	std::string item_hash(int x,int y);
	sf::IntRect norm_rect(sf::Vector2f p,sf::FloatRect rect);

public:
	sf::Vector2f offset;
	std::vector<sf::Texture*> bitmaps;

	void set_spacing(float spacing);
	float get_spacing();

	BGSpawner(float border_size,float _spacing);
	void frame(sf::Vector2f pos,sf::FloatRect area);
	void draw(const NodeState& state);
};

class BGManagerLayer {
public:
	BGSpawner spawner;
	float mult;
	BGManagerLayer(float spacing);
};

class BGManager : public Node {
	NodeContainer* bg;

public:
	std::vector<BGManagerLayer> layers;
	bool background_color;

	BGManager();
	void frame(sf::Vector2f pos,sf::FloatRect area);
	void draw(const NodeState& state);

	void add_layer(std::vector<sf::Texture*> bitmaps,float mult,float spacing);
};

#endif

