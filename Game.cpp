#include "Game.h"
#include "Audio.h"

#include <math.h>
#include <noise/noise.h>
#include <tr1/memory>
#include <sstream>

#include <SFML/OpenGL.hpp>

class Global {
public:
	GameFramework* framework;
	Game* game;
};
Global g_global;





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

	void draw(const NodeState& state) {
		state.render_target->draw(text,state.render_state);
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
	void draw(const NodeState& state) {
		state.render_target->draw(shape_outline,state.render_state);
		state.render_target->draw(shape_fill,state.render_state);
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

				if(i==3) {
					s->setScale(3.0f);
				}

				game->spawn_ship(s);
				printf("Spawned ship %d\n",i);
				break;
			}
		}
	}

};

class MenuTweaker : public Menu {
	std::vector<UIItem*> tweakers;
	std::vector<sf::Keyboard::Key> tweaker_keys;
	UIItem* tweaker_current;

	void add_tweaker(UIItem* tweaker,sf::Keyboard::Key key) {
		tweakers.push_back(tweaker);
		tweaker_keys.push_back(key);

		add_child(tweaker);
		tweaker->visible=false;
	}
	void tweaker_toggle(UIItem* tweaker) {
		if(!tweaker_current) {
			tweaker->visible=true;
			tweaker_current=tweaker;
		}
		else if(tweaker_current==tweaker) {
			tweaker->visible=false;
			tweaker_current=NULL;
		}
		else {
			tweaker_current->visible=false;
			tweaker->visible=true;
			tweaker_current=tweaker;
		}
		g_global.framework->grab_mouse(!tweaker_current);
		g_global.game->input_enabled=(!tweaker_current);
	}
public:
	MenuTweaker() {
		tweaker_current=NULL;
		//add_tweaker(new Tweaker(g_global.game),sf::Keyboard::D);
		add_tweaker(new ShipSelector(g_global.game),sf::Keyboard::F);
	}

	void on_event(sf::Event event) {

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
};




class MenuGame : public Menu {
	Game* game;
	MenuTweaker tweaker;

public:

	int action_esc;

	MenuGame() {
		action_esc=-1;
	}
	void add_game(Game* _game) {
		game=_game;
		add_item(game);
		add_item(&tweaker);
//		tweaker.set_enabled(false);
	}
	void menu_frame(float delta) {
		game->update(delta);
	}
	void start_level(std::string level) {
		game->current_level=level;
		game->start_level();

		sf::Mouse::setPosition(Utils::vec_to_i(size/2.0f),g_global.framework->window);
		g_global.framework->grab_mouse(true);
	}
	void on_key_pressed(sf::Keyboard::Key key) {
		if(key==sf::Keyboard::Escape) {
			message_send(action_esc);
		}
	}
	void on_resized() {

	}
};

/*

class MenuMainPage : public Menu {
	sf::Vector2f ship_pos;
	float anim;

	sf::Sprite sprite_ship;
	SpriteAnim anim_fire;

	sf::Sprite sprite_flare;
	sf::Sprite sprite_border;

	noise::module::Perlin perlin;
	BGManager bg;

public:
	MenuMainPage() {
		anim=0;
		perlin.SetFrequency(0.0001);
		perlin.SetOctaveCount(3);
		perlin.SetSeed(rand());

		sprite_ship.setTexture(*GameRes::get_texture("menu/mini_ship.png"));
		anim_fire.load(GameRes::get_anim("menu/mini_fire.png"),true);

		sprite_flare.setTexture(*GameRes::get_texture("menu/flare.png"));
		sprite_flare.setScale(3,3);
		sprite_flare.setOrigin(0.5,0.5);

		sprite_border.setTexture(*GameRes::get_texture("menu/border.png"));

	}
	void draw(sf::RenderTarget& target, const sf::RenderStates& state) {
		bg.draw(target,state);

		sf::RenderStates s=state;
		s.transform.translate(ship_pos);
		s.transform.translate(-Utils::sprite_size(sprite_ship)*0.5f);
		s.transform.scale(3,3);

		sf::RenderStates e1=s;
		e1.transform.translate(9+4,40+7);
		anim_fire.draw(target,e1);
		e1=s;
		e1.transform.translate(25+4,40+7);
		anim_fire.draw(target,e1);
		target.draw(sprite_ship,s);

		target.draw(sprite_border,state);

		sf::Vector2f translates[]={
				sf::Vector2f(0,0),
				sf::Vector2f(size.x,0),
				sf::Vector2f(size.x,size.y),
				sf::Vector2f(0,size.y)
		};

		for(int j=0;j<4;j++) {

			sf::RenderStates e1=state;

			e1.transform.translate(translates[j]);
			e1.transform.rotate(j*90);

			for(int i=0;i<10;i++) {
				float noise=perlin.GetValue(anim*0.02,i*10000,(j+1)*1000);
				float n2=sin(noise*M_PI);
				float a=(Utils::lerp(noise,n2,0.5f)+1.0f)*0.5f*120.0f-15.0f;

				sf::RenderStates e=e1;

				sprite_flare.setColor(sf::Color(255,255,255,
						Utils::clampi(0,255,
								255*1.5f*(perlin.GetValue(anim*0.01f,1000*i,1000*j)*0.5f+0.5f)
						)
				));

				e.transform.translate(30,30);
				e.transform.rotate(a);
				e.transform.translate(0,-43*3);
				target.draw(sprite_flare,e);
			}
		}

	}
private:
	void menu_frame(float delta) {
		anim+=delta*100.0f;

		float gain=get_master_size().y*0.2;
		float v=anim*0.01f;
		ship_pos.x=perlin.GetValue(v,0,0)*gain;
		ship_pos.y=perlin.GetValue(0,v,0)*gain;
		ship_pos+=get_master_size()*0.5f;

		anim_fire.update(delta);
		bg.frame(sf::Vector2f(0,-anim*0.003f),sf::FloatRect(0,0,size.x,size.y));
	}
	void on_resized() {
		if(sprite_border.getTexture()) {
			sf::Vector2f s2=Utils::vec_to_f(sprite_border.getTexture()->getSize());
			sf::Vector2f s3(
					size.x/s2.x,
					size.y/s2.y);
			sprite_border.setScale(s3);
		}
	}
};
*/

typedef std::tr1::shared_ptr<Node> NodePtr;
typedef std::tr1::shared_ptr<NodeText> NodeTextPtr;
typedef std::tr1::shared_ptr<NodeSprite> NodeSpritePtr;
typedef std::tr1::shared_ptr<NodeContainer> NodeContainerPtr;

class MenuButton : public Menu {
public:
	int action;
	NodeColor color_normal;
	NodeColor color_hover;
	MenuButton() {
		action=-1;
		color_normal=NodeColor(1,1,1,1);
		color_hover=NodeColor(0.5,0.5,0.5,1);
	}
	virtual void on_clicked(sf::Vector2f pos) {
		message_send(action);
	}
	virtual void on_hover(bool hover) {
		if(hover) {
			color=color_hover;
		}
		else {
			color=color_normal;
		}
	}
};

class MenuSpriteButton : public MenuButton {
	NodeSpritePtr sprite;
public:

