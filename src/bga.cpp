#include <stdio.h>
#include <vector>
#include <string>

#include <SFML/System.hpp>

#include "Framework.h"
#include "Node.h"
#include "Menu.h"
#include "Texture.h"
#include "Loader.h"

#include "SpaceBackground.h"
#include "MenuMain.h"

/*
class TestMenu : public Menu {
public:

	std::vector<Node::Ptr> items;
	std::vector<Menu::Ptr> menus;
	SpaceBackground bg;

	sf::Vector2f mouse_pos;

	Node* add_item(std::string id,sf::Vector2f pos) {
		Node::Ptr new_node(new Node());
		new_node->texture=Loader::get_texture(id);
		new_node->pos=pos;

		new_node->rotation=45;
		new_node->set_origin_center();

		node.add_child(new_node.get());
		items.push_back(std::move(new_node));
		return new_node.get();
	}
	void add_menu(Menu* btn,sf::Vector2f pos) {
		btn->node.pos=pos;
		menus.push_back(Menu::Ptr(btn));
		add_child(btn);
	}
	MenuSpriteButton* add_sprite_button(std::string id,sf::Vector2f pos) {
		MenuSpriteButton* btn=new MenuSpriteButton(Loader::get_texture(id));
		add_menu(btn,pos);
		return btn;
	}
	MenuTextButton* add_text_button(std::string text,sf::Vector2f pos) {
		MenuTextButton* btn=new MenuTextButton();
		btn->set_text(text);
		btn->set_align(MenuTextButton::ALIGN_CENTER);
		btn->resize(sf::Vector2f(150,30));
		add_menu(btn,pos);
		return btn;
	}
	MenuSlider* add_slider(sf::Vector2f pos,sf::Vector2f size) {
		MenuSlider* slider=new MenuSlider();
		slider->resize(sf::Vector2f(348,16));
		add_menu(slider,pos);
		return slider;
	}

	TestMenu() {
		//bg=SpaceBackground::create_default();
		bg.create_default();
		node.add_child(&bg);
		add_item("background/a_5.png",sf::Vector2f(10,10));

		add_sprite_button("menu/start button.png",sf::Vector2f(100,100));
		add_text_button("hello foo!",sf::Vector2f(100,150));
		add_slider(sf::Vector2f(100,200),sf::Vector2f(348,16));
	}

	void event_resize() override {
		bg.pos=sf::Vector2f(100,100);
	}

	bool event_input(const MenuEvent& event) override {
		if(event.event.type==sf::Event::MouseMoved) {
			items[0]->pos=event.pos;
			mouse_pos=event.pos;
		}

		return false;
	}
	void event_frame(float delta) override {
		items[0]->rotation+=delta/1000.0*90.0;

		sf::Vector2f offset=mouse_pos*20.0f;
		sf::FloatRect r(offset.x,offset.y,size.x-200,size.y-200);
		bg.update(r);
	}

};
*/

class TestMenu : public Menu {
public:
	Node foo;

	TestMenu() {
		node.add_child(&foo);

		foo.pos=sf::Vector2f(12,34);
		//foo.scale=sf::Vector2f(100,100);
		foo.type=Node::TYPE_TEXTURE;

		Texture tex;
		tex.tex=new sf::Texture();

		int w=32;
		int h=32;
		bool create_ret=tex.tex->create(w,h);
		printf("create: %d\n",create_ret);

		sf::Uint8* pixels=new sf::Uint8[w*h*4];

		for(int x=0;x<w;x++) {
			for(int y=0;y<h;y++) {
				sf::Uint8* dst=pixels+(x+y*w)*4;
				dst[0]=255*y/h;
				dst[1]=0;
				dst[2]=0;
				dst[3]=255;
			}
		}
		tex.tex->update(pixels);
		delete[] pixels;

		//tex.tex->
		foo.texture=Texture(tex.tex);
		//foo.texture=Loader::get_texture("general assets/explosions.png");
	}

};

#ifdef WIN32
#include <Windows.h>
int WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow) {

	int argc=1;
	char* argv[]={"bga.exe"};

#else
int main(int argc,char** argv) {
#endif
	printf("Welcome to BGA!\n");

	Loader::init();
	Framework f;

	/*
	TestMenu test_menu;
	f.get_root_menu()->add_child(&test_menu);
	*/


	MenuMain main_menu;
	main_menu.set_quit_action(f.MESSAGE_QUIT);
	f.get_root_menu()->add_child(&main_menu);
	if(argc>1 && (std::string)argv[1]=="-s") {
		main_menu.go_game();
	}


	f.run();

	Loader::clear_all();

	return 0;
}
