#include "Menu.h"
#include "Utils.h"
#include "GameRes.h"

Menu::Menu() {
	menu_master=NULL;
	state_hover=false;
}
Menu::~Menu() {

}
void Menu::frame(float delta) {
	if(!visible) {
		return;
	}

	menu_frame(delta);
	events.clear();

	for(int i=0;i<items.size();i++) {
		items[i]->frame(delta);
	}
}
void Menu::resize(sf::Vector2f _size) {
//	printf("menu resize %f %f\n",_size.x,_size.y);

	if(_size==size) {
		return;
	}

	size=_size;
	for(int i=0;i<items.size();i++) {
		items[i]->resize(size);
	}
	on_resized();
}
void Menu::push_event(Event e) {
	events.push_back(e);
}
Event Menu::get_event() {
	return Event();
}
void Menu::set_enabled(bool enabled) {
	visible=enabled;
}
bool Menu::is_enabled() {
	return visible;
}
void Menu::add_item(Menu* item) {
	item->menu_master=this;
	items.push_back(item);
	add_child(item);
}
void Menu::remove_item(Menu* item) {
	if(Utils::vector_remove(items,item)) {
		item->menu_master=NULL;
		item->remove();
	}
}
void Menu::show_page(Menu* item) {
	for(int i=0;i<items.size();i++) {
		items[i]->set_enabled(items[i] == item);
	}
}
sf::Vector2f Menu::get_master_size() {
	if(menu_master) {
		return menu_master->size;
	}
	return size;
}

void Menu::set_pos(sf::Vector2f _pos) {
	pos=_pos;
}
void Menu::set_pos_size(sf::Vector2f _pos,sf::Vector2f _size) {
	set_pos(_pos);
	size=_size;
}
bool Menu::point_inside(sf::Vector2f p) {
	//return (p.x>=pos.x && p.x<=pos.x+size.x && p.y>=pos.y && p.y<=pos.y+size.y);
	return (p.x>=0 && p.x<=size.x && p.y>=0 && p.y<=size.y);
}
bool Menu::event_inside(sf::Event::MouseButtonEvent event) {
	return point_inside(sf::Vector2f(event.x,event.y));
}
bool Menu::event_inside(sf::Event::MouseMoveEvent event) {
	return point_inside(sf::Vector2f(event.x,event.y));
}

sf::Vector2f event_get_pos(const sf::Event& e) {
	switch(e.type) {
	case sf::Event::MouseButtonPressed:
	case sf::Event::MouseButtonReleased:
		return sf::Vector2f(e.mouseButton.x,e.mouseButton.y);
	case sf::Event::MouseMoved:
		return sf::Vector2f(e.mouseMove.x,e.mouseMove.y);
	}
	return sf::Vector2f(0,0);
}

void Menu::event(sf::Event event) {
	if(!is_enabled()) {
		return;
	}

	sf::Vector2f event_pos=event_get_pos(event);
	event_pos.x=(event_pos.x-pos.x)/scale.x;
	event_pos.y=(event_pos.y-pos.y)/scale.y;

	switch(event.type) {
	case sf::Event::MouseButtonPressed:
	case sf::Event::MouseButtonReleased:
		event.mouseButton.x=event_pos.x;
		event.mouseButton.y=event_pos.y;
		break;
	case sf::Event::MouseMoved:
		event.mouseMove.x=event_pos.x;
		event.mouseMove.y=event_pos.y;
	}
	for(int i=0;i<items.size();i++) {
		items[i]->event(event);
	}

	switch(event.type) {
	case sf::Event::MouseButtonReleased:
		if(event.mouseButton.button==sf::Mouse::Left && event_inside(event.mouseButton)) {
			on_clicked(sf::Vector2f(event.mouseButton.x,event.mouseButton.y));

			if(state_hover) {
				state_hover=false;
				on_hover(false);
			}
		}
		break;
	case sf::Event::MouseMoved:
		if(!state_hover && event_inside(event.mouseMove)) {
			state_hover=true;
			on_hover(true);
		}
		else if(state_hover && !event_inside(event.mouseMove)) {
			state_hover=false;
			on_hover(false);
		}
		break;
	case sf::Event::KeyPressed:
		on_key_pressed(event.key.code);
		break;
	}
	on_event(event);
}


void Menu::message_send(int msg) {
	if(menu_master) {
		menu_master->message(msg);
	}
}
void Menu::message(int msg) {
	if(on_message(msg)) {
		return;
	}
	message_send(msg);
}


