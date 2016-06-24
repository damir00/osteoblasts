#ifndef _Ship_H_
#define _Ship_H_

#include <SFML/Graphics.hpp>

#include "Node.h"
#include "Utils.h"
#include "Anim.h"
#include "GameRes.h"

class Game;
class Ship;

enum Team { TEAM_PLAYER, TEAM_ENEMY };
enum ShipGunAimType { GUN_AIM_TYPE_FIXED,GUN_AIM_TYPE_AIM,GUN_AIM_TYPE_SPIRAL };


class Bullet : public Node {
public:
	sf::Vector2f pos;
	sf::Vector2f vel;
	float size;
	long life;
	int type;
	float damage;
	bool is_point;	//for collision, if it's a point or a complex geom

	bool on_top;	//place on top layer
	bool explode_on_death;

	sf::Sprite sprite;

	Game* game;
	Ship* target;

	virtual void update(float delta);
	Bullet();
	void autorotate();
	virtual ~Bullet() {}
	virtual void draw(const NodeState& state);
	virtual sf::FloatRect get_bb();
	virtual void check_ship(Ship* s,float delta);
};


class ShipGun {
public:
	sf::Vector2f pos;
	float lat_spread;

	float angle;
	float angle_spread;

	ShipGunAimType aim_type;
	float spiral_speed;
	bool use_master_timer;

	float bullet_speed;
	sf::Texture* bullet_texture;

	static ShipGun fixed(sf::Vector2f pos,float angle,float bullet_speed,sf::Texture* texture);
	static ShipGun spiral(sf::Vector2f pos,float spiral_speed,float bullet_speed,sf::Texture* texture);
};


class ShipController {
public:
	Ship* ship;
	virtual ~ShipController() {}
	virtual void start() {}
	virtual void frame(float delta) {}

	virtual ShipController* clone(ShipController* new_instance=NULL) {
		ShipController* c=(new_instance ? new_instance : new ShipController());
		*c=*this;
		return c;
	}
};

class BlinkCurve {
public:
	int count;
	float interval;
	float position;

	BlinkCurve() {
		count=1;
		interval=200;
		end();
	}
	void reset() {
		position=0;
	}
	void end() {
		position=count*interval+1;
	}
	void update(float delta) {
		position+=delta;
	}
	bool get() {
		if(done()) {
			return false;
		}
		return ((int)position%(int)interval)<(interval/2);
	}
	bool done() {
		return (position>=(float)count*interval);
	}
};

class Ship : public Node {

protected:
	//big death explosion
	Atlas* death_atlas;
	bool death_active;
	int death_total_explosion_count;
	int death_explosion_count;
	Timeout death_explosion_timeout;

	void set_death_atlas(Atlas* atlas);

public:
	Game* game;

	bool hit;	//was hit in the last update
	bool can_remove;	//can game remove when life<0
	bool blink_enabled;	//damage blink

	BlinkCurve blink_damage;

	ShipController* controller;

	sf::Sprite sprite;
	sf::Vector2f size;
	sf::Vector2f pos;
	sf::Vector2f prev_pos;
	sf::Vector2f vel;
	sf::Vector2f acc;
	sf::Vector2f ctrl_move;
	//float speed;
	float max_health;	//calculated on_spawn
	float health;
	Team team;
	bool entered;	//passes trough terrain until entered. when not touching terrain, entered is true

	float damage_mult;

	float max_acceleration;	//pix/sec2
	float max_speed;		//pix/sec

	//Timeout timeout_fire;
	FireTimeout fire_timeout;

	sf::FloatRect bounding_box;

	SpriteAnim anim;
	bool anim_is_fire;
	StickySwitch switch_anim_fire;

	sf::Texture* projectile_texture;

	bool ctrl_fire;
	float circle_anim;

	bool hurt_by_terrain;
	bool hurt_by_ship;
	bool hurt_by_bullet;

	bool shield_on;
	float shield_size;

	float scale;

	//weapons
	std::vector<ShipGun> guns;
	sf::Vector2f find_attack_vector(float dist);


	void update_bounding_box();
	Ship();
	virtual ~Ship();
	virtual void draw(const NodeState& state);
	virtual void frame(float delta);
	void set_texture(const sf::Texture* tex);
	void set_anim(SpriteAnimData* _anim,bool _is_fire);
	void move(sf::Vector2f x);
	virtual void fire();
	virtual bool on_collide_ship(Ship* s) { return false; }
	virtual void on_spawn();
	virtual void on_damage(float dmg) {}

	void damage(float dmg);
	void setScale(float _scale);
	sf::Vector2f getScaledSize() {
		return size*scale;
	}

	virtual Ship* clone(Ship* new_instance=NULL) {
		Ship* s=(new_instance ? new_instance : new Ship());
		*s=*this;
		if(controller) {
			s->steal_controller();
			s->set_controller(controller->clone());
		}
		return s;
	}
	void set_controller(ShipController* _controller) {
		if(controller) {
			delete(controller);
		}
		controller=_controller;
		if(!controller) {
			return;
		}
		controller->ship=this;
		controller->start();
	}
	ShipController* steal_controller() {
		ShipController* c=controller;
		controller=0;
		return c;
	}
};



class Drone : public Ship {
public:
	Drone();
	void frame(float delta);
	void fire();

	virtual Ship* clone(Ship* new_instance=NULL) {
		Drone* d=(new_instance ? (Drone*)new_instance : new Drone());
		return Ship::clone(d);
	}
};

class Player : public Ship {

	void fire_blaster(int count,float angle_spread,float angle_offset);