	MenuSpriteButton() {
		sprite=NodeSpritePtr(new NodeSprite());
		add_child(sprite.get());
	}
	MenuSpriteButton(const sf::Texture& tex) {
		sprite->sprite.setTexture(tex);
	}
	void set_texture(const sf::Texture& tex) {
		sprite->sprite.setTexture(tex);
		size.x=scale.x*(float)tex.getSize().x;
		size.y=scale.y*(float)tex.getSize().y;
	}
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

	void update_align() {
		float offset;
		if(align==ALIGN_LEFT) offset=0.0;
		else if(align==ALIGN_CENTER) offset=0.5;
		else offset=1.0;
		text->pos.x=(size.x-text->text.getLocalBounds().width)*offset;
	}
public:

	NodeTextPtr text;

	MenuTextButton() {
		text=NodeTextPtr(new NodeText());
		text->text.setFont(GameRes::font);
		text->text.setCharacterSize(22);
		add_child(text.get());
		set_align(ALIGN_LEFT);
	}
	void set_text(const std::string& txt) {
		text->text.setString(txt);
	}
	void set_align(Align a) {
		align=a;
		update_align();
	}
	virtual void on_resized() {
		update_align();
	}

};

typedef std::tr1::shared_ptr<MenuButton> MenuButtonPtr;
typedef std::tr1::shared_ptr<MenuSpriteButton> MenuSpriteButtonPtr;
typedef std::tr1::shared_ptr<MenuTextButton> MenuTextButtonPtr;

class MenuMainHeaderShadeRect : public NodeContainer {
public:
	sf::RectangleShape rect;
	MenuMainHeaderShadeRect() {
		item=&rect;
	}
};
typedef std::tr1::shared_ptr<MenuMainHeaderShadeRect> MenuMainHeaderShadeRectPtr;
typedef std::tr1::shared_ptr<BGManager> BGManagerPtr;

class MenuMainBg : public Menu {
	BGManagerPtr bg;
	sf::RectangleShape color_sprite;
	NodeContainer color_overlay;

	sf::Vector2f pos;
public:
	MenuMainBg() {
		bg=BGManagerPtr(new BGManager());
		bg->background_color=false;
		add_child(bg.get());

		color_sprite.setFillColor(sf::Color(20,12,28,179));
		color_sprite.setPosition(0,0);
		color_overlay.item=&color_sprite;
		add_child(&color_overlay);
	}
	void menu_frame(float delta) {
		pos.y+=delta*0.2;
		sf::FloatRect rect(0,0,size.x,size.y*2);
		bg->frame(pos,rect);
	}
	void on_resized() {
		color_sprite.setSize(size);
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
			shade_rects[i]->rect.setFillColor(sf::Color(20,12,28,255*0.5*x));
		}
		bool index_taken(int i) {
			for(int i1=0;i1<blinks.size();i1++) {
				if(blinks[i1].index==i) {
					return true;
				}
			}
			return false;
		}
		int get_new_index() {
			int i=Utils::rand_range_i(0,shade_rects.size()-1);
			//while(index_taken(i)) {
			for(int x=0;x<shade_rects.size() && index_taken(i);x++) {
				i=(i+1)%shade_rects.size();
			}
			return i;
		}

	public:
		std::vector<MenuMainHeaderShadeRectPtr> shade_rects;

