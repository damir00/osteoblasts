#include "GameRes.h"

#include "Utils.h"
#include "Game.h"

#include <string>

using namespace std;

int main(int argc,char** argv) {

	Game* game=Utils::init_game(800,600,"assets/game.json");

	for(int i=1;i<argc;i++) {
		string arg=argv[i];
		if(arg=="-b") {
			sf::RectangleShape* blackout=new sf::RectangleShape(sf::Vector2f(10000,10000));
			blackout->setFillColor(sf::Color(0,0,0,240));
			game->node_root->childs.push_back(new NodeContainer(blackout));
		}
		else {
			Ship* ship=GameRes::get_ship(arg);
			if(ship) {
				ship->pos=sf::Vector2f(1000,0);
				ship->game=game;
				ship->team=TEAM_ENEMY;
				game->player=(Player*)ship;
			}
		}
	}

	while(game->window.isOpen()) {

		sf::Event event;
		while(game->window.pollEvent(event)) {
			switch(event.type) {
			case sf::Event::Closed:
			        game->window.close();
				break;
			}
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
			game->window.close();
		}

		game->update();
	}


	return 0;
}
