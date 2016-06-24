#ifndef _Game_H_
#define _Game_H_

#include <SFML/Graphics.hpp>

#include "Node.h"
#include "BGManager.h"
#include "Terrain.h"
#include "Spawner.h"
#include "Utils.h"
#include "Anim.h"
#include "Ship.h"
#include "Menu.h"

#include <vector>
#include <string>
#include <memory>

class Camera {
public:
	sf::Vector2f pos;
	sf::FloatRect rect;
};

class Decal : public Node {
public:
	Decal();
	virtual void draw(const NodeState& state);
	virtual void update(float delta);

	Game* game;
	sf::Sprite sprite;
	sf::Vector2f pos;
	sf::Vector2f vel;
	float life;

	float wind_speed;	//how much pos is affected by player speed (for splash)
	float alpha_fade;	//at what life alpha starts to fade
};


class Game : public Menu {

	void ship_terrain_collision(Ship* ship);
	void ship_ship_collision(Ship* s1,Ship* s2);
	void ship_bullets_collision(Ship* s1,std::vector<Bullet*> *vect,int bullet_type,float delta);

	void remove_bullet(int index);

public:

	bool input_enabled;

	bool effect_blast_on;
	float effect_blast_anim;

	Spawner* spawner;

	Node* node_root;
	Node* node_action;

	Node* node_ships;
	Node* node_bullets;
	Node* node_anims;
	Node* node_menu;

	BGManager *bg;
	Player* player;
	Terrain* terrain;

	Camera camera;

	sf::Clock game_clock;

	Timeout timeout_player_damage;

	float player_scale;
	float mouse_sensitivity;

	std::vector<Bullet*> bullets;
	std::vector<Ship*> enemies;
	std::vector<Ship*> friends;
	std::vector<Decal*> decals;

	std::string current_level;

	Game();
	~Game();
	void init();
	void start_level();
	void update(long delta);
	void render();
	void add_bullet(sf::Vector2f pos,sf::Vector2f vel,int type);
	void add_bullet(Bullet* b);
	//void spawn_enemy(sf::Vector2f pos,int type);
	void spawn_ship(Ship* e);
	void add_anim(SpriteAnimData* data,sf::Vector2f pos);
	void add_explosion(sf::Vector2f pos,bool sound=true);
	void add_decal(Decal* decal);
	void add_splash(Decal* splash);

	Ship* get_target(sf::Vector2f pos);
	std::vector<Ship*> get_targets(sf::Vector2f pos,int count);	//can return less items than count

	void set_effect_blast(bool on,float anim);

	void on_resized();
};

class GameHUD : public Node {
	Game* game;

	sf::Text text_weapons[6];
	sf::Text text_time;

public:
	GameHUD(Game* _game);
	void draw(const NodeState& state);
};

class GameFramework {
	sf::Clock clock;
	long measure_fps_count;
	long measure_fps_time;
	sf::Color clear_color;

	Node* node_root;

	bool grab_mouse_on;

public:
	sf::RenderWindow window;
	sf::RenderTexture rtt;
	sf::RenderTexture rtt2;
	bool rtt_enabled;

	bool shader_enabled;

	sf::View window_view;
	sf::Vector2i game_size;
	bool has_focus;

	std::auto_ptr<Menu> menu_root;
	Game* game;

	GameFramework();
	~GameFramework();

	void create_window(int w,int h);
	//void init_rtt(int w,int h);
	void init_resources();

	void frame();

	void start_level(std::string level);
	void grab_mouse(bool grab);
};


#endif
