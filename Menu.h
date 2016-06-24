#ifndef _Menu_H_
#define _Menu_H_

#include "Node.h"
#include <vector>

class Event {
public:

	enum EventType {
		EVENT_NONE
	};

	EventType type;
	int x;
	int y;
	int key;
	bool down;
};

class Menu : public NodeSprite {
public:
	Menu* menu_master;
	sf::Vector2f size;

	bool state_hover;

	Menu();
	virtual ~Menu();

	void frame(float delta);
	//virtual void draw(sf::RenderTarget& target, const sf::RenderStates& state) {}

	void push_event(Event event);
	Event get_event();

	void set_enabled(bool enabled);
	bool is_enabled();
	void add_item(Menu* item);
	void remove_item(Menu* item);
	void show_page(Menu* item);
	virtual void resize(sf::Vector2f _size);
	sf::Vector2f get_master_size();
	void event(sf::Event event);
	bool event_inside(sf::Event::MouseButtonEvent event);
	bool event_inside(sf::Event::MouseMoveEvent event);
	bool point_inside(sf::Vector2f pos);

	void message_send(int msg);
	void message(int msg);

	void set_pos(sf::Vector2f _pos);
	void set_pos_size(sf::Vector2f _pos,sf::Vector2f _size);
	sf::Vector2f scaled_size() { return sf::Vector2f(size.x*scale.x,size.y*scale.y); }

private:
	bool enabled;
	std::vector<Event> events;
	std::vector<Menu*> items;

	virtual void menu_frame(float delta) {}
	virtual void on_resized() {}
	virtual void on_event(sf::Event event) {}
	virtual void on_clicked(sf::Vector2f mouse_pos) {}
	virtual void on_hover(bool hover) {}
	virtual void on_key_pressed(sf::Keyboard::Key key) {}

	virtual bool on_message(int msg) { return false; } //true if consumed
};


#endif

