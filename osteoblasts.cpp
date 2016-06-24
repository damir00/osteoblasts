#include <stdio.h>

#include <sstream>

#include <SFML/Graphics.hpp>

#include "GameRes.h"
#include "Node.h"
#include "Game.h"
#include "Audio.h"

#include "Parser.h"

#include "Menu.h"
/*
//UI
class UISlider;

class UIDelegate {
public:
	UIDelegate() {}
	virtual ~UIDelegate() {}
	virtual void slider_changed(UISlider* slider) {}
};

class UIItem : public Node {
public:
	sf::Vector2f size;
	UIDelegate* delegate;
	UIItem() {
		delegate=NULL;
	}
	virtual ~UIItem() {}
	sf::Vector2f point_to_local(float x,float y) {
		sf::Vector2f p=render_state.transform.transformPoint(0,0);
		return sf::Vector2f(x-p.x,y-p.y);
	}
	bool point_inside(float x,float y) {
		sf::Vector2f p=point_to_local(x,y);
		return (p.x>=0 && p.y>=0 && p.x<=size.x && p.y<=size.y);
	}

};

class UILabel : public UIItem {
public:
	sf::Text text;

	UILabel() {
		text.setFont(GameRes::font);
		text.setCharacterSize(12);
		text.setColor(sf::Color(255,0,0));
	}
	UILabel(std::string str,int size=12) {
		text.setFont(GameRes::font);
		text.setString(str);
		text.setCharacterSize(size);
	}
	virtual ~UILabel() {};

	void draw(sf::RenderTarget& target,const sf::RenderStates& state) {
		target.draw(text,state);
	}
};

class UISlider : public UIItem {
	sf::RectangleShape shape_outline;
	sf::RectangleShape shape_fill;
	bool pressed;

	UILabel label;

	void calc_value(float x) {
		value=min+point_to_local(x,0).x/size.x*(max-min);
	}
public:

	float min,max;
	float value;

	UISlider() {
		min=0;
		max=1;
		value=0.5;

		size=sf::Vector2f(200,15);
		shape_outline.setSize(size);
		shape_outline.setOutlineColor(sf::Color(255,0,0));
		shape_fill.setFillColor(sf::Color(255,0,0));

		update_size();

		label.text.setColor(sf::Color::White);
		label.render_state.transform.translate(220,0);
		childs.push_back(&label);
	}
	void update_size() {
		shape_fill.setSize(sf::Vector2f(size.x*get_percent_value(),size.y));
		label.text.setString(Utils::to_string(value));
	}

	float get_percent_value() {
		return (value-min)/(max-min);
	}

	void mouse_moved(sf::Event event) {
		if(!pressed || !point_inside(event.mouseMove.x,event.mouseMove.y)) return;
		calc_value(event.mouseMove.x);
		update_size();
		delegate->slider_changed(this);
	}
	void mouse_clicked(sf::Event event,bool on) {
		if(!on) {
			pressed=false;
			return;
		}
		if(!point_inside(event.mouseButton.x,event.mouseButton.y)) return;
		pressed=on;
		if(pressed) {
			calc_value(event.mouseButton.x);
			update_size();
			delegate->slider_changed(this);
		}
	}
	void draw(sf::RenderTarget& target,const sf::RenderStates& state) {
		target.draw(shape_outline,state);
		target.draw(shape_fill,state);
	}
};


//tweaker
class Tweaker : public UIItem, public UIDelegate {
	UISlider* slider_player_size;
	UISlider* slider_player_speed;
	UISlider* slider_player_acc;
	std::vector<UISlider*> sliders_bg;

	Game* game;

	sf::Vector2f widget_pos;

public:
	Tweaker(Game* _game) {
		game=_game;
		widget_pos=sf::Vector2f(30,100);

		add_label("Player:");
		tab(1);
		slider_player_size=add_slider("size",0.1,2,game->player_scale);
		slider_player_speed=add_slider("speed",0.1,50,game->player->max_speed);
		slider_player_acc=add_slider("acceleration",0.0001,0.1,game->player->max_acceleration);
		tab(-1);


		add_label("Background layers:");
		tab(1);
		for(int i=0;i<game->bg->layers.size();i++) {

			std::ostringstream ss;

			ss<<(i+1)<<" spacing";
			sliders_bg.push_back(add_slider(ss.str(),10,2000,game->bg->layers[i].spawner.get_spacing()));

			ss.str("");
			ss<<(i+1)<<" speed";
			sliders_bg.push_back(add_slider(ss.str(),0,1,game->bg->layers[i].mult));
		}
		tab(-1);
	}

	UISlider* add_slider(std::string label_str,float min,float max,float def=1.0) {
		UISlider* slider=new UISlider();
		slider->min=min;
		slider->max=max;
		slider->value=def;
		slider->update_size();

		UILabel* label=new UILabel(label_str);
		add_item(slider,label);

		return slider;
	}
	void add_label(std::string text) {
		UILabel* label=new UILabel(text);
		add_item(label);
	}
	void tab(int x) {
		widget_pos.x+=x*20;
	}

	void add_item(UIItem* item,UIItem* item2=NULL) {
		item->delegate=this;
		item->render_state.transform.translate(widget_pos);

		if(item2) {
			item2->delegate=this;
			item2->render_state.transform.translate(widget_pos);
			item->render_state.transform.translate(100,0);
			childs.push_back(item2);
		}

		childs.push_back(item);

		widget_pos.y+=30;
	}
	void slider_changed(UISlider* slider) {
		if(slider==slider_player_size)	game->player_scale=slider->value;
		if(slider==slider_player_speed)	game->player->max_speed=slider->value;
		if(slider==slider_player_acc)	game->player->max_acceleration=slider->value;
		for(int i=0;i<game->bg->layers.size();i++) {
			if(slider==sliders_bg[i*2+0]) game->bg->layers[i].spawner.set_spacing(slider->value);
			if(slider==sliders_bg[i*2+1]) game->bg->layers[i].mult=slider->value;
		}
	}
};


class ShipSelectorItem : public UIItem {
public:
	ShipSelectorItem(Ship* _ship) {
		ship=_ship;
		add_child(ship);
	}
	Ship* ship;
};

class ShipSelector : public UIItem, public UIDelegate {
	Game* game;

	std::vector<ShipSelectorItem*> items;

	void add_ship(Ship* s) {

		ShipSelectorItem* item=new ShipSelectorItem(s);

		sf::Vector2f widget_size(150,150);

		int cell_x=items.size()/3;
		int cell_y=items.size()%3;

		item->render_state.transform.translate(25+cell_x*widget_size.x,25+cell_y*widget_size.y);
		s->render_state.transform.translate(widget_size*0.5f);
		item->size=widget_size;
		items.push_back(item);
		add_child(item);
	}

public:
	ShipSelector(Game* _game) {
		game=_game;

		static const char* ships[]={
				"boss_platypus",
				"boss_walrus",
				"boss_luna",
				"boss_octopuss",
				"dummy",
				NULL
		};

		for(int i=0;ships[i];i++) {
			add_ship(GameRes::get_ship(ships[i]));
		}
	}
	void mouse_clicked(sf::Event event,bool on) {
		if(!on) {
			return;
		}
		printf("mouse click %d %d\n",event.mouseButton.x,event.mouseButton.y);
		for(int i=0;i<items.size();i++) {
			if(items[i]->point_inside(event.mouseButton.x,event.mouseButton.y)) {
				Ship* s=items[i]->ship->clone();
				s->pos=game->player->pos+sf::Vector2f(-200,0);
				game->spawn_ship(s);
				printf("Spawned ship %d\n",i);
				break;
			}
		}
	}

};

class MainMenuShip : public Ship {

};

class MainMenu : public UIItem, public UIDelegate {
	Ship* ship;
public:
	MainMenu() {
		ship=GameRes::get_ship("player");
		add_child(ship);
	}
};
*/
class GameMain {