		Blinker() {
			blink_count=3;
			blink_duration=500;
			blink_pause=1500;
			blinks.resize(blink_count);
			for(int i=0;i<blinks.size();i++) {
				blinks[i].index=-1;
			}
		}
		void frame(float delta) {
			for(int i=0;i<blinks.size();i++) {
				Blink& b=blinks[i];
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
	std::vector<NodePtr> nodes;

	NodeSpritePtr node_red_lines;
	NodeSpritePtr node_boom;
	sf::Shader shader_boom;

	noise::module::Perlin perlin;
	//std::vector<MenuMainHeaderShadeRectPtr> shade_rects;
	Blinker blinker;

	NodeSpritePtr add_node(const std::string& texture,float x,float y) {
		NodeSpritePtr node(new NodeSprite());
		node->sprite.setTexture(*GameRes::get_texture(texture));
		node->pos=sf::Vector2f(x,y);
		add_child(node.get());
		nodes.push_back(node);

		return node;
	}
	void add_shade_rect(int x,int y,int w,int h) {
		MenuMainHeaderShadeRectPtr rect=MenuMainHeaderShadeRectPtr(new MenuMainHeaderShadeRect());
		rect->rect.setSize(sf::Vector2f(w,h));
		rect->render_state.transform=sf::Transform::Identity;
		rect->render_state.transform.translate(sf::Vector2f(x,y));
		rect->rect.setFillColor(sf::Color(20,12,28,0));
		add_child(rect.get());
		blinker.shade_rects.push_back(rect);
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

		shader_boom.loadFromFile("assets/shader/boom.frag",sf::Shader::Fragment);
		shader_boom.setParameter("texture", sf::Shader::CurrentTexture);

		node_boom->shader.shader=&shader_boom;
		node_boom->shader.set_param("freq",5);
		node_boom->shader.set_param("amount",0.01);

		perlin.SetFrequency(1);
		perlin.SetOctaveCount(3);
		perlin.SetSeed(rand());

		time=0;
		boom_amount=0;

		size=sf::Vector2f(960,540);
	}
	void menu_frame(float delta) {
		time+=delta;

		blinker.frame(delta);

		node_boom->shader.set_param("time",-time/700.0);
		node_boom->shader.set_param("amount",boom_amount);

		boom_amount=Utils::num_move_towards(boom_amount,0.004,delta*0.00003);

		float a=(cos(time/1000.0)+1.0)/2.0;
		NodeColor red_color1(196.0/255.0,81.0/255.0,83.0/255.0,1.0);
		NodeColor red_color2(255.0/255.0,22.0/255.0,26.0/255.0,1.0);
		node_red_lines->color=red_color1.lerp(red_color2,a);
	}
	void on_clicked(sf::Vector2f mouse_pos) {
		if(Utils::probability(0.75)) {
			boom_amount=0.02;
		}
	}
};

class MenuMainFirst : public Menu {
	std::vector<NodePtr> nodes;

	MenuSpriteButtonPtr add_button(const std::string& texture,float x,float y) {
		MenuSpriteButtonPtr btn(new MenuSpriteButton());
		btn->set_texture(*GameRes::get_texture(texture));
		btn->pos=sf::Vector2f(x,y);
		add_item(btn.get());
		return btn;
	}
	MenuTextButtonPtr add_text_button(const std::string& text,float x,float y,float w,float h) {
		MenuTextButtonPtr btn(new MenuTextButton());
		btn->set_text(text);
		btn->pos=sf::Vector2f(x,y);
		btn->resize(sf::Vector2f(w,h));
		btn->set_align(MenuTextButton::ALIGN_CENTER);
		add_item(btn.get());
		return btn;
	}

	NodeSpritePtr add_node(const std::string& texture,float x,float y) {
		NodeSpritePtr node(new NodeSprite());
		node->sprite.setTexture(*GameRes::get_texture(texture));
		node->pos=sf::Vector2f(x,y);
		add_child(node.get());
		nodes.push_back(node);

		return node;
	}

public:

	MenuButtonPtr btn_start;
	MenuButtonPtr btn_quit;
	MenuButtonPtr btn_settings;

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
		add_node("menu/white lines.png",0,407);
	}
};


class MenuSlider : public Menu {
	NodeSpritePtr anchor1;
	NodeSpritePtr anchor2;
	NodeSpritePtr bar;
	NodeSpritePtr handle;

	NodeSpritePtr create_sprite(const std::string& texture) {
		NodeSpritePtr n=NodeSpritePtr(new NodeSprite());
		n->sprite.setTexture(*GameRes::get_texture("menu/slider/"+texture));
		add_child(n.get());
		return n;
	}

	bool pressed;

	float value;	//0-1

	float value_for_x(float x) {
		return Utils::clamp(0,1,(x-17)/(size.x-17*2));
	}
	float x_for_value(float value) {
		return 17+value*(size.x-17*2);
	}

	void update_hover_state() {
		if(state_hover || pressed) {
			//color.set(0.5,0.5,0.5,1.0);
			color.set(1.0,0.5,0.5,1.0);
		}
		else {
			color.set(1.0,1.0,1.0,1.0);
		}
	}

public:

	MenuSlider() {
		anchor1=create_sprite("anchor1.png");
		anchor2=create_sprite("anchor2.png");
		bar=create_sprite("bar.png");
		handle=create_sprite("handle.png");
		pressed=false;
	}

	float get_value() {
		return value;
	}
	void set_value(float v) {
		value=v;
		handle->pos.x=x_for_value(value)-handle->getSize().x/2;
	}
	void on_hover(bool hover) {
		update_hover_state();
	}

	void on_resized() {
		anchor1->pos=sf::Vector2f(0,2);
		anchor2->pos=sf::Vector2f(size.x-anchor2->getSize().x,2);
		bar->pos=sf::Vector2f(9,5);
		bar->sprite.setScale((size.x-9*2)/(float)bar->getSize().x,1.0);
		handle->pos=sf::Vector2f(size.x/2,0);
	}
	virtual void on_event(sf::Event event) {
		if(event.type==sf::Event::MouseButtonPressed) {
			if(event_inside(event.mouseButton)) {
				pressed=true;
				set_value(value_for_x(event.mouseButton.x));
			}
		}
		else if(event.type==sf::Event::MouseButtonReleased) {
			pressed=false;
			update_hover_state();
		}
		else if(event.type==sf::Event::MouseMoved && pressed) {
			set_value(value_for_x(event.mouseMove.x));
		}
	}
};

typedef std::tr1::shared_ptr<MenuSlider> MenuSliderPtr;

class MenuMainSettings : public Menu {

