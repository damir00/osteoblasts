#include "Game.h"

/*
CompDisplay::CompDisplay(Entity* e) : Component(e) {
	e->node_main.add_child(&root_node);
}

CompEngine::CompEngine(Entity* e) : Component(e) {
	entity->node_main.add_child(&node);
}
*/
void CompDisplay::remove() {
	entity->node_main.remove_child(&root_node);
}
void CompEngine::remove() {
	entity->node_main.remove_child(&node);
}

