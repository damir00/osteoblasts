#include "Game.h"


CompDisplay::CompDisplay(Entity* e) : Component(e) {
	e->node_main.add_child(&root_node);
}