	std::vector<NodePtr> items;

	MenuSpriteButtonPtr add_button(const std::string& texture,float x,float y) {
		MenuSpriteButtonPtr btn(new MenuSpriteButton());
		btn->set_texture(*GameRes::get_texture(texture));
		btn->pos=sf::Vector2f(x,y);
		add_item(btn.get());
		return btn;
	}
	NodeTextPtr add_text(const std::string& text,int col,int row) {
		NodeTextPtr t(new NodeText());
		t->text.setString(text);
		t->text.setFont(GameRes::font);
		t->text.setCharacterSize(22);

		t->pos.x=405-t->text.getLocalBounds().width;
		t->pos.y=239+row*46;

		add_child(t.get());
		items.push_back(t);
		return t;
	}
	MenuSliderPtr add_slider(int col,int row) {
		MenuSliderPtr slider(new MenuSlider());
		if(col==0) {
			slider->pos=sf::Vector2f(147,239+row*46);
		}
		else {
			slider->pos=sf::Vector2f(502,239+row*46);
		}
		slider->resize(sf::Vector2f(348,16));
		add_item(slider.get());
		return slider;
	}
	MenuTextButtonPtr add_text_button(const std::string& text,int col,int row) {
		MenuTextButtonPtr btn=MenuTextButtonPtr(new MenuTextButton());

		btn->set_text(text);
		if(col==0) {
			btn->pos=sf::Vector2f(145,239+row*46);
			btn->set_align(MenuTextButton::ALIGN_RIGHT);
			btn->resize(sf::Vector2f(405-145,25));
		}
		else {
			btn->pos=sf::Vector2f(500,239+row*46);
			btn->set_align(MenuTextButton::ALIGN_CENTER);
			btn->resize(sf::Vector2f(332,25));
		}

		add_item(btn.get());
		return btn;
	}

public:
	MenuButtonPtr btn_back;
	MenuButtonPtr btn_clear_save;
	MenuButtonPtr btn_reset_defaults;
	MenuButtonPtr btn_fullscreen;
	MenuSliderPtr music_slider;
	MenuSliderPtr sfx_slider;

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

class MenuMain : public Menu {
public:
	MenuMainBg bg;
	MenuMainHeader header;
	MenuMainFirst page_first;
	MenuMainSettings page_settings;
	MenuGame menu_game;

	std::vector<Menu*> pages;
	Menu* page_current;

	class Transitioner {
		bool _active;
		Menu* menu_out;
		Menu* menu_in;
		float anim;

		void update_anim() {
			menu_in->color.a=anim;
			menu_out->color.a=1.0-anim;
		}
	public:
		float duration;

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
				menu_out->set_enabled(false);
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

			menu_out->set_enabled(true);
			menu_in->set_enabled(true);
			update_anim();
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
		add_item(page);
		pages.push_back(page);
	}
	void select_page(Menu* page) {
		if(page==page_current) {
			return;
		}
		transitioner.start(page_current,page);
		page_current=page;
	}

