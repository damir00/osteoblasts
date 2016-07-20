#ifndef _BGA_SPACE_BACKGROUND_H_
#define _BGA_SPACE_BACKGROUND_H_

#include <vector>

#include <SFML/Graphics.hpp>

#include "Node.h"

class SpaceBackground : public Node {
	class Layer;

	Node::Ptr bg;
public:
	std::vector<Layer*> layers;

	SpaceBackground();
	~SpaceBackground();
	void add_layer(std::vector<Texture> bitmaps,float mult,float spacing);
	void update(sf::FloatRect area);
	void set_background_visible(bool visible);

	void create_default();
};

#endif
