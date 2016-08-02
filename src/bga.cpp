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
	Node t_node;

	TestMenu() {
		node.add_child(&t_node);

		t_node.type=Node::TYPE_SOLID;
		t_node.color.set(1,0.5,0,1);
		t_node.pos=sf::Vector2f(10,10);
		//t_node.scale=sf::Vector2f(100,200);

		for(int i=0;i<100;i++) {
			Node* n=new Node();
			n->type=Node::TYPE_SOLID;
			n->color=Color::rand();
			n->pos=Utils::rand_vec(0,1500);
			n->scale=Utils::rand_vec(10,50);
			t_node.add_child(n);
		}

		NodeShader shader1;
		shader1.shader=Loader::get_shader("shader/boom.frag");
		shader1.set_param("freq",5);
		shader1.set_param("amount",0.01);
		t_node.post_process_shaders.push_back(shader1);

		NodeShader shader2;
		shader2.shader=Loader::get_shader("shader/test1.frag");
		//shader2.set_param("freq",5);
		//shader2.set_param("amount",0.01);
		t_node.post_process_shaders.push_back(shader2);
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