	MenuMain() {
		add_item(&bg);
		add_item(&header);
		add_page(&page_first);
		add_page(&page_settings);
		add_item(&menu_game);

		menu_game.set_enabled(false);
		for(int i=0;i<pages.size();i++) {
			pages[i]->set_enabled(false);
		}
		page_first.set_enabled(true);
		page_current=&page_first;

		page_first.btn_quit->action=MSG_BTN_QUIT;
		page_first.btn_start->action=MSG_BTN_START;
		page_first.btn_settings->action=MSG_BTN_SETTINGS;
		page_settings.btn_back->action=MSG_SETTINGS_BACK;
		menu_game.action_esc=MSG_GAME_ESC;
	}
	void resize(sf::Vector2f _size) {
		sf::Vector2f menu_size(960,540);
		sf::Vector2f offset(0,0);
		float scale=1;
		sf::Vector2f scale_f;

		size=_size;

		if(size.x/size.y<menu_size.x/menu_size.y) {
			scale=size.x/menu_size.x;
			offset.y=(size.y-menu_size.y*scale)/2;
		}
		else {
			scale=size.y/menu_size.y;
			offset.x=(size.x-menu_size.x*scale)/2;
		}
		scale_f=sf::Vector2f(scale,scale);

		for(int i=0;i<pages.size();i++) {
			pages[i]->pos=offset;
			pages[i]->scale=scale_f;
		}
		header.pos=offset;
		header.scale=scale_f;

		menu_game.resize(size);
		bg.resize(size);
	}
	void on_clicked(sf::Vector2f pos) {

	}
	bool on_message(int msg) {
		switch(msg) {
		case MSG_BTN_QUIT:
			g_global.framework->window.close();
			return true;
		case MSG_BTN_START:
			menu_game.start_level("assets/game2.json");
			select_page(&menu_game);
			return true;
		case MSG_GAME_ESC:
			select_page(&page_first);
			g_global.framework->grab_mouse(false);
			return true;
		case MSG_BTN_SETTINGS:
			select_page(&page_settings);
			return true;
		case MSG_SETTINGS_BACK:
			select_page(&page_first);
			return true;
		}

		return false;
	}
	void menu_frame(float delta) {
		transitioner.frame(delta);
	}
};

GameFramework::GameFramework() {
	game=new Game();
	clear_color=sf::Color(1,0,0,255);

	measure_fps_count=0;
	measure_fps_time=0;

	node_root=new Node();
	grab_mouse_on=false;

	g_global.framework=this;
	g_global.game=game;
}
void GameFramework::init_resources() {

	menu_root.reset(new MenuMain());
	((MenuMain*)menu_root.get())->menu_game.add_game(game);
	node_root->add_child(menu_root.get());
	menu_root->resize(Utils::vec_to_f(game_size));

}
GameFramework::~GameFramework() {
	delete(game);
}
void GameFramework::create_window(int w,int h) {
	window.setVerticalSyncEnabled(true);
	window.setFramerateLimit(80);

	window.create(sf::VideoMode(1024,768), "BGTA"/*,sf::Style::None*/);
	window_view=window.getView();
	game_size=Utils::vec_to_i(window_view.getSize());
	has_focus=true;

	window.setPosition(
			Utils::vec_to_i(
				Utils::vec_to_f(sf::Vector2i(
					sf::VideoMode::getDesktopMode().width,
					sf::VideoMode::getDesktopMode().height)-game_size)*0.5f));

	shader_enabled=sf::Shader::isAvailable();
	rtt_enabled=false;

	sf::ContextSettings settings = window.getSettings();
	printf("OpenGL version: %d.%d\n",settings.majorVersion,settings.minorVersion);
	printf("RTT supported: %d\n",rtt_enabled);
	printf("shader supported: %d\n",shader_enabled);
}
void GameFramework::start_level(std::string level) {
	game->current_level=level;
	game->start_level();
	clock.restart();
}
void GameFramework::grab_mouse(bool grab) {
	grab_mouse_on=grab;
	window.setMouseCursorVisible(!grab);
	if(grab) {
		sf::Mouse::setPosition(game_size/2,window);
	}
}
void GameFramework::frame() {

//		bool ignore_player_control=(tweaker_current!=NULL);

	//events
	sf::Event event;
	while(window.pollEvent(event)) {

		menu_root->event(event);

		//tweakers
//		tweaker_event(event);

		switch(event.type) {
			case sf::Event::Closed:
					window.close();
				break;
			case sf::Event::Resized:
				window_view=sf::View(sf::FloatRect(0.f, 0.f,event.size.width,event.size.height));
				game_size=Utils::vec_to_i(window_view.getSize());
				menu_root->resize(Utils::vec_to_f(game_size));
				window.setView(window_view);
				break;
			case sf::Event::LostFocus:
				has_focus=false;
				break;
			case sf::Event::GainedFocus:
				has_focus=true;
				break;
			case sf::Event::KeyPressed:
				switch(event.key.code) {
					case sf::Keyboard::S:
						game->spawner->update(10000);
						break;

					case sf::Keyboard::Num1: game->player->weapon_i=0; break;
					case sf::Keyboard::Num2: game->player->weapon_i=1; break;
					case sf::Keyboard::Num3: game->player->weapon_i=2; break;
					case sf::Keyboard::Num4: game->player->weapon_i=3; break;
					case sf::Keyboard::Num5: game->player->weapon_i=4; break;
					case sf::Keyboard::Num6: game->player->weapon_i=5; break;
				}
				break;
			case sf::Event::MouseMoved:
				break;
			case sf::Event::MouseButtonPressed:
				break;
			case sf::Event::MouseButtonReleased:
				break;

		}
	}

	if(rtt.getSize()!=window.getSize()) {
		int w=window.getSize().x;
		int h=window.getSize().y;
		rtt_enabled=(rtt.create(w,h,false) && rtt2.create(w,h,false));
		printf("reinited rtt to %d %d\n",w,h);
	}


	/*
	if(has_focus && sf::Keyboard::isKeyPressed(sf::Keyboard::Escape)) {
		window.close();
	}
	*/


	sf::Time delta_t=clock.getElapsedTime();
	clock.restart();
	long delta=delta_t.asMilliseconds();

	measure_fps_count++;
	measure_fps_time+=delta;
	if(measure_fps_time>1000) {
		printf("%ld frames in %ld ms: %ld FPS\n",
			measure_fps_count,
			measure_fps_time,
			measure_fps_count*1000/measure_fps_time);
		measure_fps_count=0;
		measure_fps_time=0;
	}

	menu_root->frame(delta);

	if(grab_mouse_on && has_focus) {
		sf::Mouse::setPosition(game_size/2,window);
	}

	//effects
	if(!game->timeout_player_damage.ready()) {
		game->timeout_player_damage.frame(delta);
/*
		if(rtt_enabled) {
			GameRes::shader_damage.setParameter("time",game->game_clock.getElapsedTime().asSeconds()*0.8f);
			GameRes::shader_damage.setParameter("amount",0.01f*(1.0-game->timeout_player_damage.get_progress()));
			GameRes::shader_damage.setParameter("freq",30.0f);
		}
*/
	}
	if(game->effect_blast_on) {
		game->effect_blast_anim+=(float)delta*0.001f;
		game->effect_blast_on=(game->effect_blast_anim<=1.0f);
//		GameRes::shader_blast.setParameter("anim",game->effect_blast_anim);
	}


	std::vector<sf::Shader*> effects;
	if(rtt_enabled && shader_enabled) {
		if(!game->timeout_player_damage.ready()) {
			effects.push_back(&GameRes::shader_damage);
		}
		if(game->effect_blast_on) {
			effects.push_back(&GameRes::shader_blast);
		}
	}

	if(!effects.empty()) {

		sf::RenderTexture* target[]={&rtt,&rtt2};
		int target_i=0;

		target[target_i]->clear(clear_color);
		node_root->render(NodeState(*target[target_i]));
		target[target_i]->display();

		GameRes::shader_damage.setParameter("time",game->game_clock.getElapsedTime().asSeconds()*0.8f);
		GameRes::shader_damage.setParameter("amount",0.01f*(1.0-game->timeout_player_damage.get_progress()));
		GameRes::shader_damage.setParameter("freq",30.0f);
		GameRes::shader_blast.setParameter("anim",game->effect_blast_anim);

		for(int i=0;i<effects.size();i++) {

			const sf::Texture& texture=target[target_i]->getTexture();
			sf::Sprite sprite(texture);

			if(i==effects.size()-1) {
				window.draw(sprite,effects[i]);
				window.display();
			}
			else {
				target_i=(target_i+1)%2;
				target[target_i]->draw(sprite,effects[i]);
				target[target_i]->display();
			}
		}
	}
	else {
		window.clear(clear_color);
		node_root->render(NodeState(window));
		window.display();
	}


}


Decal::Decal() {
	life=1000.0f;
	wind_speed=0;
	alpha_fade=0;
}
void Decal::draw(const NodeState& state) {
	sf::RenderStates mystate=state.render_state;
	mystate.transform.translate(pos);
	state.render_target->draw(sprite,mystate);
}
void Decal::update(float delta) {
	life-=delta;
	pos+=vel*delta;
	pos-=game->player->vel*wind_speed*delta;

	if(life<alpha_fade) {
		float a=std::max(0.0f,life/alpha_fade);
		sprite.setColor(sf::Color(255,255,255,a*255));
	}
}

Game::Game() {
	player_scale=0.7f;
	mouse_sensitivity=0.02f;
	input_enabled=true;
}
Game::~Game() {

}

void Game::on_resized() {
	//g_global.framework->init_rtt(size.x,size.y);

	float min_size=Utils::minf(size.x,size.y);
	float s=min_size/1000.0f;
	scale.x=scale.y=s;
}


void Game::init() {

	node_root=new Node();
	node_action=new Node();

	node_ships=new Node();
	node_bullets=new Node();
	node_anims=new Node();
	node_menu=new Node();

	bg=new BGManager();
	player=new Player();
	player->game=this;
	player->on_spawn();

	terrain=new Terrain();

//	menu=new MenuMain();
//	menu->resize(sf::Vector2f(game_size.x,game_size.y));

	node_root->add_child(bg);
	node_root->add_child(node_action);
	node_action->add_child(terrain);
	node_action->add_child(node_bullets);
	node_action->add_child(player);
	node_action->add_child(node_ships);
	node_action->add_child(node_anims);
	node_root->add_child(new GameHUD(this));
//	node_root->add_child(node_menu);
//	node_menu->add_child(menu);

	//node_ships->childs.push_back(player);

	timeout_player_damage.set(350);

	add_child(node_root);
}


void Game::set_effect_blast(bool on,float anim) {
	effect_blast_on=on;
	effect_blast_anim=anim;
}

void Game::start_level() {
	spawner=new Spawner(this);
	spawner->start();
	Audio::play_music("noise_attack.ogg");
	Audio::play_sound("can.ogg");

	set_effect_blast(false,0.0f);
}

void Game::update(long delta) {

	//printf(".\n");


	if(input_enabled) {
	//input
		player->ctrl_fire=sf::Mouse::isButtonPressed(sf::Mouse::Left) || sf::Mouse::isButtonPressed(sf::Mouse::Right);
		player->weapon_slot=(sf::Mouse::isButtonPressed(sf::Mouse::Left) ? 0 : 1);

		sf::Vector2f mouse_delta=(Utils::vec_to_f(sf::Mouse::getPosition(g_global.framework->window))-size/2.0f)*mouse_sensitivity;
		player->move(mouse_delta*0.01f);
	}

	DEBUG_P();

//	menu->frame(delta);
	spawner->update(delta);

	//update physics
	player->render_state.transform=sf::Transform::Identity;
	player->frame(delta);

	//printf("delta %ld\n",delta);

	ship_bullets_collision(player,&bullets,1,delta);

	for(int i=0;i<decals.size();i++) {
		decals[i]->update(delta);
		if(decals[i]->life<=0) {
			decals[i]->remove();
			delete(decals[i]);
			decals.erase(decals.begin()+i);
			i--;
			continue;
		}
	}

	for(int i=0;i<enemies.size();i++) {
		Ship* e=enemies[i];

		e->prev_pos=e->pos;
		e->frame(delta);
		e->update_bounding_box();

		if(e->entered) {
			ship_terrain_collision(e);

			if(e->team==TEAM_ENEMY) {
				ship_ship_collision(player,e);
				ship_bullets_collision(e,&bullets,0,delta);
			}
			else {
				ship_bullets_collision(e,&bullets,1,delta);
			}
		}
		else {
			if(!terrain->check_collision(Utils::vec_center_rect(e->pos,e->getScaledSize()))) {
				e->entered=true;
			}
		}

		if(e->health<=0 && e->can_remove) {

			enemies.erase(enemies.begin()+i);

			//node_ships->childs.erase(node_ships->childs.begin()+i);
			e->remove();

			//add_anim(GameRes::anim_explosion,e->pos);	//skip explosion sound
			add_explosion(e->pos,false);

			if(camera.rect.contains(e->pos)) {
				Audio::play_sound("explosion_big.ogg");
			}

			for(int b=0;b<bullets.size();b++) {
				if(bullets[b]->target==e) {
					bullets[b]->target=get_target(bullets[b]->pos);
				}
			}

			delete(e);
			i--;
		}

	}
	for(int i=0;i<node_anims->childs.size();i++) {
		SpriteAnim* anim=(SpriteAnim*)node_anims->childs[i];

		anim->update(delta);
		if(anim->done) {
			//node_anims->childs.erase(node_anims->childs.begin()+i);
			anim->remove();
			delete(anim);
			i--;
		}
	}

	sf::FloatRect bullet_dmg;
	bullet_dmg.width=50;
	bullet_dmg.height=50;

	for(int i=0;i<bullets.size();i++) {
		bool remove=false;
		Bullet* b=bullets[i];

		b->update(delta);

		if(b->life<=0) {
			remove=true;
		}
		if(b->is_point && terrain->check_collision(b->pos)) {
			bullet_dmg.left=b->pos.x-(bullet_dmg.width/2);
			bullet_dmg.top=b->pos.y-(bullet_dmg.height/2);
			terrain->damage_area(bullet_dmg);

			add_explosion(b->pos);

			remove=true;
		}

		if(remove) {
			bullets.erase(bullets.begin()+i);
			//node_bullets->childs.erase(node_bullets->childs.begin()+i);
			b->remove();

			if(b->explode_on_death) {
				add_explosion(b->pos);
			}

			delete(b);
			i--;
		}
	}

	sf::Vector2f scaledsize=scaled_size();
	camera.pos=player->render_state.transform.transformPoint(0,0);
	camera.rect=sf::FloatRect(camera.pos.x-scaledsize.x/2.0f,camera.pos.y-scaledsize.y/2.0f,scaledsize.x,scaledsize.y);

	bg->frame(camera.pos,camera.rect);
	terrain->update_visual(camera.rect);

	//player->sprite.setScale(player_scale,player_scale);
	player->render_state.transform.scale(player_scale,player_scale);

	float player_size=80*player_scale;
	player->size=sf::Vector2f(player_size,player_size);
	player->update_bounding_box();

	ship_terrain_collision(player);

	if(player->hit) {
		player->hit=false;

		timeout_player_damage.reset();
	}

	node_action->render_state.transform=sf::Transform::Identity;
	node_action->render_state.transform.translate(-camera.pos+size*0.5f/scale.x);

}

void Game::ship_ship_collision(Ship* s1,Ship* s2) {
	if(!s1->bounding_box.intersects(s2->bounding_box)) {
		return;
	}

	sf::Vector2f norm=Utils::vec_normalize(s1->pos-s2->pos)*0.1f;

	//float vel1=Utils::vec_len(s1->vel);
	//float vel2=Utils::vec_len(s2->vel);
	//s1->vel=norm*vel1*0.2f;
	//s2->vel=-norm*vel2*0.2f;

	bool c1=s1->on_collide_ship(s2);
	bool c2=s2->on_collide_ship(s1);

	if(s1->hurt_by_ship && !s1->shield_on) {
		s1->damage(10.0f);
	}
	if(s2->hurt_by_ship && !s2->shield_on) {
		s2->damage(10.0f);
	}

	if(!c1) {
		s1->vel+=norm;
	}
	if(!c2) {
		s2->vel-=norm;
	}
}
void Game::ship_terrain_collision(Ship* ship) {
	if(!terrain->check_collision(ship->bounding_box)) {
		return;
	}
	sf::FloatRect damage_rect=Utils::vec_center_rect(ship->pos,ship->getScaledSize()*1.5f);
	terrain->damage_area(damage_rect);

	if(!ship->shield_on) {
		ship->vel*=-0.2f;
		ship->damage(5);
	}
}
void Game::ship_bullets_collision(Ship* ship,std::vector<Bullet*>* vect,int bullet_type,float delta) {
	for(int i=0;i<vect->size();i++) {
		Bullet* b=vect->at(i);
		if(b->type!=bullet_type) continue;

		if(ship->shield_on) {
			if(b->is_point) {
				if(Utils::vec_len_fast(b->pos-ship->pos)<ship->shield_size*ship->shield_size) {
					b->life=0;
				}
			}
		}
		else {
			if(b->is_point) {
				if(ship->bounding_box.contains(b->pos)) {
					b->check_ship(ship,delta);
				}
			}
			else {
				b->check_ship(ship,delta);
			}
		}
	}
}

void Game::add_bullet(sf::Vector2f pos,sf::Vector2f vel,int type) {
	Bullet* b=new Bullet();
	b->pos=pos;
	b->vel=vel;
	b->type=type;
	b->sprite.setRotation(Utils::vec_angle(vel)*180/M_PI-90);

	add_bullet(b);
}
void Game::add_bullet(Bullet* b) {
	b->game=this;
	bullets.push_back(b);

	if(b->on_top) {
		node_ships->add_child(b);
	}
	else {
		node_bullets->add_child(b);
	}

	if(camera.rect.contains(b->pos)) {
		Audio::play_sound("gun.ogg");
	}
}

/*
void Game::spawn_enemy(sf::Vector2f pos,int type) {
	Enemy* e;

	switch(type) {
	case 1:
		e=new EnemyParent();
		break;
	case 2:
		e=new EnemyGrenade();
		break;
	default:
		e=new Enemy();
		break;
	}

	e->game=this;
	e->pos=pos;
	e->team=type;
	e->set_texture(Utils::vector_rand(GameRes::enemies));
	e->size=sf::Vector2f(50,50);
	enemies.push_back(e);
	node_ships->childs.push_back(e);
}
*/
void Game::spawn_ship(Ship* e) {
	e->game=this;
	e->prev_pos=e->pos;
	enemies.push_back(e);
//	node_ships->childs.push_back(e);
	node_ships->add_child(e);

	e->on_spawn();
}
void Game::add_anim(SpriteAnimData* data,sf::Vector2f pos) {
	SpriteAnim* anim=new SpriteAnim();
	anim->load(data);
	anim->render_state.transform.translate(pos);
	//node_anims->childs.push_back(anim);
	node_anims->add_child(anim);
}
void Game::add_explosion(sf::Vector2f pos,bool sound) {
	add_anim(Utils::vector_rand(GameRes::anim_explosions),pos);

	if(sound && camera.rect.contains(pos)) {
		Audio::play_sound("explosion.ogg");
	}
}
void Game::add_decal(Decal* decal) {
	decal->game=this;
	decals.push_back(decal);
	node_bullets->add_child(decal);
}
void Game::add_splash(Decal* decal) {
	decal->game=this;
	decals.push_back(decal);
	node_root->add_child(decal);
}

Ship* Game::get_target(sf::Vector2f pos) {
	if(enemies.size()==0) return NULL;

	Ship* target=NULL;

	int start=0;
	float min_len;

	for(start=0;start<enemies.size();start++) {
		if(enemies[start]->team==TEAM_ENEMY) {
			target=enemies[start];
			min_len=Utils::vec_len_fast(pos-enemies[start]->pos);
			break;
		}
	}
	if(!target) return NULL;

	for(int i=start+1;i<enemies.size();i++) {
		if(enemies[i]->team!=TEAM_ENEMY) continue;

		float len=Utils::vec_len_fast(pos-enemies[i]->pos);
		if(len<min_len) {
			target=enemies[i];
			min_len=len;
		}
	}
	return target;
}
std::vector<Ship*> Game::get_targets(sf::Vector2f pos,int count) {
	std::vector<Ship*> ret;

	for(int c=0;c<count;c++) {
		float min_len=0;
		Ship* min=NULL;

		for(int i=0;i<enemies.size();i++) {
			if(enemies[i]->team!=TEAM_ENEMY) continue;

			bool ignore=false;
			for(int c2=0;c2<ret.size();c2++) {
				if(enemies[i]==ret[c2]) {
					ignore=true;
					break;
				}
			}
			if(ignore) continue;

			float len=Utils::vec_len_fast(pos-enemies[i]->pos);
			if(!min || len<min_len) {
				min=enemies[i];
				min_len=len;
			}
		}
		if(min) {
			ret.push_back(min);
		}
	}

	return ret;
}

GameHUD::GameHUD(Game* _game) {
	game=_game;
	const char* weapon_names[]={"Blaster","Mega Pulse","Swinger","Missiles","Follower","Shield"};

	for(int i=0;i<6;i++) {
		text_weapons[i].setFont(GameRes::font);
		text_weapons[i].setCharacterSize(20);
		text_weapons[i].setString(weapon_names[i]);
		text_weapons[i].setColor(sf::Color::White);
	}

	text_time.setFont(GameRes::font);
	text_time.setCharacterSize(30);
	text_time.setColor(sf::Color::Yellow);

}
void GameHUD::draw(const NodeState& state) {
	sf::RenderStates mystate=state.render_state;
	for(int i=0;i<6;i++) {
		mystate.transform=state.render_state.transform;
		mystate.transform.translate(30,(game->size.y-6*25-30)+i*25);
		text_weapons[i].setColor( game->player->weapon_i==i ? sf::Color::Yellow : sf::Color::White );

		sf::Vector2f charge_size(200,20);

		float line_x=game->player->getWeaponDrain(i)*charge_size.x;
		bool weapon_ready=(game->player->weapon_charge>game->player->getWeaponDrain(i));

		sf::Color line_color=(weapon_ready ? sf::Color(255,255,255) : sf::Color(255,0,0));
		sf::Color rect_color=(weapon_ready ? sf::Color(255,0,0) : sf::Color(180,0,0));

		sf::Vertex line[] = {
		    sf::Vertex(sf::Vector2f(line_x, 0),line_color),
		    sf::Vertex(sf::Vector2f(line_x, charge_size.y),line_color)
		};

		sf::RectangleShape charge_rect(sf::Vector2f(charge_size.x*game->player->weapon_charge,charge_size.y));
		charge_rect.setFillColor(rect_color);

		state.render_target->draw(line,2,sf::Lines,mystate);
		state.render_target->draw(charge_rect,mystate);
		state.render_target->draw(text_weapons[i],mystate);
	}

	static char buff[100];
	int seconds=game->spawner->ts/1000;
	sprintf(buff,"%02d:%02d",seconds/60,seconds%60);
	text_time.setString(buff);

	sf::FloatRect textRect = text_time.getLocalBounds();
	text_time.setOrigin(textRect.left + textRect.width/2.0f,textRect.top  + textRect.height/2.0f);
	//text_time.setPosition(sf::Vector2f(SCRWIDTH/2.0f,SCRHEIGHT/2.0f));
	mystate.transform=state.render_state.transform;
	mystate.transform.translate(game->size.x/2,game->size.y-75);
	state.render_target->draw(text_time,mystate);

	sf::Vector2f health_size(400,20);
	sf::Vector2f health_pos=sf::Vector2f((game->size.x-health_size.x)/2.0,game->size.y-40);


	sf::RectangleShape health_frame(health_size);
	sf::RectangleShape health_bar(sf::Vector2f(health_size.x*std::max(0.0f,game->player->health)/100.0,health_size.y));
	health_frame.setOutlineColor(sf::Color(255,0,0));
	health_bar.setFillColor(sf::Color(255,0,0));

	mystate.transform=state.render_state.transform;
	mystate.transform.translate(health_pos);
	state.render_target->draw(health_frame,mystate);
	state.render_target->draw(health_bar,mystate);
}




