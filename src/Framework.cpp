#include <stdio.h>

#include <SFML/OpenGL.hpp>
#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

#include "Framework.h"
#include "Node.h"
#include "Menu.h"
#include "Utils.h"

namespace {

class Renderer {

	class RenderState {
	public:
		sf::RenderTarget* target;
		Color color;
		sf::RenderStates sfml_state;
	};

	sf::Sprite tmp_sprite;
	sf::RectangleShape tmp_rectangle;

	void render_node(Node* node,const RenderState& state) {
		if(!node->visible) {
			return;
		}

		RenderState new_state=state;

		new_state.sfml_state.transform.translate(node->pos-node->origin);
		new_state.sfml_state.transform.scale(node->scale);
		new_state.sfml_state.transform.rotate(node->rotation,node->origin);
		new_state.color*=node->color;

		switch(node->type) {
		case Node::TYPE_NO_RENDER:
			break;
		case Node::TYPE_TEXTURE:
			if(node->texture.tex) {
				tmp_sprite.setTexture(*node->texture.tex,false);
				tmp_sprite.setTextureRect(node->texture.rect);
				tmp_sprite.setColor(new_state.color.sfColor());
				new_state.target->draw(tmp_sprite,new_state.sfml_state);
			}
			break;
		case Node::TYPE_SOLID:
			tmp_rectangle.setFillColor(new_state.color.sfColor());
			new_state.target->draw(tmp_rectangle,new_state.sfml_state);
			break;
		case Node::TYPE_TEXT:
			node->text.setColor(new_state.color.sfColor());
			new_state.target->draw(node->text,new_state.sfml_state);
			break;
		}
		for(Node* child : node->children) {
			render_node(child,new_state);
		}
	}

public:

	Renderer() {
		tmp_rectangle.setSize(sf::Vector2f(1,1));
	}

	void resize(sf::Vector2f _size) {

	}
	void render(sf::RenderTarget* _target,Node* node) {
		RenderState state;
		state.target=_target;
		render_node(node,state);
	}
};

class MenuRoot : public Menu {
public:
	Framework* owner;

	MenuRoot() {
		owner=NULL;
	}

	virtual void event_resize() override {
		for(Menu* child : children) {
			child->resize(size);
		}
	}
	virtual bool event_message(int msg) override {
		if(owner && msg==owner->MESSAGE_QUIT) {
			owner->quit();
		}
		return true;
	}
};

}


class Framework::Impl {

public:

	sf::RenderWindow window;
	sf::Clock clock;
	Renderer renderer;
	MenuRoot menu_root;

	bool has_focus;
	bool shader_enabled;
	bool rtt_enabled;
	bool do_run;

	Impl() {
		do_run=false;
	}
	~Impl() {
	}

	static void center_window(sf::Window& win) {
		sf::Vector2i desktop_size=sf::Vector2i(sf::VideoMode::getDesktopMode().width,sf::VideoMode::getDesktopMode().height);
		sf::Vector2i win_size=Utils::vec_to_i(win.getSize());
		win.setPosition((desktop_size-win_size)/2);
	}

	void init_window(int w,int h) {
		window.setVerticalSyncEnabled(true);
		window.setFramerateLimit(80);

		window.create(sf::VideoMode(w,h), "BGA"/*,sf::Style::None*/);
		center_window(window);
		has_focus=true;

		shader_enabled=sf::Shader::isAvailable();
		rtt_enabled=false;

		sf::ContextSettings settings = window.getSettings();
		printf("OpenGL version: %d.%d\n",settings.majorVersion,settings.minorVersion);
		printf("RTT supported: %d\n",rtt_enabled);
		printf("shader supported: %d\n",shader_enabled);
	}

	void window_resized(sf::Vector2f size) {
		sf::View view=sf::View(sf::FloatRect(0.f, 0.f,size.x,size.y));
		window.setView(view);

		menu_root.resize(size);
		renderer.resize(size);
	}
	void forward_event(const sf::Event& event) {
		menu_root.send_event(MenuEvent(event).adapted(&menu_root));
	}

	void run() {
		init_window(1600,960);
		window_resized(Utils::vec_to_f(window.getSize()));

		do_run=true;
		while(do_run && window.isOpen()) {
			frame();
		}
	}
	void frame() {
		sf::Event event;
		while(window.pollEvent(event)) {

			//menu_root->event(event);

			//tweakers
	//		tweaker_event(event);

			switch(event.type) {
				case sf::Event::Closed:
					window.close();
					break;
				case sf::Event::Resized:
					//event.size.width,event.size.height
					window_resized(Utils::vec_to_f(window.getSize()));
					break;
				case sf::Event::LostFocus:
					has_focus=false;
					break;
				case sf::Event::GainedFocus:
					has_focus=true;
					break;
				case sf::Event::KeyPressed:
					/*
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
					*/
				case sf::Event::MouseMoved:
				case sf::Event::MouseButtonPressed:
				case sf::Event::MouseButtonReleased:
					forward_event(event);
					break;
				default:
					break;

			}
		}

		/*
		if(rtt.getSize()!=window.getSize()) {
			int w=window.getSize().x;
			int h=window.getSize().y;
			rtt_enabled=(rtt.create(w,h,false) && rtt2.create(w,h,false));
			printf("reinited rtt to %d %d\n",w,h);
		}

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
		*/


		sf::Time delta_t=clock.getElapsedTime();
		clock.restart();
		long delta=delta_t.asMilliseconds();

		/*
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
		 */

		menu_root.frame(delta);

		window.clear(sf::Color::Black);
		renderer.render(&window,&menu_root.node);
		window.display();
	}

};

Framework::Framework() {
	impl=new Impl();
	impl->menu_root.owner=this;
}
Framework::~Framework() {
	delete(impl);
}
void Framework::run() {
	impl->run();
}
void Framework::quit() {
	impl->do_run=false;
}
Menu* Framework::get_root_menu() {
	return &impl->menu_root;
}
int Framework::MESSAGE_QUIT=0;
