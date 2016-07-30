#include <memory>
#include <vector>
#include <functional>

#include <noise/noise.h>

#include "Texture.h"
#include "Loader.h"
#include "SpaceBackground.h"
#include "Game.h"
#include "MenuMain.h"

class MenuMainSettings : public Menu {

	std::vector<Node::Ptr> items;

	MenuSpriteButton::Ptr add_button(const std::string& texture,float x,float y) {
		MenuSpriteButton::Ptr btn(new MenuSpriteButton());
		btn->set_texture(Loader::get_texture(texture));
		btn->node.pos=sf::Vector2f(x,y);
		add_child(btn.get());
		return btn;
	}
	Node* add_text(const std::string& text,int col,int row) {
		Node* t=new Node();
		t->type=Node::TYPE_TEXT;
		t->text.setString(text);
		t->text.setFont(*Loader::get_menu_font());
		t->text.setCharacterSize(22);

		t->pos.x=405-t->text.getLocalBounds().width;
		t->pos.y=239+row*46;

		node.add_child(t);
		items.push_back(Node::Ptr(t));
		return t;
	}
	MenuSlider::Ptr add_slider(int col,int row) {
		MenuSlider::Ptr slider(new MenuSlider());
		if(col==0) {
			slider->node.pos=sf::Vector2f(147,239+row*46);
		}
		else {
			slider->node.pos=sf::Vector2f(502,239+row*46);
		}
		slider->resize(sf::Vector2f(348,16));
		add_child(slider.get());
		return slider;
	}
	MenuTextButton::Ptr add_text_button(const std::string& text,int col,int row) {
		MenuTextButton::Ptr btn(new MenuTextButton());

		btn->set_text(text);
		if(col==0) {
			btn->node.pos=sf::Vector2f(145,239+row*46);
			btn->set_align(MenuTextButton::ALIGN_RIGHT);
			btn->resize(sf::Vector2f(405-145,25));
		}
		else {
			btn->node.pos=sf::Vector2f(500,239+row*46);
			btn->set_align(MenuTextButton::ALIGN_CENTER);
			btn->resize(sf::Vector2f(332,25));
		}

		add_child(btn.get());
		return btn;
	}

public:
	MenuButton::Ptr btn_back;
	MenuButton::Ptr btn_clear_save;
	MenuButton::Ptr btn_reset_defaults;
	MenuButton::Ptr btn_fullscreen;
	MenuSlider::Ptr music_slider;
	MenuSlider::Ptr sfx_slider;

	MenuMainSettings() {
		btn_back=add_button("menu/exit icon.png",907,487);
		add_text("MUSIC",0,0);
		add_text("SFX",0,1);
		add_text("FULLSCREEN",0,2);
		btn_clear_save=add_text_button("CLEAR SAVE",0,3);
		btn_reset_defaults=add_text_button("RESET DEFAULTS",0,4);

		music_slider=add_slider(1,0);
		sfx_slider=add_slider(1,1);
		btn_fullscreen=add_text_button("OFF",1,2);
	}
};


class MenuMainHeader : public Menu {
	class Blinker {

		struct Blink {
			float ts;
			float duration;
			float pause;
			int index;
		};

		int blink_count;
		float blink_duration;
		float blink_pause;
		std::vector<Blink> blinks;

		void update_shade(int i,float x) {
			x=1.0-Utils::clamp(0,1,x);
			shade_rects[i]->color=Color(sf::Color(20,12,28,255*0.5*x));
		}
		bool index_taken(int i) {
			for(const Blink& b : blinks) {
				if(b.index==i) {
					return true;
				}
			}
			return false;
		}
		int get_new_index() {
			int i=Utils::rand_range_i(0,shade_rects.size()-1);
			for(std::size_t x=0;x<shade_rects.size() && index_taken(i);x++) {
				i=(i+1)%shade_rects.size();
			}
			return i;
		}

	public:
		std::vector<Node::Ptr> shade_rects;

