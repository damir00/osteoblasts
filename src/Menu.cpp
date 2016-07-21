#include <stdio.h>

#include "Utils.h"
#include "Menu.h"
#include "Loader.h"

//MenuEvent
MenuEvent MenuEvent::adapted(Menu* menu) const {
	MenuEvent e=*this;
	e.pos.x=(pos.x-menu->node.pos.x)/menu->node.scale.x;
	e.pos.y=(pos.y-menu->node.pos.y)/menu->node.scale.y;

	return e;
}

//Menu
Menu::Menu() {
	pressed=false;
	hover=false;
	parent=NULL;
}
void Menu::resize(sf::Vector2f _size) {
	size=_size;
	event_resize();
}
void Menu::add_child(Menu* menu) {
	menu->parent=this;
	children.push_back(menu);
	//node.children.push_back(menu->node.get());
	node.add_child(&menu->node);
}
bool Menu::remove_child(Menu* menu) {
	if(Utils::vector_remove(children,menu)) {
		menu->parent=NULL;
		node.remove_child(&menu->node);
		return true;
	}
	return false;
}
void Menu::clear_children() {
	children.clear();
	node.clear_children();
}

bool Menu::send_event(const MenuEvent& event) {
	if(!node.visible) {
		return false;
	}

	if(event.event.type==sf::Event::MouseMoved) {
		pointer_pos=event.pos;
	}

	for(int i=children.size()-1;i>=0;i--) {
		if(children[i]->send_event(event.adapted(children[i]))) {
			return true;
		}
	}

	switch(event.event.type) {
		case sf::Event::MouseMoved: {
			bool inside=point_inside(event.pos);
			if(hover!=inside) {
				hover=inside;
				event_hover();
			}
			break;
		}
		case sf::Event::MouseButtonPressed:
			if(point_inside(event.pos)) {
				pressed=true;
			}
			break;
		case sf::Event::MouseButtonReleased:
			if(pressed && point_inside(event.pos)) {
				event_click(event.pos);
			}
			pressed=false;
			break;
		default:
			break;
	}

	return event_input(event);
}
bool Menu::send_message(int msg) {
	if(event_message(msg)) {
		return true;
	}
	if(!parent) {
		return false;
	}
	return parent->send_message(msg);
}
bool Menu::trigger_message(int msg) {
	if(!parent) {
		return false;
	}
	return parent->send_message(msg);
}

void Menu::frame(float delta) {
	for(Menu* child : children) {
		child->frame(delta);
	}
	event_frame(delta);
}
bool Menu::point_inside(sf::Vector2f p) {	//relative to this menu
	return (p.x>=0 && p.y>=0 && p.x<=size.x && p.y<=size.y);
}

//MenuButton
MenuButton::MenuButton() {
	action=-1;
	color_normal=Color(1,1,1,1);
	color_hover=Color(0.5,0.5,0.5,1);
}
void MenuButton::event_hover() {
	if(hover) {
		node.color=color_hover;
	}
	else {
		node.color=color_normal;
	}
}

//MenuTextButton
void MenuTextButton::update_align() {
	float offset;
	if(align==ALIGN_LEFT) offset=0.0;
	else if(align==ALIGN_CENTER) offset=0.5;
	else offset=1.0;
	text.pos.x=(size.x-text.text.getLocalBounds().width)*offset;
}
MenuTextButton::MenuTextButton() {
	text.type=Node::TYPE_TEXT;
	text.text.setFont(*Loader::get_menu_font());
	text.text.setCharacterSize(22);
	node.add_child(&text);
	set_align(ALIGN_LEFT);
}
void MenuTextButton::set_text(const std::string& txt) {
	text.text.setString(txt);
}
void MenuTextButton::set_align(Align a) {
	align=a;
	update_align();
}
void MenuTextButton::event_resize() {
	update_align();
}

//MenuSlider
Node::Ptr MenuSlider::create_sprite(const std::string& texture) {
	Node::Ptr n=Node::Ptr(new Node());
	n->texture=Loader::get_texture("menu/slider/"+texture);
	node.add_child(n.get());
	return n;
}
float MenuSlider::value_for_x(float x) {
	return Utils::clamp(0,1,(x-17)/(size.x-17*2));
}
float MenuSlider::x_for_value(float value) {
	return 17+value*(size.x-17*2);
}
void MenuSlider::update_hover_state() {
	if(hover || pressed) {
		node.color.set(1.0,0.5,0.5,1.0);
	}
	else {
		node.color.set(1.0,1.0,1.0,1.0);
	}
}
MenuSlider::MenuSlider() {
	anchor1=create_sprite("anchor1.png");
	anchor2=create_sprite("anchor2.png");
	bar=create_sprite("bar.png");
	handle=create_sprite("handle.png");
	pressed=false;
	value=0.5;
}
float MenuSlider::get_value() {
	return value;
}
void MenuSlider::set_value(float v) {
	value=v;
	handle->pos.x=x_for_value(value)-handle->texture.get_size().x/2;
}
void MenuSlider::event_hover() {
	update_hover_state();
}

void MenuSlider::event_resize() {
	anchor1->pos=sf::Vector2f(0,2);
	anchor2->pos=sf::Vector2f(size.x-anchor2->texture.get_size().x,2);
	bar->pos=sf::Vector2f(9,5);
	bar->scale.x=(size.x-9*2)/(float)bar->texture.get_size().x;
	handle->pos=sf::Vector2f(size.x/2,0);
}
bool MenuSlider::event_input(const MenuEvent& event) {
	if(event.event.type==sf::Event::MouseButtonPressed) {
		if(point_inside(event.pos)) {
			set_value(value_for_x(event.pos.x));
			return true;
		}
	}
	else if(event.event.type==sf::Event::MouseButtonReleased) {
		update_hover_state();
	}
	else if(event.event.type==sf::Event::MouseMoved && pressed) {
		set_value(value_for_x(event.pos.x));
	}
	return false;
}
