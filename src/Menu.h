#ifndef _BGA_MENU_H_
#define _BGA_MENU_H_

#include <memory>
#include <vector>

#include <SFML/Window.hpp>

#include "Node.h"

class Menu;
class MenuEvent {
public:
	sf::Event event;
	sf::Vector2f pos;

	MenuEvent() {}
	MenuEvent(const sf::Event& e) {
		event=e;

		switch(e.type) {
		case sf::Event::MouseButtonPressed:
		case sf::Event::MouseButtonReleased:
			pos=sf::Vector2f(e.mouseButton.x,e.mouseButton.y);
			break;
		case sf::Event::MouseMoved:
			pos=sf::Vector2f(e.mouseMove.x,e.mouseMove.y);
			break;
		default:
			pos=sf::Vector2f(0,0);
		}
	}
	MenuEvent adapted(Menu* menu) const;
};

class Menu {
protected:
	Menu* parent;
	std::vector<Menu*> children;

	virtual void event_hover() {}
	virtual void event_click(sf::Vector2f pos) {}
	virtual void event_resize() {}
	virtual bool event_input(const MenuEvent& event) { return false; }
	virtual void event_frame(float delta) {}
	virtual bool event_message(int msg) { return false; }

public:
	Node node;
	sf::Vector2f size;
	bool hover;
	bool pressed;
	sf::Vector2f pointer_pos;

	Menu();
	virtual ~Menu() {}

	bool point_inside(sf::Vector2f p);	//relative to this menu

	void resize(sf::Vector2f size);
	bool send_event(const MenuEvent& event);	//event must be adapted to this menu
	bool send_message(int msg);		//this menu will process message before sending it up
	bool trigger_message(int msg);	//message will be sent up directily
	void frame(float delta);

	void add_child(Menu* menu);
	bool remove_child(Menu* menu);
	void clear_children();

	typedef std::unique_ptr<Menu> Ptr;
};

//widgets:

class MenuButton : public Menu {
public:
	int action;
	Color color_normal;
	Color color_hover;
	MenuButton();
	virtual void event_hover() override;
	virtual void event_click(sf::Vector2f pos) override { send_message(action); }

	typedef std::unique_ptr<MenuButton> Ptr;
};

class MenuSpriteButton : public MenuButton {
public:
	MenuSpriteButton() {}
	MenuSpriteButton(const Texture& tex) {
		set_texture(tex);
	}
	void set_texture(const Texture& tex) {
		node.texture=tex;
		size=tex.get_size();
	}
	typedef std::unique_ptr<MenuSpriteButton> Ptr;
};

class MenuTextButton : public MenuButton {
public:
	enum Align {
		ALIGN_LEFT,
		ALIGN_CENTER,
		ALIGN_RIGHT
	};

private:
	Align align;
	Node text;

	void update_align();
public:
	MenuTextButton();
	void set_text(const std::string& txt);
	void set_align(Align a);
	virtual void event_resize() override;
	typedef std::unique_ptr<MenuTextButton> Ptr;
};


class MenuSlider : public Menu {
	Node::Ptr anchor1;
	Node::Ptr anchor2;
	Node::Ptr bar;
	Node::Ptr handle;

	float value;	//0-1

	Node::Ptr create_sprite(const std::string& texture);
	float value_for_x(float x);
	float x_for_value(float value);
	void update_hover_state();

public:
	MenuSlider() ;
	float get_value();
	void set_value(float v);
	virtual void event_hover() override;
	virtual void event_resize() override;
	virtual bool event_input(const MenuEvent& event) override;

	typedef std::unique_ptr<MenuSlider> Ptr;
};



#endif