		Blinker() {
			blink_count=3;
			blink_duration=500;
			blink_pause=1500;
			blinks.resize(blink_count);
			for(Blink& b : blinks) {
				b.index=-1;
			}
		}
		void frame(float delta) {
			for(Blink& b : blinks) {
				if(b.index==-1 || b.ts>b.duration+b.pause) {
					if(b.index!=-1) {
						update_shade(b.index,1.0);
						b.index=-1;
						b.ts=0;
					}
					else {
						b.ts=Utils::rand_range(0,blink_duration+blink_pause);
					}
					b.index=get_new_index();
					b.duration=blink_duration*Utils::rand_range(0.8,1.2);
					b.pause=blink_pause*Utils::rand_range(0.8,1.2);
					continue;
				}
				b.ts+=delta;
				update_shade(b.index,b.ts/b.duration);
			}
		}
	};

	float time;
	float boom_amount;
	std::vector<Node::Ptr> nodes;

	Node* node_red_lines;
	Node* node_boom;
	sf::Shader shader_boom;

	noise::module::Perlin perlin;
	//std::vector<MenuMainHeaderShadeRectPtr> shade_rects;
	Blinker blinker;

	Node* add_node(const std::string& texture,float x,float y) {
		Node* nnode=new Node();
		nnode->texture=Loader::get_texture(texture);
		nnode->pos=sf::Vector2f(x,y);
		node.add_child(nnode);
		nodes.push_back(Node::Ptr(nnode));
		return nnode;
	}
	void add_shade_rect(int x,int y,int w,int h) {
		Node::Ptr rect(new Node());
		rect->type=Node::TYPE_SOLID;
		rect->scale=sf::Vector2f(w,h);
		rect->pos=sf::Vector2f(x,y);
		rect->color=Color(sf::Color(20,12,28,0));
		node.add_child(rect.get());
		blinker.shade_rects.push_back(std::move(rect));
	}
	float get_shade_perlin(float x,float time) {
		float t=(perlin.GetValue(x*6000.0,time/5000.0,0)+1.0)/2.0;
		return Utils::clamp(0,1,t*30.0-5.0);
	}

	sf::Color color_lerp(const sf::Color& c1,const sf::Color& c2,float a) {
		return sf::Color(
				Utils::lerp(c1.r,c2.r,a),
				Utils::lerp(c1.g,c2.g,a),
				Utils::lerp(c1.b,c2.b,a),
				Utils::lerp(c1.a,c2.a,a));
	}

public:
	MenuMainHeader() {

		node_boom=add_node("menu/boom.png",128-15,96);
		node_red_lines=add_node("menu/red lines.png",37,80);
		add_node("menu/17x15 the.png",618,125);
		add_node("menu/27x23 asteroid.png",623,145);
		add_node("menu/27x23 goes.png",623,97);

		add_shade_rect(618+0, 125+0,16,15);
		add_shade_rect(618+17,125+0,17,15);
		add_shade_rect(618+35,125+0,17,15);

		add_shade_rect(623+0, 97+0,25,23);
		add_shade_rect(623+27,97+0,25,23);
		add_shade_rect(623+54,97+0,25,23);
		add_shade_rect(623+81,97+0,25,23);

		add_shade_rect(623+0,  145+0,25,23);
		add_shade_rect(623+27, 145+0,25,23);
		add_shade_rect(623+54, 145+0,23,23);
		add_shade_rect(623+81, 145+0,25,23);
		add_shade_rect(623+108,145+0,25,23);
		add_shade_rect(623+135,145+0,25,23);
		add_shade_rect(623+162,145+0,23,23);
		add_shade_rect(623+189,145+0,25,23);

		/*
		shader_boom.loadFromFile("assets/shader/boom.frag",sf::Shader::Fragment);
		shader_boom.setParameter("texture", sf::Shader::CurrentTexture);

		node_boom->shader.shader=&shader_boom;
		node_boom->shader.set_param("freq",5);
		node_boom->shader.set_param("amount",0.01);
		*/

		perlin.SetFrequency(1);
		perlin.SetOctaveCount(3);
		perlin.SetSeed(rand());

		time=0;
		boom_amount=0;

		size=sf::Vector2f(960,540);
	}
	void event_frame(float delta) override {
		time+=delta;

		blinker.frame(delta);
		/*
		node_boom->shader.set_param("time",-time/700.0);
		node_boom->shader.set_param("amount",boom_amount);
		*/
		boom_amount=Utils::num_move_towards(boom_amount,0.004,delta*0.00003);

		float a=(cos(time/1000.0)+1.0)/2.0;
		Color red_color1(196.0/255.0,81.0/255.0,83.0/255.0,1.0);
		Color red_color2(255.0/255.0,22.0/255.0,26.0/255.0,1.0);
		node_red_lines->color=red_color1.lerp(red_color2,a);
	}
	void on_clicked(sf::Vector2f mouse_pos) {
		if(Utils::probability(0.75)) {
			boom_amount=0.02;
		}
	}
};


