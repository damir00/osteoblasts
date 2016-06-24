#ifndef _Anim_H_
#define _Anim_H_

#include <SFML/Graphics.hpp>
#include <vector>

#include "Node.h"

class SpriteAnimData {
public:
	sf::Texture* texture;
	std::vector<sf::IntRect> frames;
	int delay;

	std::vector<int> sections;
	int frame_start;
	int frame_end;

	float scale;
};

class SpriteAnim : public Node {
	SpriteAnimData* anim;
	sf::Sprite sprite;
	int prev_frame;

	void update_to_frame(int frame);

public:
	float pos;
	float duration;

	bool done;
	bool loop;
	bool reverse;

	SpriteAnim();
	SpriteAnim(SpriteAnimData* _anim,bool _loop=false);
	void load(SpriteAnimData* _anim,bool _loop=false);
	void update(float delta);
	void set_frame(int frame);
	void set_progress(float progress);
	void set_offset(sf::Vector2f offset);
	int get_frame();

	void draw(const NodeState& state);
	sf::Vector2f get_size();

	bool loaded();
};

class AtlasItem {
public:
	sf::Vector2i pos;
	sf::Vector2i size;
	sf::Vector2i map_pos;
};
class Atlas {
public:
	sf::Texture* texture;
	std::vector<AtlasItem> items;

	Atlas();
	sf::Sprite get_sprite(int i);
};

#endif