	Timeout timeout_shinning;

	//SpriteAnim anim_hammer;

	Timeout hammer_timeout;

	SpriteAnim anim_shinning;
	SpriteAnim anim_engine_fire;
	SpriteAnim anim_shield;
	sf::Sprite sprite_hammer;
	sf::Sprite sprite_hammer_swing;
	StickySwitch engine_fire_visible;

	sf::Sprite sprite_shield_ring[3];

	bool hammer_on;

	bool prev_shield_enabled;
	Tracer tracer_shield;

	float weapon_drain[10];

public:

	int weapon_i;
	int weapon_slot;
	float weapon_charge;	//0-1

	Player();
	virtual void fire();
	virtual void frame(float delta);
	virtual void draw(const NodeState& state);

	float getWeaponCharge(int i);
	float getWeaponDrain(int i);
	//bool getWeaponDrainContinious(int i);
};



class EnemyKamikaze: public Ship {
protected:
	bool inited;
	sf::Vector2f vec;
public:
	EnemyKamikaze();
	void frame(float delta);

	virtual Ship* clone(Ship* new_instance=NULL) {
		EnemyKamikaze* d=(new_instance ? (EnemyKamikaze*)new_instance : new EnemyKamikaze());
		return Ship::clone(d);
	}
};


class EnemyMelee: public EnemyKamikaze {
protected:
	bool retracting;
public:
	EnemyMelee();
	void frame(float delta);
	bool on_collide_ship(Ship* s);

	virtual Ship* clone(Ship* new_instance=NULL) {
		EnemyMelee* d=(new_instance ? (EnemyMelee*)new_instance : new EnemyMelee());
		return Ship::clone(d);
	}
};

/*
class EnemyShooter: public Ship {
public:
	std::vector<ShipGun> guns;

	EnemyShooter();
	sf::Vector2f find_attack_vector(float dist);
	void frame(float delta);
	void fire();
};
*/

class EnemyParent: public Ship {
public:

	int child_type;

	EnemyParent();
	void frame(float delta);
	void fire();
};


//bosses

class BossPlatypus : public Ship {

	SpriteAnim anim_engine;
	SpriteAnim anim_appear;
	SpriteAnim anim_dissappear;
	SpriteAnim anim_bellyshot;

	SpriteAnim* current_platypus_anim;

	Timeout timeout_teleport;
	Timeout timeout_launch;
	Timeout timeout_bellyshot;
	bool bellyshot_active;
	int launch_count;

	float platypus_pos;

	int phase;

	sf::Vector2f anim_offset;

	void draw_anim(const NodeState& state,SpriteAnim& anim,sf::Vector2f pos);
	void set_anim(SpriteAnim* anim);

public:
	BossPlatypus();

	virtual void frame(float delta);
	virtual void draw(const NodeState& state);

	virtual Ship* clone(Ship* new_instance=NULL) {
		BossPlatypus* s=(new_instance ? (BossPlatypus*)new_instance : new BossPlatypus());
		return Ship::clone(s);
	}
};
class BossWalrus : public Ship {
	SpriteAnim hull;
	Timeout timeout_death;
	Timeout timeout_death_explosion;
	bool death_active;

public:
	int stage;

	BossWalrus();

	virtual void frame(float delta);
	virtual void draw(const NodeState& state);
	virtual void on_damage(float dmg);

	virtual Ship* clone(Ship* new_instance=NULL) {
		BossWalrus* s=(new_instance ? (BossWalrus*)new_instance : new BossWalrus());
		*s=*this;
		Ship::clone(s);
		return s;
	}
};

class BossLuna : public Ship {
	SpriteAnim hull;

	bool rocket_fired;
	Timeout junior_timeout;

	Timeout laser_start_timeout;
	Timeout laser_fire_timeout;
	Bullet* laser;

public:

	BossLuna();

	virtual void frame(float delta);
	virtual void draw(const NodeState& state);
	virtual void on_damage(float dmg);

	virtual Ship* clone(Ship* new_instance=NULL) {
		BossLuna* s=(new_instance ? (BossLuna*)new_instance : new BossLuna());
		*s=*this;
		Ship::clone(s);
		return s;
	}
};
class BossOctopuss : public Ship {
	SpriteAnim hull;
	Timeout fire_timeout;
public:
	BossOctopuss();
	virtual void frame(float delta);
	virtual void draw(const NodeState& state);
	//virtual void on_damage(float dmg);

	virtual Ship* clone(Ship* new_instance=NULL) {
		BossOctopuss* s=(new_instance ? (BossOctopuss*)new_instance : new BossOctopuss());
		*s=*this;
		Ship::clone(s);
		return s;
	}

};

//controllers
class ControllerShooter : public ShipController {

public:
	float dist;
	float circle_speed;

	void start() {
		dist=Utils::rand_range(200,300);
		circle_speed=Utils::rand_range(-0.001,0.001);
	}
	void frame(float delta) {

		ship->ctrl_fire=true;
		sf::Vector2f target=ship->find_attack_vector(dist);

		ship->circle_anim+=circle_speed*delta;
		sf::Vector2f vec=Utils::vec_normalize(target-ship->pos)*0.5f;
		ship->move( (vec-ship->vel)*0.5f );
	}

	virtual ShipController* clone(ShipController* new_instance=NULL) {
		ControllerShooter* c=(new_instance ? (ControllerShooter*)new_instance : new ControllerShooter());
		//return ShipController::clone(c);
		*(ControllerShooter*)c=*this;
		return c;
	}
};





#endif