class MenuMainBg : public Menu {
	SpaceBackground bg;
	Node color_overlay;

	sf::Vector2f bg_pos;
public:
	MenuMainBg() {
		bg.create_default();
		bg.set_background_visible(false);
		node.add_child(&bg);

		color_overlay.type=Node::TYPE_SOLID;
		color_overlay.color=Color(sf::Color(20,12,28,179));
		node.add_child(&color_overlay);
	}
	void event_frame(float delta) override {
		bg_pos.y+=delta*0.2;
		sf::Vector2f p=bg_pos+pointer_pos*0.1f;
		sf::FloatRect rect(p.x,p.y,size.x,size.y);
		bg.update(rect);
	}
	void event_resize() override {
		color_overlay.scale=size;
	}
};

class MenuMainFirst : public Menu {
	std::vector<Node::Ptr> nodes;

	MenuSpriteButton::Ptr add_button(const std::string& texture,float x,float y) {
		MenuSpriteButton::Ptr btn(new MenuSpriteButton());
		btn->set_texture(Loader::get_texture(texture));
		btn->node.pos=sf::Vector2f(x,y);
		add_child(btn.get());
		return btn;
	}
	MenuTextButton::Ptr add_text_button(const std::string& text,float x,float y,float w,float h) {
		MenuTextButton::Ptr btn(new MenuTextButton());
		btn->set_text(text);
		btn->set_align(MenuTextButton::ALIGN_CENTER);
		btn->node.pos=sf::Vector2f(x,y);
		btn->resize(sf::Vector2f(w,h));
		add_child(btn.get());
		return btn;
	}
	Node* add_sprite(const std::string& texture,float x,float y) {
		Node* new_node=new Node();
		new_node->texture=Loader::get_texture(texture);
		new_node->pos=sf::Vector2f(x,y);
		node.add_child(new_node);
		nodes.push_back(Node::Ptr(new_node));
		return new_node;
	}

public:
	MenuButton::Ptr btn_start;
	MenuButton::Ptr btn_quit;
	MenuButton::Ptr btn_settings;

	MenuMainFirst() {
		/*
		options icon - 30x487
		exit icon - 907x487
		start button - 435x398
		BOOM - 128x96
		Goes - 623x97
		the - 618x125
		Asteroid - 623x145
		red lines - 37x80
		white lines - 0x407
		 */

		//btn_start=add_button("menu/start button.png",435,398);
		btn_start=add_text_button("START",436-10,398-5,85+20,15+10);
		btn_quit=add_button("menu/exit icon.png",907,487);
		btn_settings=add_button("menu/options icon.png",30,487);
		add_sprite("menu/white lines.png",0,407);
	}
};

class MenuMain::MenuMainImpl : public Menu {
public:
	MenuMainBg bg;
	MenuMainHeader header;
	MenuMainFirst page_first;
	MenuMainSettings page_settings;
	Game menu_game;

	std::vector<Menu*> pages;
	Menu* page_current;

	int action_quit;

	class Transitioner {
		bool _active;
		Menu* menu_out;
		Menu* menu_in;
		float anim;

		void update_anim() {
			menu_in->node.color.a=anim;
			menu_out->node.color.a=1.0-anim;
		}
	public:
		float duration;

		enum Event {
			EVENT_TRANSITION_START,
			EVENT_TRANSITION_END
		};
		typedef std::function<void(Event,Menu* to_menu)> Callback;
		Callback callback;