	GameFramework* framework;
//	Menu* menu;

	/*
	std::vector<UIItem*> tweakers;
	std::vector<sf::Keyboard::Key> tweaker_keys;
	UIItem* tweaker_current;

	void add_tweaker(UIItem* tweaker,sf::Keyboard::Key key) {
		tweakers.push_back(tweaker);
		tweaker_keys.push_back(key);

		framework->game->node_root->add_child(tweaker);
		tweaker->visible=false;
	}
	void tweaker_toggle(UIItem* tweaker) {
		if(!tweaker_current) {
			tweaker->visible=true;
			tweaker_current=tweaker;
			framework->window.setMouseCursorVisible(tweaker_current);
			return;
		}
		if(tweaker_current==tweaker) {
			tweaker->visible=false;
			tweaker_current=NULL;
			framework->window.setMouseCursorVisible(tweaker_current);
			return;
		}
		tweaker_current->visible=false;
		tweaker->visible=true;
		tweaker_current=tweaker;
		framework->window.setMouseCursorVisible(tweaker_current);
	}
	void tweaker_event(sf::Event event) {
		if(event.type==sf::Event::KeyPressed) {
			for(int i=0;i<tweaker_keys.size();i++) {
				if(event.key.code==tweaker_keys[i]) {
					tweaker_toggle(tweakers[i]);
				}
			}
		}

		if(!tweaker_current) {
			return;
		}

		switch(event.type) {
			case sf::Event::MouseMoved:
				tweaker_current->event_mouse_moved(event);
				break;
			case sf::Event::MouseButtonPressed:
				tweaker_current->event_mouse_clicked(event,true);
				break;
			case sf::Event::MouseButtonReleased:
				tweaker_current->event_mouse_clicked(event,false);
				break;
		}
	}
	*/
public:

	GameMain() {
		//tweaker_current=NULL;
	}

	int run(int argc,char** argv) {

	#ifdef _WIN32
		freopen("osteolog.txt","w",stdout);
	#endif

		printf("BOOM Goes the Asteroid!\n");

		//game=Utils::init_game(1024,768, (argc>1 ? argv[1] : "assets/game.json" ) );
		GameFramework* framework=Utils::init_framework(1024,768);

		//game=framework->game;

		/*
		if(argc>=2 && (std::string)argv[1]=="-b") {
			sf::RectangleShape* blackout=new sf::RectangleShape(sf::Vector2f(10000,10000));
			blackout->setFillColor(sf::Color(0,0,0,240));
			game->node_root->childs.push_back(new NodeContainer(blackout));
		}
		*/

		//add_tweaker(new Tweaker(framework->game),sf::Keyboard::D);
		//add_tweaker(new ShipSelector(framework->game),sf::Keyboard::F);

		sf::Mouse::setPosition(framework->game_size/2,framework->window);
		framework->start_level(argc>1 ? argv[1] : "assets/game.json");

		while(framework->window.isOpen()) {
			framework->frame();
		}

		Audio::deinit();

		return 0;
	}
};

int main(int argc,char** argv) {
	GameMain g;
	return g.run(argc,argv);
}