		Transitioner() {
			_active=false;
			duration=100;
		}
		void frame(float dt) {
			if(!active()) {
				return;
			}
			anim+=dt/duration;
			if(anim>=1.0) {
				anim=1.0;
				_active=false;
				menu_out->node.visible=false;
				if(callback) {
					callback(EVENT_TRANSITION_END,menu_in);
				}
			}
			update_anim();
		}
		bool active() {
			return _active;
		}
		void start(Menu* _menu_out,Menu* _menu_in) {
			menu_out=_menu_out;
			menu_in=_menu_in;
			anim=0;
			_active=true;

			menu_out->node.visible=true;
			menu_in->node.visible=true;
			update_anim();

			if(callback) {
				callback(EVENT_TRANSITION_START,menu_in);
			}
		}
	};
	Transitioner transitioner;

	enum MyMessage {
		MSG_BTN_QUIT=1,
		MSG_BTN_START,
		MSG_BTN_SETTINGS,
		MSG_SETTINGS_BACK,
		MSG_GAME_ESC,
	};

	void add_page(Menu* page) {
		add_child(page);
		pages.push_back(page);
	}
	void select_page(Menu* page) {
		if(page==page_current) {
			return;
		}
		transitioner.start(page_current,page);
		page_current=page;
	}

	MenuMainImpl() {
		action_quit=-1;

		add_child(&bg);
		add_child(&header);
		add_page(&page_first);
		add_page(&page_settings);
		add_child(&menu_game);

		menu_game.node.visible=false;
		for(Menu* m : pages) {
			m->node.visible=false;
		}
		page_first.node.visible=true;
		page_current=&page_first;

		page_first.btn_quit->action=MSG_BTN_QUIT;
		page_first.btn_start->action=MSG_BTN_START;
		page_first.btn_settings->action=MSG_BTN_SETTINGS;
		page_settings.btn_back->action=MSG_SETTINGS_BACK;
		menu_game.action_esc=MSG_GAME_ESC;

		transitioner.callback=([this](Transitioner::Event e,Menu* m) {
			bool show=true;
			if(e==Transitioner::EVENT_TRANSITION_END && m==&menu_game) {
				show=false;
			}
			bg.node.visible=show;
			header.node.visible=show;
		});
	}
	void event_resize() override {
		sf::Vector2f menu_size(960,540);
		sf::Vector2f offset(0,0);
		float scale=1;
		sf::Vector2f scale_f;

		if(size.x/size.y<menu_size.x/menu_size.y) {
			scale=size.x/menu_size.x;
			offset.y=(size.y-menu_size.y*scale)/2;
		}
		else {
			scale=size.y/menu_size.y;
			offset.x=(size.x-menu_size.x*scale)/2;
		}
		scale_f=sf::Vector2f(scale,scale);

		for(Menu * page : pages) {
			page->node.pos=offset;
			page->node.scale=scale_f;
		}
		header.node.pos=offset;
		header.node.scale=scale_f;

		/*
		menu_game.node.pos=sf::Vector2f(200,200);
		menu_game.resize(size-sf::Vector2f(400,400));
		*/
		menu_game.resize(size);

		bg.resize(size);
	}
	bool event_message(int msg) override {
		printf("MSG %d\n",msg);

		switch(msg) {
		case MSG_BTN_QUIT:
			//g_global.framework->window.close();
			trigger_message(action_quit);
			return true;
		case MSG_BTN_START:
			//menu_game.start_level("assets/game2.json");
			menu_game.start_level();
			select_page(&menu_game);
			return true;
		case MSG_GAME_ESC:
			select_page(&page_first);
			//g_global.framework->grab_mouse(false);
			return true;
		case MSG_BTN_SETTINGS:
			select_page(&page_settings);
			return true;
		case MSG_SETTINGS_BACK:
			select_page(&page_first);
			return true;
		}

		return true;
	}
	void event_frame(float delta) override {
		transitioner.frame(delta);
	}
};

MenuMain::MenuMain() {
	impl=new MenuMainImpl();
	add_child(impl);
}
MenuMain::~MenuMain() {
	delete(impl);
}
void MenuMain::set_quit_action(int action) {
	impl->action_quit=action;
}
void MenuMain::event_resize() {
	impl->resize(size);
}
void MenuMain::go_game() {
	impl->menu_game.start_level();
	impl->select_page(&impl->menu_game);
}


