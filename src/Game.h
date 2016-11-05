
#ifndef _BGA_GAME_H_
#define _BGA_GAME_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <functional>
#include <sstream>
#include <array>

#include <SFML/System.hpp>

#include "Utils.h"
#include "Loader.h"
#include "Menu.h"
#include "SpaceBackground.h"
#include "Terrain.h"
#include "Quad.h"
#include "SimpleList.h"
#include "Easing.h"

//utils


class ProgressBar : public Menu {
	void update() {
		frame[0].scale=sf::Vector2f(size.x,border);
		frame[2].scale=sf::Vector2f(size.x,border);
		frame[1].scale=sf::Vector2f(border,size.y);
		frame[3].scale=sf::Vector2f(border,size.y);

		frame[1].pos=sf::Vector2f(size.x-border,0);
		frame[2].pos=sf::Vector2f(0,size.y-border);

		fill.scale=sf::Vector2f(Utils::clamp(0,size.x,size.x*progress),size.y);
	}

	float border;
	float progress;

public:
	std::array<Node,4> frame;
	Node fill;

	ProgressBar() {
		border=2.0f;
		progress=0.0f;

		for(int i=0;i<4;i++) {
			frame[i].type=Node::TYPE_SOLID;
			node.add_child(&frame[i]);
		}
		fill.type=Node::TYPE_SOLID;
		node.add_child(&fill);
	}
	void set_progress(float _progress) {
		progress=_progress;
		update();
	}
	void set_border(float _border) {
		border=_border;
		update();
	}
	void event_resize() override {
		update();
	}
};

class SimpleTimer {
public:
	float current;
	float max;
	SimpleTimer() {
		reset(1.0);
	}
	SimpleTimer(float _max) {
		reset(_max);
	}
	void reset() {
		current=0.0;
	}
	void reset(float _max) {
		current=0.0;
		max=_max;
	}
	void update(float dt) {
		current+=dt;
	}
	bool is_done() {
		return (current>=max);
	}
	float get_percentage() {
		return current/max;
	}
};
class EventTimer {
public:
	float current;
	float duration;
	bool repeat;

	std::vector<std::pair<float,int> > events;
	SimpleList<int> event_list;

	EventTimer() {
		current=0;
		duration=1;
		repeat=false;
	}
	void add_event(float time,int event) {
		events.push_back(std::make_pair(time,event));
	}

	bool is_done() {
		return (current>=duration);
	}
	float get_percentage() {
		return Utils::clamp(0.0f,1.0f,current/duration);
	}
	void reset() {
		current=0;
	}
	void reset(float _duration) {
		duration=_duration;
		current=0;
	}
	void end() {
		current=duration;
	}

	void update(float dt) {
		event_list.clear();
		float t_new=current+dt;
		for(const std::pair<float,int>& e : events) {
			if(e.first>=current && e.first<t_new) {
				event_list.push_back(e.second);
			}
		}

		current=t_new;
		if(repeat) {

			if(current>=duration) {
				for(const std::pair<float,int>& e : events) {
					if(e.first>=current) {
						event_list.push_back(e.second);
					}
				}
			}

			current=fmod(current,duration);
		}
	}
};

class Animation {
public:
	Texture texture;
	std::vector<sf::IntRect> frames;
	int frame_w;
	int frame_h;
	float delay;	//seconds
	float duration;

	Animation() {
		frame_w=0;
		frame_h=0;
		delay=0;
		duration=0;
	}

	Animation(const Texture& tex,int _frame_w,int _frame_h,float _delay) {
		set(tex,_frame_w,_frame_h,_delay);
	}
	void add_empty_frame() {
		frames.push_back(sf::IntRect(0,0,0,0));
		duration=frames.size()*delay;
	}
	void set(const Texture& tex,int _frame_w,int _frame_h,float _delay) {
		texture=tex;
		frame_w=_frame_w;
		frame_h=_frame_h;
		delay=_delay;
		frames.clear();

		if(tex.tex==nullptr) {
			return;
		}

		int x=tex.rect.left;
		int y=tex.rect.top;

		while(y+frame_h<=tex.rect.top+tex.rect.height) {
			while(x+frame_w<=tex.rect.left+tex.rect.width) {
				frames.push_back(sf::IntRect(x,y,frame_w,frame_h));
				x+=frame_w;
			}
			x=0;
			y+=frame_h;
		}
		duration=frames.size()*delay;
	}
	Texture get_texture(int frame) const {
		if(frames.empty()) {
			return texture;
		}

		Texture t=texture;
		t.rect=frames[frame%frames.size()];
		if(t.rect.width==0 || t.rect.height==0) {
			return Texture();
		}
		return t;
	}

};

//animation, texture or something else
class Graphic {
public:
	Texture texture;
	Animation animation;

	Graphic() {}
	Graphic(const Texture& tex) {
		set_texture(tex);
	}
	Graphic(const Animation& anim) {
		set_animation(anim);
	}
	void set_texture(const Texture& tex) {
		texture=tex;
		animation=Animation();
	}
	void set_animation(const Animation& anim) {
		animation=anim;
		texture=Texture();
	}
	bool is_animated() const {
		return (texture.tex==NULL && animation.texture.tex!=nullptr);
	}
	Texture get_texture() const {
		if(texture.tex) {
			return texture;
		}
		return animation.get_texture(0);
	}
	Texture get_texture(int frame) const {
		if(texture.tex) {
			return texture;
		}
		return animation.get_texture(frame);
	}

};


class GraphicNode : public Node {

	void update_frame() {
		int next_frame=(anim_time/graphic.animation.delay);
		if(anim_current_frame!=next_frame) {
			texture=graphic.get_texture(next_frame);
			anim_current_frame=next_frame;
		}
	}

public:
	Graphic graphic;
	int anim_current_frame;
	float anim_time;
	bool repeat;
	float speed;
	float duration_hidden;		//after anim ends, hide it for this duration

	GraphicNode() {
		anim_time=0;
		anim_current_frame=0;
		repeat=false;
		speed=1.0;
		duration_hidden=0.0f;
	}
	GraphicNode(const Graphic& _graphic) {
		set_graphic(_graphic);
	}
	void set_graphic(const Graphic& _graphic) {
		graphic=_graphic;
		texture=graphic.get_texture();
		anim_time=0;
		anim_current_frame=0;
	}
	void set_animation_progress(float progress) { //0-1
		anim_time=graphic.animation.duration*progress;
		update_frame();
	}

	void update(float dt) {
		if(!graphic.is_animated()) {
			return;
		}
		if(graphic.animation.duration==0.0f) {
			return;
		}

		anim_time+=dt*speed;
		float total_duration=graphic.animation.duration+duration_hidden;

		if(repeat) {
			anim_time=fmod(anim_time,total_duration);
		}
		else {
			//anim_time=std::min(anim_time,graphic.animation.duration-0.001f);
			anim_time=std::min(anim_time,total_duration-0.001f);
		}

		if(anim_time>graphic.animation.duration) {
			//hidden
			visible=false;
		}
		else {
			visible=true;
			update_frame();
		}
	}
	typedef std::unique_ptr<GraphicNode> Ptr;
};



#define COMPONENT_INDEX_SIZE 10
#define ATTRIBUTE_INDEX_SIZE 10
class Entity;

class EntityRef {
public:
	Entity* entity;
	EntityRef() {
		entity=NULL;
	}
};

class Component {
public:

	enum Type {
		TYPE_SHAPE=COMPONENT_INDEX_SIZE,
		TYPE_DISPLAY,
		TYPE_HEALTH,
		TYPE_GUN,
		TYPE_TIMEOUT,
		TYPE_AI,
		TYPE_AI2,
		TYPE_ENGINE,
		TYPE_TELEPORTATION,
		TYPE_SHOW_DAMAGE,
		TYPE_BOUNCE,
		TYPE_FLAME_DAMAGE,
		TYPE_SHOW_ON_MINIMAP,
		TYPE_GRAVITY_FORCE,
		TYPE_SHIELD,
		TYPE_HAMMER,
		TYPE_SPLATTER,
		TYPE_SPAWN_PIECES,
		TYPE_STUN_BLAST,
		TYPE_PLATYPUS_BOSS,
		TYPE_FIGHTER_SHIP,
		TYPE_ELECTRICITY
	};
	enum Event {
		EVENT_FRAME,
		EVENT_DAMAGED
	};

	Entity* entity;
	Type type;

	std::vector<EntityRef*> my_refs;	//will automatically get released when comp is removed
	std::unordered_set<Event,std::hash<short> > events;

	Component() {
		entity=nullptr;
		type=TYPE_SHAPE;
	}
	virtual ~Component() {}
	virtual void insert() {}
	virtual void remove() {}
};

class CompShape : public Component {
public:
	std::vector<Quad> quads;

	enum CollisionGroup {
		COLLISION_GROUP_TERRAIN=1<<0,
		COLLISION_GROUP_PLAYER_BULLET=1<<1,
		COLLISION_GROUP_ENEMY_BULLET=1<<2,
		COLLISION_GROUP_PLAYER=1<<3,
		COLLISION_GROUP_ENEMY=1<<4,
		COLLISION_GROUP_GRAB=1<<5,	//for hook
	};

	uint8_t collision_group;
	uint8_t collision_mask;
	bool bounce;			//bounce off entities
	bool terrain_bounce;

	Quad bbox;
	bool enabled;

	float hit_damage;	//how much damage is done to the colliding entity
	float take_damage_terrain_mult;

	Texture hit_splatter;

	CompShape() {
		collision_group=1;
		collision_mask=0xff;
		enabled=true;
		bounce=false;
		terrain_bounce=false;

		hit_damage=10;
		take_damage_terrain_mult=1.0f;
	}

	void add_quad_center(sf::Vector2f size) {
		quads.push_back(Quad(-size*0.5f,size*0.5f));
	}
};

class CompDisplay : public Component {
	GraphicNode* add_node() {
		GraphicNode* node=new GraphicNode();
		root_node.add_child(node);
		nodes.push_back(GraphicNode::Ptr(node));
		return node;
	}
public:
	Node root_node;
	std::vector<GraphicNode::Ptr> nodes;
	std::vector<std::pair<float,Node*> > node_timeouts;

	void add_node_timeout(Node* node,float time) {
		node_timeouts.push_back(std::make_pair(time,node));
	}

	GraphicNode* add_texture(const Texture& tex,sf::Vector2f pos) {
		GraphicNode* n=add_node();
		n->set_graphic(Graphic(tex));
		n->pos=pos;
		return n;
	}
	GraphicNode* add_texture_center(const Texture& tex,float scale=1.0) {
		GraphicNode* node=add_texture(tex,-tex.get_size()*scale*0.5f);
		node->scale=sf::Vector2f(scale,scale);
		return node;
	}

	GraphicNode* add_graphic(const Graphic& g,sf::Vector2f pos) {
		GraphicNode* n=add_node();
		n->set_graphic(g);
		n->pos=pos;
		return n;
	}
	GraphicNode* add_graphic_center(const Graphic& g,float scale=1.0) {
		GraphicNode* node=add_graphic(g,-g.get_texture().get_size()*scale*0.5f);
		node->scale=sf::Vector2f(scale,scale);
		return node;
	}

	void remove() override;

	void remove_node(GraphicNode* node) {
		root_node.remove_child(node);
		for(std::size_t i=0;nodes.size();i++) {
			if(nodes[i].get()==node) {
				nodes.erase(nodes.begin()+i);
				return;
			}
		}
	}
};
class CompGun : public Component {
public:

	enum GunType {
		GUN_GUN,
		GUN_LASER,
		GUN_HOOK
	};

	GunType gun_type;

	uint8_t group;
	//bool fire;
	Texture texture;
	float bullet_speed;
	float angle;	//deg
	float fire_timeout;
	float cur_fire_timeout;
	sf::Vector2f pos;

	//shotgun
	float angle_spread;
	int bullet_count;

	EntityRef* laser_entity;
	GraphicNode* laser_node;

	std::vector<Texture> splatter_textures;

	class Hook {
	public:
		Node node_root;

		GraphicNode node_hook;
		Node node_chain;

		float max_length;
		float position;	//units
		bool extending;
		bool idle;
		sf::Vector2f world_pos;
		float angle;

		bool pull_mode;	//pull or grapple
		bool grabbed;
		EntityRef* grab_entity;
		float fire_timeout;
		float grab_angle;

		void set_textures(Texture hook_texture,Texture chain_texture) {
			node_hook.set_graphic(Graphic(hook_texture));
			node_hook.origin.x=hook_texture.get_size().x*0.5f;
			node_hook.origin.y=hook_texture.get_size().y;

			chain_texture.tex->setRepeated(true);
			node_chain.texture=chain_texture;
			node_chain.origin.x=chain_texture.get_size().x*0.5f;
		}
		Hook() {
			node_root.add_child(&node_chain);
			node_root.add_child(&node_hook);
			max_length=150;
			//position=0;
			extending=false;
			idle=true;
			angle=0;

			pull_mode=false;
			grabbed=false;
			//grab_position=0;
			grab_entity=nullptr;
			position=0;
			fire_timeout=0;
			grab_angle=0;

			node_chain.shader.shader=Loader::get_shader("shader/laser.frag");
		}

	};
	Hook hook;

	CompGun() {
		//fire=false;
		gun_type=GUN_GUN;
		group=0;
		bullet_speed=700;
		angle=0;
		fire_timeout=0.1;
		cur_fire_timeout=0;

		angle_spread=0;
		bullet_count=1;

		laser_entity=nullptr;
		laser_node=nullptr;
	}
};
class CompEngine : public Component {
public:
	GraphicNode node;

	void set(const Graphic& g,sf::Vector2f pos) {
		node.set_graphic(g);
		node.repeat=true;
		node.pos=pos-sf::Vector2f(g.get_texture().get_size().x*0.5f,0.0);
	}
	void remove() override;
};

class CompHealth : public Component {
public:
	bool alive;
	float health;
	float health_max;

	CompHealth() {
		alive=true;
		health=10;
		health_max=10;
	}
	void reset(float max) {
		health_max=health=max;
	}
};
/*
class CompInventory : public Component {
public:
	std::vector<Entity*> items;
};
*/
class CompTimeout : public Component {
public:
	enum Action {
		ACTION_REMOVE_ENTITY,
		ACTION_BIG_EXPLOSION
	};
	Action action;
	float timeout;
	CompTimeout() {
		action=ACTION_REMOVE_ENTITY;
		timeout=0;
	}
	void set(Action _action,float _timeout) {
		action=_action;
		timeout=_timeout;
	}
};
class CompAI : public Component {
public:

	enum AIType {
		AI_SUICIDE,
		AI_MELEE,
		AI_SHOOTER,
		AI_MISSILE,
		AI_FOLLOWER,
		AI_AVOIDER
	};
	AIType ai_type;

	EntityRef* target;

	sf::Vector2f rand_offset;	//normalized [-1,1]
	sf::Vector2f target_offset;	//world-space

	float engage_distance;

	float stun_timeout;

	CompAI() {
		ai_type=AI_SUICIDE;
		rand_offset=Utils::rand_vec(-1,1);
		target=NULL;
		engage_distance=800.0f;
		stun_timeout=0;
	}
};
class CompAI2 : public Component {
public:

	enum AIBehavior {
		AI_BEHAVIOR_COLLIDE,
		AI_BEHAVIOR_SAFE_FOLLOW,
		AI_BEHAVIOR_SHOOT,
		AI_BEHAVIOR_AIM_SHOOT,	//rotate guns to aim
		AI_BEHAVIOR_SPAWN_MINIONS
	};

	float idle_distance;
	float safe_follow_distance;

	std::vector<std::string> spawn_ids;
	float spawn_interval;
	float spawn_timeout;
	GraphicNode* spawn_node;	//animates node on spawn (ie crock boss mouth opening)
	sf::Vector2f spawn_vel;
	sf::Vector2f spawn_pos;
	float spawn_angle;

	sf::Vector2f rand_offset;	//normalized [-1,1]

	std::vector<AIBehavior> behaviors;

	std::vector<std::pair<std::string,int> > death_spawn_ids;

	SimpleTimer shoot_timer;
	SimpleTimer shoot_idle_timer;

	float stun_timeout;

	CompAI2() {
		idle_distance=600.0f;
		safe_follow_distance=300.0f;
		spawn_interval=3;
		spawn_timeout=0;
		spawn_node=nullptr;
		spawn_angle=270;
		rand_offset=Utils::rand_vec(-1,1);

		shoot_timer.reset(1.0);
		shoot_idle_timer.reset(3.0);

		stun_timeout=0;
	}
};


class CompTeleportation : public Component {
public:
	sf::Vector2f origin;
	sf::Vector2f destination;
	Node origin_node;
	float anim;
	float anim_duration;

	CompTeleportation() {
		anim=0.0f;
		anim_duration=0.0f;
	}
};
class CompShowDamage : public Component {
public:
	enum DamageType {
		DAMAGE_TYPE_BLINK,
		DAMAGE_TYPE_SCREEN_EFFECT,
		DAMAGE_TYPE_ANIMATION_PROGRESS
	};
	DamageType damage_type;
	SimpleTimer timer;
	GraphicNode* animation_progress_node;

	CompShowDamage() {
		damage_type=DAMAGE_TYPE_BLINK;
		timer.reset(0.5);
		animation_progress_node=nullptr;
	}
};
class CompBounce : public Component {
public:
	sf::Vector2f vel;
	SimpleTimer timer;

	CompBounce() {
		timer.reset(1.0);
	}
};
class CompFlameDamage : public Component {
public:
	SimpleTimer timer;
	CompFlameDamage() {
		timer.reset(0.05);
	}
};
class CompShowOnMinimap : public Component {
public:
	Node node;
};

class CompGravityForce : public Component {
public:
	bool enabled;
	float radius;
	float power_center;
	float power_edge;

	CompGravityForce() {
		enabled=true;
		radius=100;
		power_center=10;
		power_edge=0;
	}
};

class CompShield : public Component {
public:
	Node node;
	std::vector<Node::Ptr> layers;
	float anim;
	CompShield() {
		anim=0;
	}
};

class CompHammer : public Component {
public:
	Node* hammer_node;
	CompGravityForce* my_gravity_force;
	bool enabled;
	float anim;

	CompHammer() {
		enabled=false;
		hammer_node=nullptr;
		my_gravity_force=nullptr;
		anim=0;
	}
};
class CompSplatter : public Component {
public:
	Node splatter_node;
	std::vector<Node::Ptr> nodes;
	float anim;
	float speed;
	CompSplatter() {
		anim=0;
		speed=1.0;
	}
	Node* add_texture(const Texture& tex) {
		Node* n=new Node();
		n->texture=tex;
		splatter_node.add_child(n);
		nodes.push_back(Node::Ptr(n));
		return n;
	}
	void set_duration(float duration) {
		speed=1.0f/duration;
	}
};
class CompSpawnPieces : public Component {
public:
	class Piece {
	public:
		sf::Vector2f pos;
		sf::Vector2f map_size;
		sf::Vector2f map_pos;
	};
	Texture texture;
	std::vector<Piece> pieces;
};
class CompElectricity : public Component {
public:

	class Connection {
	public:
		EntityRef* entity;
		Entity* bolt_entity;
		GraphicNode* bolt_node;
		float timeout;
		Connection() {
			entity=nullptr;
			bolt_entity=nullptr;
			bolt_node=nullptr;
			timeout=0;
		}
	};

	bool is_source;
	bool player_side;
	float potential;
	float decay_speed;

	GraphicNode* node_flare;

	//Node root_node;
	std::vector<Connection> connections;

	CompElectricity() {
		is_source=false;
		player_side=true;
		potential=0;
		node_flare=nullptr;
		decay_speed=2.0f;
	}
};
class CompStunBlast : public Component {
public:
	float anim;
	float duration;
	float range;

	CompStunBlast() {
		anim=0;
		range=200;
		duration=1.0;
	}
};


class CompPlatypusBoss : public Component {
public:

	enum SpawnEvent {
		SPAWN_EVENT_OPEN,
		SPAWN_EVENT_SPAWN1,
		SPAWN_EVENT_SPAWN2,
		SPAWN_EVENT_CLOSE
	};

	Node node;
	GraphicNode anim;

	Animation anim_appear;
	Animation anim_dissapear;
	Animation anim_belly_shot;
	float offset_appear;
	float offset_dissapear;
	float offset_belly_shot;

	float cur_x;
	float floor_y;

	bool shown;
	bool shooting;
	int shots_remaining;
	SimpleTimer timer;
	EventTimer spawn_timer;

	Texture texture_bullet;
	Texture texture_state_closed;
	Texture texture_state_open;

	CompPlatypusBoss() {
		anim_appear=Animation(Loader::get_texture("enemies/Platypus/boss/King animations/platypus appear.png"),25,35,0.1);
		anim_dissapear=Animation(Loader::get_texture("enemies/Platypus/boss/King animations/platypus dissappear.png"),36,39,0.1);
		anim_dissapear.add_empty_frame();
		anim_belly_shot=Animation(Loader::get_texture("enemies/Platypus/boss/King animations/belly shot.png"),50,36,0.1);
		texture_bullet=Loader::get_texture("general assets/proj4.png");
		texture_state_closed=Loader::get_texture("enemies/Platypus/boss/phase1.png");
		texture_state_open=Loader::get_texture("enemies/Platypus/boss/phase2.png");


		offset_appear=14;
		offset_dissapear=25;
		offset_belly_shot=40;

		node.add_child(&anim);

		floor_y=-2;
		cur_x=0;
		shots_remaining=0;
		shown=false;
		shooting=false;

		anim.repeat=false;

		spawn_timer.repeat=true;
		spawn_timer.duration=4.5;
		spawn_timer.add_event(2,SPAWN_EVENT_OPEN);
		spawn_timer.add_event(3,SPAWN_EVENT_SPAWN1);
		spawn_timer.add_event(3.5,SPAWN_EVENT_SPAWN2);
		spawn_timer.add_event(4.5,SPAWN_EVENT_CLOSE);
	}
};

class CompFighterShip : public Component {
public:

	Node* node_blade1;
	Node* node_blade2;

	EventTimer slash_timer;

	bool ctrl_slash;
	bool ctrl_rotating;

	float rotating_anim;

	CompFighterShip() {
		node_blade1=nullptr;
		node_blade2=nullptr;
		slash_timer.reset(0.2);
		slash_timer.add_event(0.0,0);
		slash_timer.end();
		ctrl_slash=false;
		ctrl_rotating=false;
		rotating_anim=0;
	}
};

class Entity {
public:
	//attributes
	enum Attribute {
		ATTRIBUTE_REMOVE_ON_DEATH=0,
		ATTRIBUTE_PLAYER_CONTROL,
		ATTRIBUTE_ENEMY,		//all enemies
		ATTRIBUTE_FRIENDLY,		//all friendlies
		ATTRIBUTE_ATTRACT,		//candy mine
		ATTRIBUTE_INTEGRATE_POSITION,

		ATTRIBUTE_BOSS
	};

	//XXX move all members to comps/attrs
	//display
	Node node_main;

	//dynamics
	sf::Vector2f pos;	//center
	sf::Vector2f vel;
	sf::Vector2f bounce_vel;	//XXX remove
	float angle;	//deg

	//component cache
	CompHealth* comp_health;
	CompShape* comp_shape;
	CompDisplay* comp_display;

	bool player_side;
	bool fire_gun[8];

	float tmp_damage;
	float tmp_timeout[10];

	//Component* components_indexed[COMPONENT_INDEX_SIZE];
	//std::vector<Component*> components_unindexed;
	std::vector<Component*> components;
	std::vector<Attribute> attributes;

	std::vector<EntityRef*> refs;

	Entity() {
		/*
		for(int i=0;i<COMPONENT_INDEX_SIZE;i++) {
			components_indexed[i]=NULL;
		}
		*/
		angle=270;	//point up by default
		for(int i=0;i<8;i++) fire_gun[i]=false;
		player_side=false;

		for(int i=0;i<10;i++) tmp_timeout[i]=0;

		comp_health=nullptr;
		comp_shape=nullptr;
		comp_display=nullptr;

		tmp_damage=10;
	}
};
/*
class Message {
public:
	enum Type {
		TYPE_SYS_ADDED,
		TYPE_SYS_REMOVED,
		TYPE_DAMAGED,
		TYPE_DIED
	};

	Type type;
};
*/

class EntityManager {

	Component* component_create(Component::Type type) {
		Component *c=nullptr;
		if(type==Component::TYPE_DISPLAY) {
			c=new CompDisplay();
		}
		else if(type==Component::TYPE_SHAPE) {
			c=new CompShape();
		}
		else if(type==Component::TYPE_GUN) {
			c=new CompGun();
		}
		else if(type==Component::TYPE_TIMEOUT) {
			c=new CompTimeout();
		}
		else if(type==Component::TYPE_AI) {
			c=new CompAI();
		}
		else if(type==Component::TYPE_AI2) {
			c=new CompAI2();
		}
		else if(type==Component::TYPE_ENGINE) {
			c=new CompEngine();
		}
		else if(type==Component::TYPE_TELEPORTATION) {
			c=new CompTeleportation();
		}
		else if(type==Component::TYPE_SHOW_DAMAGE) {
			c=new CompShowDamage();
		}
		else if(type==Component::TYPE_BOUNCE) {
			c=new CompBounce();
		}
		else if(type==Component::TYPE_FLAME_DAMAGE) {
			c=new CompFlameDamage();
		}
		else if(type==Component::TYPE_SHOW_ON_MINIMAP) {
			c=new CompShowOnMinimap();
		}
		else if(type==Component::TYPE_GRAVITY_FORCE) {
			c=new CompGravityForce();
		}
		else if(type==Component::TYPE_SHIELD) {
			c=new CompShield();
		}
		else if(type==Component::TYPE_HAMMER) {
			c=new CompHammer();
		}
		else if(type==Component::TYPE_HEALTH) {
			c=new CompHealth();
		}
		else if(type==Component::TYPE_SPLATTER) {
			c=new CompSplatter();
		}
		else if(type==Component::TYPE_SPAWN_PIECES) {
			c=new CompSpawnPieces();
		}
		else if(type==Component::TYPE_STUN_BLAST) {
			c=new CompStunBlast();
		}
		else if(type==Component::TYPE_PLATYPUS_BOSS) {
			c=new CompPlatypusBoss();
		}
		else if(type==Component::TYPE_FIGHTER_SHIP) {
			c=new CompFighterShip();
		}
		else if(type==Component::TYPE_ELECTRICITY) {
			c=new CompElectricity();
		}
		else {
			printf("WARN: comp type %d not handled\n",type);
		}

		if(c) {
			c->type=type;
		}
		return c;
	}


	std::vector<std::vector<Component*> > component_map;
	std::vector<std::vector<Entity*> > attribute_map;

	//component type -> event type -> set of components
	std::vector<std::vector<std::unordered_set<Component*> > > component_events;

	SimpleList<Component*> components_to_remove;
	SimpleList<Component*> components_to_delete;
	SimpleList<Entity*> entities_to_remove;
	SimpleList<Entity*> entities_to_add;
	SimpleList<Entity*> entities_to_delete;
	SimpleList<std::pair<Component*,Component::Event> > events_to_add;
	SimpleList<std::pair<Component*,Component::Event> > events_to_remove;


public:
	std::vector<Entity*> entities;

	//callbacks
	typedef std::function<void(Component*)> ComponentCallback;

	ComponentCallback on_component_added;
	ComponentCallback on_component_removed;

	//entities
	Entity* entity_create() {
		//printf("new entity\n");
		return new Entity();
	}
	void entity_add(Entity* entity) {
		entities_to_add.push_back(entity);
	}
	void entity_remove(Entity* entity) {
		entities_to_remove.push_back(entity);
	}
	EntityRef* entity_ref_create(Entity* entity) {
		EntityRef* ref=new EntityRef();
		ref->entity=entity;
		entity->refs.push_back(ref);
		return ref;
	}
	void entity_ref_delete(EntityRef* ref) {
		if(ref->entity) {
			Utils::vector_remove(ref->entity->refs,ref);
		}
		delete(ref);
	}

	//events
	void event_add(Component* c,Component::Event e) {
		events_to_add.push_back(std::pair<Component*,Component::Event>(c,e));
	}
	void event_remove(Component* c,Component::Event e) {
		events_to_remove.push_back(std::pair<Component*,Component::Event>(c,e));
	}
	const std::unordered_set<Component*>& event_list_components(Component::Type component_type,Component::Event event) {
		Utils::vector_fit_size(component_events,component_type+1);
		Utils::vector_fit_size(component_events[component_type],event+1);
		return component_events[component_type][event];
	}


	//attributes
	void attribute_add(Entity* e,Entity::Attribute attr) {
		Utils::vector_fit_size(attribute_map,attr+1);
		attribute_map[attr].push_back(e);
		e->attributes.push_back(attr);
	}
	void attribute_remove(Entity* e,Entity::Attribute attr) {
		if(attribute_map.size()<(std::size_t)(attr+1)) {
			return;
		}
		Utils::vector_remove(attribute_map[attr],e);
	}
	bool attribute_has(Entity* e,Entity::Attribute attr) {
		if(attribute_map.size()<(std::size_t)(attr+1)) {
			return false;
		}
		return (Utils::vector_index_of(attribute_map[attr],e)!=-1);
	}
	const std::vector<Entity*>& attribute_list_entities(Entity::Attribute attr) {
		Utils::vector_fit_size(attribute_map,attr+1);
		return attribute_map[attr];
	}


	//components

	//"simple" components, no indexing
	Component* component_add(Entity* entity,Component::Type type) {
		Component* comp=component_create(type);
		if(comp==nullptr) {
			return comp;
		}
		comp->entity=entity;
		Utils::vector_fit_size(component_map,type+1);
		component_map[type].push_back(comp);
		entity->components.push_back(comp);

		//cached components
		if(type==Component::TYPE_HEALTH) {
			entity->comp_health=(CompHealth*)comp;
		}
		else if(type==Component::TYPE_SHAPE) {
			entity->comp_shape=(CompShape*)comp;
		}
		else if(type==Component::TYPE_DISPLAY) {
			entity->comp_display=(CompDisplay*)comp;
		}

		if(on_component_added) {
			on_component_added(comp);
		}

		return comp;
	}
	void component_remove(Component* component) {
		components_to_remove.push_back(component);
	}
	//returns first found component of type
	//TODO: slow
	Component* component_get(Entity* e,Component::Type type) {
		for(Component* c : e->components) {
			if(c->type==type) {
				return c;
			}
		}
		return nullptr;
	}

	const std::vector<Component*>& component_list(Component::Type type) {
		Utils::vector_fit_size(component_map,type+1);
		return component_map[type];
	}

	//apply add/remove operations
	void update() {

		//events add/remove
		for(int i=0;i<events_to_add.size();i++) {
			Component* c=events_to_add[i].first;
			Component::Event e=events_to_add[i].second;
			if(c->events.insert(e).second) {
				Utils::vector_fit_size(component_events,c->type+1);
				Utils::vector_fit_size(component_events[c->type],e+1);
				component_events[c->type][e].insert(c);
			}
		}
		events_to_add.clear();

		for(int i=0;i<events_to_remove.size();i++) {
			Component* c=events_to_remove[i].first;
			Component::Event e=events_to_remove[i].second;
			if(c->events.erase(e)!=0) {
				component_events[c->type][e].erase(c);
			}
		}
		events_to_remove.clear();


		//entities add/remove
		if(entities_to_add.size()>0) {
			for(int i=0;i<entities_to_add.size();i++) {
				Entity* e=entities_to_add[i];
				entities.push_back(e);
				for(Component* c : e->components) {
					c->insert();
				}

			}
			entities_to_add.clear();
		}

		if(entities_to_remove.size()>0) {
			//for(Entity* e : entities_to_remove) {
			for(int i=0;i<entities_to_remove.size();i++) {
				Entity* e=entities_to_remove[i];

				int index=Utils::vector_index_of(entities,e);
				if(index==-1) {
					printf("entity not in list\n");
					continue;
				}

				entities.erase(entities.begin()+index);

				for(Component* c : e->components) {
					c->remove();
					c->entity=NULL;
					components_to_remove.push_back(c);
				}

				for(Entity::Attribute a : e->attributes) {
					attribute_remove(e,a);
				}

				for(EntityRef* ref : e->refs) {
					ref->entity=NULL;
				}

				entities_to_delete.push_back(e);
			}
			entities_to_remove.clear();
		}

		//components add/remove
		if(components_to_remove.size()>0) {

			for(int i=0;i<components_to_remove.size();i++) {
				Component* c=components_to_remove[i];

				if(components_to_delete.contains(c)) {
					continue;
				}

				if(component_map.size()<(std::size_t)(c->type+1)) {
					printf("type not in map %d\n",c->type);
					continue;
				}
				int index=Utils::vector_index_of(component_map[c->type],c);
				if(index==-1) {
					//printf("comp not in map\n");
					continue;
				}

				if(c->entity) {
					if(c->type==Component::TYPE_HEALTH) {
						c->entity->comp_health=nullptr;
					}
					else if(c->type==Component::TYPE_SHAPE) {
						c->entity->comp_shape=nullptr;
					}
					else if(c->type==Component::TYPE_DISPLAY) {
						c->entity->comp_display=nullptr;
					}
				}

				if(on_component_removed) {
					on_component_removed(c);
				}

				for(Component::Event event : c->events) {
					component_events[c->type][event].erase(c);
				}

				if(c->entity) {
					c->remove();
					Utils::vector_remove(c->entity->components,c);
					c->entity=NULL;
				}
				for(EntityRef* ref : c->my_refs) {
					entity_ref_delete(ref);
				}

				component_map[c->type].erase(component_map[c->type].begin()+index);
				components_to_delete.push_back(c);
			}

			for(int i=0;i<components_to_delete.size();i++) {
				delete(components_to_delete[i]);
			}
			components_to_remove.clear();
			components_to_delete.clear();

			for(int i=0;i<entities_to_delete.size();i++) {
				delete(entities_to_delete[i]);
			}
			entities_to_delete.clear();
		}
	}
};

class SwarmManagerCtrl {
public:
	sf::Vector2f pos;
	bool fire;

	SwarmManagerCtrl() {
		pos=sf::Vector2f(0,0);
		fire=false;
	}
	SwarmManagerCtrl(sf::Vector2f _pos,bool _fire) {
		pos=_pos;
		fire=_fire;
	}
};

class SwarmManager {
public:

	int pattern;
	//0 = circling
	//1 = intruders


	float anim;
	std::vector<EntityRef*> entities;

	SwarmManager() {
		pattern=0;
		anim=0;
	}

	SwarmManagerCtrl get_entity_pos(int index) {
		if(entities.empty()) {
			return SwarmManagerCtrl();
		}
		if(pattern==0) {
			float dist=200;
			float duration=0.3;

			float a=(float)index/(float)entities.size();
			float angle=Utils::lerp(0,360,a)+anim/duration*20.0f;

			bool fire=(fmod(angle+anim*50.0f,360.0f)<90.0f);
			return SwarmManagerCtrl(Utils::vec_for_angle_deg(angle,dist),fire);
		}
		if(pattern==1) {
			float w=400;
			//float h=300;

			float fi=index;

			int row_size=4;

			int ix=index%row_size;
			int iy=index/row_size;

			float fx=(float)ix/3.0f;

			sf::Vector2f pos;

			pos.x=-w/2.0f+Utils::lerp(0,w,fx);
			pos.y=-100-iy*70;

			pos.x+=w*0.5f*sin(anim+fi*0.2f);

			bool fire=(sin(anim*1.23f+fi*12.3f)>0.5f);

			return SwarmManagerCtrl(pos,fire);
		}

		return SwarmManagerCtrl();
	}
};


class Game : public Menu {

	class Controls {
	public:
		bool move_left;
		bool move_right;
		bool move_up;
		bool move_down;

		Controls() {
			move_left=false;
			move_right=false;
			move_up=false;
			move_down=false;
		}
	};
	class Minimap : public Node {
	public:

		class TerrainNode {
		public:
			Node node;
			uint32_t texture_version_id;
		};

		Node node_bg;
		Node node_terrain;
		Node items;
		sf::Vector2f size;
		float map_scale;

		Terrain* terrain;

		typedef std::unordered_map<TerrainIsland*,TerrainNode*> IslandMap;
		IslandMap loaded_islands;

		Minimap() {
			add_child(&node_bg);
			add_child(&node_terrain);
			add_child(&items);
			node_bg.type=Node::TYPE_SOLID;
			node_bg.color.set(0,0,0,1);
			resize(sf::Vector2f(200,200));

			map_scale=0.08;
			terrain=nullptr;
			clip_enabled=true;
		}
		void resize(sf::Vector2f _size) {
			size=_size;
			node_bg.scale=size;
			clip_quad.p2=size;
		}

		void update(sf::Vector2f center) {

			Quad world_quad(center-size*0.5f/map_scale,center+size*0.5f/map_scale);

			for(Node* n : items.children) {

				if(world_quad.contains(n->pos)) {
					n->pos=(n->pos-center)*map_scale+size*0.5f;
					n->visible=true;
				}
				else {
					n->visible=false;
				}
			}

			if(terrain!=nullptr) {

				for(IslandMap::iterator it=loaded_islands.begin();it!=loaded_islands.end();) {
					if(!terrain->island_intersects(it->first,world_quad)) {
						//printf("unload island\n");
						it->second->node.texture.unload();
						node_terrain.remove_child(&it->second->node);
						it=loaded_islands.erase(it);
					}
					else {

						if(it->first->version_id!=it->second->texture_version_id) {
							it->first->generate_icon_texture(it->second->node.texture.tex);
							it->second->texture_version_id=it->first->version_id;
						}

						it++;
					}
				}

				SimpleList<TerrainIsland*>& islands=terrain->list_islands(world_quad);

				for(int i=0;i<islands.size();i++) {
					TerrainIsland* isl=islands[i];
					if(terrain->island_intersects(isl,world_quad) && loaded_islands.find(isl)==loaded_islands.end()) {
						//printf("load island\n");
						Texture texture=isl->generate_icon_texture();

						if(texture.tex==nullptr) {
							continue;
						}

						sf::Vector2f isl_pos=isl->box.p1;

						if(isl_pos.x>world_quad.p2.x) isl_pos.x-=terrain->field_size.x;
						if(isl_pos.y>world_quad.p2.y) isl_pos.y-=terrain->field_size.y;

						TerrainNode* t_node=new TerrainNode();
						t_node->node.texture=texture;
						t_node->node.pos=isl_pos*map_scale;
						t_node->node.scale=sf::Vector2f(1,1)*(float)isl->cell_size*map_scale;
						t_node->texture_version_id=isl->version_id;

						node_terrain.add_child(&t_node->node);
						loaded_islands.insert(std::pair<TerrainIsland*,TerrainNode*>(isl,t_node));
					}
				}

				node_terrain.pos=-center*map_scale+size*0.5f;
			}

		}
	};

	class CamShake {
	public:

		float timeout;
		float duration;
		float gain;
		sf::Vector2f offset;

		CamShake() {
			offset.x=0;
			offset.y=0;

			timeout=0;
			gain=5;
			duration=0.1;
		}
		void start() {
			timeout=duration;
		}
		void frame(float delta) {
			if(timeout<=0.0) {
				return;
			}
			timeout-=delta;
			if(timeout<=0.0) {
				timeout=0.0;
			}
			float g=gain*timeout/duration;
			offset.x=Utils::rand_range(-1,1)*g;
			offset.y=Utils::rand_range(-1,1)*g;
		}
	};

	class WaveManager {
	public:

		float timeout;

		WaveManager() {
			timeout=2;
		}
	};

	CamShake cam_shake;

	//WaveManager wave_manager;
	//std::vector<SwarmManager*> swarms;

	Node node_help_text;
	Node node_level_complete;

	Minimap minimap;

	SpaceBackground background;
	Node node_game;
	Node node_ships;
	Node node_splatter;

	ProgressBar player_health;

	Terrain terrain;

	EntityManager entities;

	Entity* player;

	std::vector<Graphic> graphic_explosion;
	std::vector<Graphic> graphic_enemy_suicide;
	std::vector<Graphic> graphic_enemy_shooter_up;
	std::vector<Graphic> graphic_enemy_shooter_down;
	std::vector<Graphic> graphic_enemy_shooter_side;
	Graphic graphic_engine;
	std::vector<Graphic> graphic_missile;

	NodeShader shader_damage;
	bool use_shader_damage;

	NodeShader shader_blast;
	bool use_shader_blast;

	float time_scale;
	Controls controls;

	float game_time;
	bool level_won;

	std::unordered_map<std::string,std::function<Entity*()> > entity_create_map;

	std::array<Node,16> dbg_nodes;

public:

	int action_esc;

	int zoom_mode;
	//bool snap_sprites_to_pixels;

	int player_ship;
	int selected_level;

	void component_added(Component* c) {

		if(c->type==Component::TYPE_DISPLAY) {
			CompDisplay* comp=(CompDisplay*)c;
			comp->entity->node_main.add_child(&comp->root_node);
		}
		else if(c->type==Component::TYPE_ENGINE) {
			CompEngine* comp=(CompEngine*)c;
			comp->entity->node_main.add_child(&comp->node);
		}
		else if(c->type==Component::TYPE_SHOW_DAMAGE) {
			entities.event_add(c,Component::EVENT_DAMAGED);
		}
		else if(c->type==Component::TYPE_SHOW_ON_MINIMAP) {
			CompShowOnMinimap* comp=(CompShowOnMinimap*)c;
			minimap.items.add_child(&comp->node);
		}
		else if(c->type==Component::TYPE_SHIELD) {
			CompShield* comp=(CompShield*)c;
			comp->entity->node_main.add_child(&comp->node);
		}
		else if(c->type==Component::TYPE_SPLATTER) {
			CompSplatter* splatter=(CompSplatter*)c;
			node_splatter.add_child(&splatter->splatter_node);
		}
		else if(c->type==Component::TYPE_PLATYPUS_BOSS) {
			CompPlatypusBoss* boss=(CompPlatypusBoss*)c;
			boss->entity->node_main.add_child(&boss->node);
		}
		else if(c->type==Component::TYPE_ELECTRICITY) {
			CompElectricity* el=(CompElectricity*)c;
			if(c->entity->comp_display) {
				el->node_flare=c->entity->comp_display->add_texture(Loader::get_texture("general assets/bolt_flare.png"),
						sf::Vector2f(0,0));
				el->node_flare->origin=el->node_flare->texture.get_size()*0.5f;
			}
		}
		/*
		else if(c->type==Component::TYPE_GUN) {
			CompGun* gun=(CompGun*)c;
			if(gun->gun_type==CompGun::GUN_HOOK) {
				gun->entity->node_main.add_child(&gun->hook.node_root);
			}
		}
		*/
	}
	void component_removed(Component* c) {

		if(c->type==Component::TYPE_SHOW_ON_MINIMAP) {
			CompShowOnMinimap* comp=(CompShowOnMinimap*)c;
			minimap.items.remove_child(&comp->node);
		}
		else if(c->type==Component::TYPE_SHIELD) {
			CompShield* comp=(CompShield*)c;
			comp->entity->node_main.remove_child(&comp->node);
		}
		else if(c->type==Component::TYPE_SPLATTER) {
			CompSplatter* splatter=(CompSplatter*)c;
			node_splatter.remove_child(&splatter->splatter_node);
		}
		else if(c->type==Component::TYPE_GUN) {
			CompGun* gun=(CompGun*)c;
			if(gun->laser_entity) {
				entity_remove(gun->laser_entity->entity);
			}
			if(gun->gun_type==CompGun::GUN_HOOK) {
				if(gun->entity) {
					gun->entity->node_main.remove_child(&gun->hook.node_root);
				}
				if(gun->hook.grab_entity) {
					entities.entity_ref_delete(gun->hook.grab_entity);
				}
			}
		}
		else if(c->type==Component::TYPE_PLATYPUS_BOSS) {
			CompPlatypusBoss* boss=(CompPlatypusBoss*)c;
			if(boss->entity) {
				boss->entity->node_main.remove_child(&boss->node);
			}
		}
		else if(c->type==Component::TYPE_ELECTRICITY) {
			CompElectricity* el=(CompElectricity*)c;
			for(CompElectricity::Connection& c : el->connections) {
				entity_remove(c.bolt_entity);
				entities.entity_ref_delete(c.entity);
			}
			if(el->node_flare && el->entity && el->entity->comp_display) {
				el->entity->comp_display->remove_node(el->node_flare);
			}
		}


	}

	Game() {

		entity_create_map["enemy/walrus/random"]=std::bind(&Game::create_walrus_random,this);
		entity_create_map["enemy/walrus/boss_split1"]=std::bind(&Game::create_walrus_boss,this,1);
		entity_create_map["enemy/walrus/boss_split2"]=std::bind(&Game::create_walrus_boss,this,2);
		entity_create_map["enemy/walrus/boss_split3"]=std::bind(&Game::create_walrus_boss,this,3);

		entity_create_map["enemy/ocean/random"]=std::bind(&Game::create_ocean_random,this);
		entity_create_map["enemy/reptile/random"]=std::bind(&Game::create_reptile_random,this);
		entity_create_map["enemy/farm/random"]=std::bind(&Game::create_farm_random,this);

		entity_create_map["enemy/fish/rocket"]=std::bind(&Game::create_fish_rocket,this);
		entity_create_map["enemy/fish/minion"]=std::bind(&Game::create_fish_minion,this);

		player_ship=0;
		selected_level=0;

		action_esc=0;
		zoom_mode=1;
		time_scale=1.0;
		//snap_sprites_to_pixels=true;

		minimap.terrain=&terrain;

		entities.on_component_added= [=](Component* c) { this->component_added(c); };
		entities.on_component_removed= [=](Component* c) { this->component_removed(c); };

		//init
		background.create_default();
		node.add_child(&background);
		node.add_child(&node_game);
		node.add_child(&minimap);
		node.add_child(&node_help_text);
		node.add_child(&node_level_complete);
		add_child(&player_health);
		node.add_child(&node_splatter);
		node_game.add_child(&terrain);
		node_game.add_child(&node_ships);

		for(std::size_t i=0;i<dbg_nodes.size();i++) {
			dbg_nodes[i].visible=false;
			dbg_nodes[i].type=Node::TYPE_SOLID;
			dbg_nodes[i].scale=sf::Vector2f(10,10);
			node_game.add_child(&dbg_nodes[i]);
		}

		player_health.set_border(3);
		player_health.resize(sf::Vector2f(300,20));


		node_help_text.type=Node::TYPE_TEXT;
		node_help_text.text.setFont(*Loader::get_menu_font());
		node_help_text.text.setCharacterSize(22);
		node_help_text.pos=sf::Vector2f(10,10);

		node_level_complete.type=Node::TYPE_TEXT;
		node_level_complete.text.setFont(*Loader::get_menu_font());
		node_level_complete.text.setCharacterSize(25);
		node_level_complete.pos=sf::Vector2f(10,10);
		node_level_complete.text.setString("LEVEL COMPLETE");
		node_level_complete.set_origin_text_center_x();


		//populate some assets
		graphic_engine=Graphic(Animation(Loader::get_texture("general assets/engine fire.png"),10,13,0.05));
		const char* explosion_texture_names[]={"general assets/explosions.png",/*"explosions1.png",*/NULL};
		for(int t=0;explosion_texture_names[t];t++) {
			Texture tex_explosions=Loader::get_texture(explosion_texture_names[t]);
			if(tex_explosions.tex!=NULL) {
				for(int i=0;i<4;i++) {
					int h=tex_explosions.tex->getSize().y/4;
					tex_explosions.rect.top=h*i;
					tex_explosions.rect.height=h;
					graphic_explosion.push_back(Graphic(Animation(tex_explosions,32,32,0.05)));
				}
			}
		}
		const char* rockets_texture_names[]={"rocket1.png","rocket2.png","rocket3.png","rocket4.png",NULL};
		for(int i=0;rockets_texture_names[i];i++) {
			graphic_missile.push_back(Graphic(Loader::get_texture((std::string)"player ships/Assaulter/"+rockets_texture_names[i])));
		}

		const char* enemy_suicide_texture_names[]={
			"Bird/Suicide/bird50.png",
			"Bird/Suicide/bird62.png",
			"Bird/Suicide/bird53.png",
			"Bird/Suicide/bird58.png",
			"Bird/Suicide/bird49.png",
			"Bird/Suicide/bird60.png",
			"Bird/Suicide/bird46.png",
			"Bird/Suicide/bird56.png",
			"Bird/Suicide/bird54.png",
			"Bird/Suicide/bird45.png",
			"Bird/Suicide/bird59.png",
			"Bird/Suicide/bird51.png",
			"Bird/Suicide/bird47.png",
			"Bird/Suicide/bird55.png",
			"Bird/Suicide/bird44.png",
			"Bird/Suicide/bird57.png",
			"Bird/Suicide/bird61.png",
			"Bird/Suicide/bird52.png",
			"Bird/Suicide/bird42.png",
			"Bird/Suicide/bird48.png",
			"Bird/Suicide/bird43.png",
			"Reptiles/Suicide/reptile5.png",
			"Reptiles/Suicide/reptile7.png",
			"Reptiles/Suicide/reptile4.png",
			"Reptiles/Suicide/reptile6.png",
			"Reptiles/Suicide/reptile3.png",
			"Ocean/Suicide/ocean10.png",
			"Ocean/Suicide/ocean6.png",
			"Ocean/Suicide/ocean3.png",
			"Ocean/Suicide/ocean2.png",
			"Ocean/Suicide/ocean5.png",
			"Ocean/Suicide/ocean11.png",
			"Farm/Suicide/chicken4.png",
			"Farm/Suicide/chicken7.png",
			"Farm/Suicide/chicken2.png",
			"Farm/Suicide/chicken5.png",
			"Farm/Suicide/chicken1.png",
			"Farm/Suicide/chicken3.png",
			"Farm/Suicide/chicken8.png",
			"Farm/Suicide/chicken6.png",
			"Turtle/Suicide/turtle4.png",
			"Fish/Suicide/fish66.png",
			"Fish/Suicide/fish72.png",
			"Fish/Suicide/fish64.png",
			"Fish/Suicide/fish79.png",
			"Fish/Suicide/fish69.png",
			"Fish/Suicide/fish80.png",
			"Fish/Suicide/fish75.png",
			"Fish/Suicide/fish73.png",
			"Fish/Suicide/fish74.png",
			"Fish/Suicide/fish84.png",
			"Fish/Suicide/fish62.png",
			"Fish/Suicide/fish76.png",
			"Fish/Suicide/fish78.png",
			"Fish/Suicide/fish82.png",
			"Fish/Suicide/fish65.png",
			"Fish/Suicide/fish81.png",
			"Fish/Suicide/fish85.png",
			"Fish/Suicide/fish63.png",
			"Fish/Suicide/fish67.png",
			"Fish/Suicide/fish71.png",
			"Fish/Suicide/fish70.png",
			"Fish/Suicide/fish83.png",
			"Fish/Suicide/fish86.png",
			"Fish/Suicide/fish68.png",
			"Platypus/Suicide/platypus7.png",
			"Platypus/Suicide/platypus3.png",
			"Platypus/Suicide/platypus5.png",
			"Platypus/Suicide/platypus4.png",
			"Platypus/Suicide/platypus8.png",
			"Platypus/Suicide/platypus6.png",NULL};

		for(int i=0;enemy_suicide_texture_names[i];i++) {
			graphic_enemy_suicide.push_back(Graphic(Loader::get_texture(
					(std::string)"enemies/"+enemy_suicide_texture_names[i])));
		}

		const char* enemy_shooter_up_texture_names[]={
			"Bird/Shooter/bird41.png",
			"Bird/Shooter/bird37.png",
			"Bird/Shooter/bird39.png",
			"Bird/Shooter/bird40.png",
			"Bird/Shooter/bird38.png",
			"Turtle/Shooter/turtle2.png",NULL};
		const char* enemy_shooter_down_texture_names[]={
			"Farm/Shooter/pig4.png",
			"Farm/Shooter/cow7.png",
			"Farm/Shooter/pig7.png",
			"Farm/Shooter/cow5.png",
			"Farm/Shooter/pig6.png",
			"Farm/Shooter/pig2.png",
			"Farm/Shooter/pig1.png",
			"Farm/Shooter/cow8.png",
			"Farm/Shooter/cow3.png",
			"Farm/Shooter/pig5.png",
			"Farm/Shooter/pig8.png",
			"Farm/Shooter/cow2.png",
			"Farm/Shooter/cow1.png",
			"Farm/Shooter/cow6.png",
			"Farm/Shooter/pig3.png",
			"Farm/Shooter/cow4.png",
			"Fish/Shooter/fish60.png",
			"Fish/Shooter/fish59.png",
			"Fish/Shooter/fish61.png",
			"Fish/Shooter/fish57.png",
			"Fish/Shooter/fish55.png",
			"Fish/Shooter/fish52.png",
			"Fish/Shooter/fish58.png",
			"Fish/Shooter/fish51.png",
			"Fish/Shooter/fish53.png",
			"Fish/Shooter/fish56.png",
			"Fish/Shooter/fish54.png",
			"Fish/Shooter/fish50.png",
			"Ocean/Shooter/ocean4.png",
			"Ocean/Shooter/ocean7.png",
			"Turtle/Shooter/turtle3.png",NULL};
		const char* enemy_shooter_side_texture_names[]={
			"Ocean/Shooter/ocean9.png",NULL};

		for(int i=0;enemy_shooter_up_texture_names[i];i++) {
			graphic_enemy_shooter_up.push_back(Graphic(Loader::get_texture(
					(std::string)"enemies/"+enemy_shooter_up_texture_names[i])));
		}
		for(int i=0;enemy_shooter_down_texture_names[i];i++) {
			graphic_enemy_shooter_down.push_back(Graphic(Loader::get_texture(
					(std::string)"enemies/"+enemy_shooter_down_texture_names[i])));
		}
		for(int i=0;enemy_shooter_side_texture_names[i];i++) {
			graphic_enemy_shooter_side.push_back(Graphic(Loader::get_texture(
					(std::string)"enemies/"+enemy_shooter_side_texture_names[i])));
		}

		shader_damage.shader=Loader::get_shader("shader/boom.frag");
		shader_damage.set_param("freq",35);
		shader_damage.set_param("amount",0.01);
		use_shader_damage=false;

		shader_blast.shader=Loader::get_shader("shader/blast.frag");
		shader_blast.set_param("anim",0.5);
		use_shader_blast=false;

		/*
		for(int i=0;i<4;i++) {
			cam_border[i].type=Node::TYPE_SOLID;
			cam_border[i].scale=sf::Vector2f(100,100);
			cam_border[i].origin=sf::Vector2f(50,50);
			node_game.add_child(&cam_border[i]);
		}
		*/

		player=NULL;

	}
	//Node cam_border[4];
	//Node tmp_node;

	~Game() {

	}
	void start_level() {
		//reset
		time_scale=1.0;
		game_time=0.0;
		level_won=false;
		use_shader_damage=false;

		node_level_complete.visible=false;

		//cleanup
		for(Entity* e : entities.entities) {
			entity_remove(e);
		}
		entities.update();

		const char* help_texts[]={
				"Bastion\nMouse - cannon\nQ - shield\nE - attract\nR - candy mine\nSPACE - hammer",
				"Engineer\nMouse - hook\nQ - mine\nE - helper\nR - electric shock\nSPACE - blackhole mine",
				"Assaulter\nMouse - machine gun/shotgun\nQ - missiles\nE - teleport\nR - laser\nSPACE - bullet time",
				"Fighter\nMouse - melee slash\nQ - stun impulse blast\nE - rotating blades\nR - grappling hook\nSPACE - electric blades"
		};

		//load
		if(player_ship==0) {
			player=create_player_bastion();
		}
		else if(player_ship==1) {
			player=create_player_engineer();
		}
		else if(player_ship==2) {
			player=create_player_assaulter();
		}
		else if(player_ship==3) {
			player=create_player_fighter();
		}

		node_help_text.text.setString(help_texts[player_ship]);

		entity_add(player);

		sf::Vector2f boss_pos=Utils::vec_for_angle(Utils::rand_angle(),3000);

		if(selected_level==1) {	//octopuss
			//boss_pos=sf::Vector2f(-200,0);

			Entity* boss=create_octopuss_boss();
			boss->pos=boss_pos;
			entity_add(boss);

			for(int i=0;i<100;i++) {
				Entity* enemy=create_ocean_random();
				float dist=Easing::inQuad(Utils::rand_range(0.2,1))*3200.0;
				enemy->pos=boss_pos+Utils::vec_for_angle(Utils::rand_angle(),dist);
				entity_add(enemy);
			}
		}
		else if(selected_level==2) {	//crock
			Entity* boss=create_crock_boss();
			boss->pos=boss_pos;
			entity_add(boss);
			for(int i=0;i<100;i++) {
				Entity* enemy=create_reptile_random();
				float dist=Easing::inQuad(Utils::rand_range(0.2,1))*3200.0;
				enemy->pos=boss_pos+Utils::vec_for_angle(Utils::rand_angle(),dist);
				entity_add(enemy);
			}
		}
		else if(selected_level==3) {	//farm/barn
			Entity* boss=create_barn_boss();
			boss->pos=boss_pos;
			entity_add(boss);

			for(int i=0;i<100;i++) {
				Entity* enemy=create_farm_random();
				float dist=Easing::inQuad(Utils::rand_range(0.2,1))*3200.0;
				enemy->pos=boss_pos+Utils::vec_for_angle(Utils::rand_angle(),dist);
				entity_add(enemy);
			}
		}
		else if(selected_level==4) {	//fish/luna
			Entity* boss=create_luna_boss();
			boss->pos=boss_pos;
			entity_add(boss);
			for(int i=0;i<100;i++) {
				Entity* enemy=create_fish_random();
				float dist=Easing::inQuad(Utils::rand_range(0.2,1))*3200.0;
				enemy->pos=boss_pos+Utils::vec_for_angle(Utils::rand_angle(),dist);
				entity_add(enemy);
			}
		}
		else if(selected_level==5) {	//platypus
			Entity* boss=create_platypus_boss();
			boss->pos=boss_pos;
			entity_add(boss);
			for(int i=0;i<100;i++) {
				Entity* enemy=create_platypus_random();
				float dist=Easing::inQuad(Utils::rand_range(0.2,1))*3200.0;
				enemy->pos=boss_pos+Utils::vec_for_angle(Utils::rand_angle(),dist);
				entity_add(enemy);
			}
		}
		else {
			Entity* boss=create_walrus_boss(0);
			boss->pos=boss_pos;
			entity_add(boss);

			for(int i=0;i<100;i++) {
				Entity* enemy=create_walrus_random();
				float dist=Easing::inQuad(Utils::rand_range(0.2,1))*3200.0;
				enemy->pos=boss_pos+Utils::vec_for_angle(Utils::rand_angle(),dist);
				entity_add(enemy);
			}
		}



		/*
		for(int i=0;i<100;i++) {
			Entity* enemy;

			if(i%5==0) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_up),0);
			else if(i%5==1) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_down),1);
			else if(i%5==2) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_side),2);
			else enemy=create_enemy_suicide(Utils::vector_rand(graphic_enemy_suicide));

			enemy->pos=sf::Vector2f(Utils::rand_vec(-4000,4000));
			entity_add(enemy);
		}
		*/

		controls=Controls();
	}

	//entities
	void entity_add(Entity* entity) {
		node_ships.add_child(&entity->node_main);
		entities.entity_add(entity);
	}
	void entity_remove(Entity* entity) {
		node_ships.remove_child(&entity->node_main);
		entities.entity_remove(entity);
	}

	//utils
	void entity_add_timeout(Entity* entity,CompTimeout::Action action,float timeout) {
		CompTimeout* t=(CompTimeout*)entities.component_add(entity,Component::TYPE_TIMEOUT);
		t->set(action,timeout);
	}
	void entity_add_health(Entity* entity,float health) {
		if(!entity->comp_health) {
			entities.component_add(entity,Component::TYPE_HEALTH);
		}
		entity->comp_health->reset(health);
	}

	//entity local to entity local
	sf::Vector2f entity_rotate_vector(const Entity* e,const sf::Vector2f p) {
		float theta = Utils::deg_to_rad(e->angle+90);	//XXX get rid of 90deg offset
		float cs=cos(theta);
		float sn=sin(theta);

		return sf::Vector2f(
				p.x*cs-p.y*sn,
				p.x*sn+p.y*cs);
	}
	sf::Vector2f entity_unrotate_vector(const Entity* e,const sf::Vector2f p) {
		float theta = -Utils::deg_to_rad(e->angle+90);	//XXX get rid of 90deg offset
		float cs=cos(theta);
		float sn=sin(theta);

		return sf::Vector2f(
				p.x*cs-p.y*sn,
				p.x*sn+p.y*cs);
	}
	void entity_teleport(Entity* e,const sf::Vector2f& pos) {
		CompTeleportation* tele=(CompTeleportation*)entities.component_add(e,Component::TYPE_TELEPORTATION);
		tele->origin=e->pos;
		tele->destination=pos;
		tele->anim=0.0;
		tele->anim_duration=0.2;

		tele->origin_node=e->node_main;

		node_ships.add_child(&tele->origin_node);
		e->pos=pos;
	}

	CompShield* entity_add_shield(Entity* e) {
		CompShield* shield=(CompShield*)entities.component_add(e,Component::TYPE_SHIELD);

		const char* names[]={"inner level.png","mid level.png","outer level.png"};
		for(int i=0;i<3;i++) {
			Node::Ptr n(new Node());
			n->texture=Loader::get_texture((std::string)"player ships/Hammership/"+names[i]);
			n->origin=n->texture.get_size()*0.5f;
			shield->node.add_child(n.get());
			shield->layers.push_back(std::move(n));
		}
		shield->anim=Utils::rand_range(0,100);
		return shield;
	}

	Entity* create_player() {
		Entity* player=entities.entity_create();

		entities.component_add(player,Component::TYPE_SHAPE);
		player->comp_shape->collision_group=CompShape::COLLISION_GROUP_PLAYER;
		player->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_ENEMY|
				CompShape::COLLISION_GROUP_ENEMY_BULLET|
				CompShape::COLLISION_GROUP_TERRAIN);
		player->comp_shape->bounce=true;
		player->comp_shape->terrain_bounce=true;

		entities.component_add(player,Component::TYPE_DISPLAY);
		entities.attribute_add(player,Entity::ATTRIBUTE_PLAYER_CONTROL);
		entities.attribute_add(player,Entity::ATTRIBUTE_FRIENDLY);
		entities.attribute_add(player,Entity::ATTRIBUTE_INTEGRATE_POSITION);

		player->player_side=true;
		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(player,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT;

		entity_add_health(player,100);

		entity_show_on_minimap(player,Color(1,1,1,1));

		return player;
	}

	Entity* create_player_assaulter() {
		Entity* player=create_player();

		Texture tex=Loader::get_texture("player ships/Assaulter/assaulter.png");
		player->comp_display->add_texture_center(tex);
		player->comp_shape->add_quad_center(tex.get_size());

		CompEngine* e=(CompEngine*)entities.component_add(player,Component::TYPE_ENGINE);
		e->set(graphic_engine,sf::Vector2f(0,tex.get_size().y*0.5));

		//machine guns
		for(int i=0;i<2;i++) {
			CompGun* g=(CompGun*)entities.component_add(player,Component::TYPE_GUN);
			g->texture=Loader::get_texture("player ships/Assaulter/projectile1.png");
			g->pos.x=-29+i*29*2;
			g->pos.y=-19;
			g->bullet_speed=400;
			g->fire_timeout=0.06;
			g->angle=0;
			g->group=1;
		}
		//shotguns
		for(int i=0;i<2;i++) {
			CompGun* g=(CompGun*)entities.component_add(player,Component::TYPE_GUN);
			g->texture=Loader::get_texture("player ships/Assaulter/projectile2.png");
			g->pos.x=-29+i*29*2;
			g->pos.y=-19;
			g->bullet_speed=400;
			g->fire_timeout=0.6;
			g->angle=0;
			g->angle_spread=60;
			g->bullet_count=5;
		}
		//laser
		CompGun* g=(CompGun*)entities.component_add(player,Component::TYPE_GUN);
		g->gun_type=CompGun::GUN_LASER;
		g->texture=Loader::get_texture("player ships/Assaulter/railgun.png");
		g->pos.x=0;
		g->pos.y=-27;
		g->bullet_speed=800;
		g->fire_timeout=0.02;
		g->angle=0;
		g->group=2;

		return player;
	}

	Entity* create_player_bastion() {
		Entity* player=create_player();

		Texture tex_hammer=Loader::get_texture("player ships/Hammership/hammer.png");
		Node* hammer_node=player->comp_display->add_texture(tex_hammer,sf::Vector2f(0,0));
		hammer_node->pos=sf::Vector2f(0,0);
		hammer_node->origin=sf::Vector2f(tex_hammer.get_size().x*0.5f,-18);

		Texture tex=Loader::get_texture("player ships/Hammership/ship1.png");
		player->comp_display->add_texture_center(tex);

		GraphicNode* node_shining=player->comp_display->add_graphic_center(
				(Animation(Loader::get_texture("player ships/Hammership/shinning.png"),82,90,0.05)));
		node_shining->repeat=true;
		node_shining->duration_hidden=3.0f;

		player->comp_shape->add_quad_center(tex.get_size());

		//shotguns
		for(int i=0;i<2;i++) {
			CompGun* g=(CompGun*)entities.component_add(player,Component::TYPE_GUN);
			g->texture=Loader::get_texture("player ships/Hammership/projectile.png");
			g->pos.x=0;
			g->pos.y=0;
			g->bullet_speed=800;
			g->fire_timeout=0.6;
			g->angle=90+i*180;
			g->angle_spread=180;
			g->bullet_count=15;
			g->group=1-i;
		}

		for(int i=0;i<2;i++) {
			CompEngine* e=(CompEngine*)entities.component_add(player,Component::TYPE_ENGINE);
			e->set(graphic_engine,sf::Vector2f(-tex.get_size().x*0.5+23+36*i,tex.get_size().y*0.5));
		}

		//grav force
		CompGravityForce* grav=(CompGravityForce*)entities.component_add(player,Component::TYPE_GRAVITY_FORCE);
		grav->enabled=false;
		grav->radius=300;
		grav->power_center=-100;
		grav->power_edge=400;

		//hammer
		CompGravityForce* hammer_grav=(CompGravityForce*)entities.component_add(player,Component::TYPE_GRAVITY_FORCE);
		hammer_grav->enabled=false;
		hammer_grav->radius=400;
		hammer_grav->power_center=-1000;
		hammer_grav->power_edge=-500;

		CompHammer* hammer=(CompHammer*)entities.component_add(player,Component::TYPE_HAMMER);
		hammer->my_gravity_force=hammer_grav;
		hammer->hammer_node=hammer_node;


		return player;
	}
	Entity* create_player_fighter() {
		Entity* player=create_player();

		Texture tex=Loader::get_texture("player ships/Slasher/slasher.png");

		Node* node_blade1=player->comp_display->add_texture(Loader::get_texture("player ships/Slasher/blade1.png"),sf::Vector2f(-24,-3));
		Node* node_blade2=player->comp_display->add_texture(Loader::get_texture("player ships/Slasher/blade2.png"),sf::Vector2f(24,-3));

		node_blade1->origin=sf::Vector2f(node_blade1->texture.get_size().x*0.5f,node_blade1->texture.get_size().y);
		node_blade2->origin=sf::Vector2f(node_blade2->texture.get_size().x*0.5f,node_blade2->texture.get_size().y);

		player->comp_display->add_texture_center(tex);
		player->comp_shape->add_quad_center(tex.get_size());

		for(int i=0;i<2;i++) {
			CompEngine* e=(CompEngine*)entities.component_add(player,Component::TYPE_ENGINE);
			e->set(graphic_engine,sf::Vector2f(-tex.get_size().x*0.5+16+i*37,tex.get_size().y*0.5));
		}

		//grappling hook
		CompGun* g=(CompGun*)entities.component_add(player,Component::TYPE_GUN);
		g->gun_type=CompGun::GUN_HOOK;
		g->hook.set_textures(Loader::get_texture("player ships/Slasher/hook.png"),
				Loader::get_texture("player ships/Slasher/chain.png"));
		player->node_main.add_child(&g->hook.node_root);
		g->hook.max_length=300;

		g->pos.x=0;
		g->pos.y=-19;
		g->group=2;

		CompFighterShip* fighter=(CompFighterShip*)entities.component_add(player,Component::TYPE_FIGHTER_SHIP);
		fighter->node_blade1=node_blade1;
		fighter->node_blade2=node_blade2;

		return player;
	}

	Entity* create_player_engineer() {
		Entity* player=create_player();
		Texture tex=Loader::get_texture("player ships/Engineer/engineer.png");
		player->comp_display->add_texture_center(tex);
		player->comp_shape->add_quad_center(tex.get_size());

		//player->comp_display->add_texture(tex_hook,-tex.get_size()*0.5f+sf::Vector2f(1,-15));
		//player->comp_display->add_texture(tex_hook,-tex.get_size()*0.5f+sf::Vector2f(53-19-1,-15));

		//hooks
		Texture tex_hook=Loader::get_texture("player ships/Engineer/hook.png");
		Texture tex_chain=Loader::get_texture("player ships/Slasher/chain.png");
		for(int i=0;i<2;i++) {
			CompGun* g=(CompGun*)entities.component_add(player,Component::TYPE_GUN);
			g->gun_type=CompGun::GUN_HOOK;
			g->hook.set_textures(tex_hook,tex_chain);
			player->node_main.add_child(&g->hook.node_root);
			g->hook.max_length=300;

			g->pos.x=-17+i*32;
			g->pos.y=-23;
			g->group=i;
			g->hook.pull_mode=true;
		}


		CompEngine* e=(CompEngine*)entities.component_add(player,Component::TYPE_ENGINE);
		e->set(graphic_engine,sf::Vector2f(0,tex.get_size().y*0.5));

		return player;
	}


	Entity* create_bullet(const Texture& tex,bool player_side,float scale=1.0) {

		Entity* bullet=entities.entity_create();

		entities.component_add(bullet,Component::TYPE_DISPLAY);
		bullet->comp_display->add_texture_center(tex,scale);

		entities.component_add(bullet,Component::TYPE_SHAPE);
		bullet->comp_shape->add_quad_center(tex.get_size()*scale);

		if(player_side) {
			bullet->comp_shape->collision_group=CompShape::COLLISION_GROUP_PLAYER_BULLET;
			bullet->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_ENEMY);
		}
		else {
			bullet->comp_shape->collision_group=CompShape::COLLISION_GROUP_ENEMY_BULLET;
			bullet->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_PLAYER);
		}
		bullet->player_side=player_side;

		entities.attribute_add(bullet,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entities.attribute_add(bullet,Entity::ATTRIBUTE_INTEGRATE_POSITION);
		entity_add_timeout(bullet,CompTimeout::ACTION_REMOVE_ENTITY,2.0f);

		entity_add_health(bullet,1);

		return bullet;
	}
	Entity* create_mine() {
		Entity* e=create_bullet(Loader::get_texture("player ships/Hammership/sticky bomb.png"),true);
		CompTimeout* t=(CompTimeout*)entities.component_get(e,Component::TYPE_TIMEOUT);
		t->timeout=20.0f;
		e->tmp_damage=100.0f;
		entity_add_health(e,10);
		return e;
	}
	Entity* create_candy_mine() {
		Entity* e=create_mine();
		CompTimeout* t=(CompTimeout*)entities.component_get(e,Component::TYPE_TIMEOUT);
		t->timeout=2.0f;
		t->action=CompTimeout::ACTION_BIG_EXPLOSION;
		e->tmp_damage=200.0f;

		entities.component_remove(e->comp_shape);

		entity_add_health(e,10);
		entities.attribute_add(e,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entities.attribute_add(e,Entity::ATTRIBUTE_ATTRACT);

		return e;
	}
	Entity* create_follower() {

		float scale=1.0;

		Entity* k=entities.entity_create();

		Texture tex=Loader::get_texture("player ships/Engineer/helper1.png");
		entities.component_add(k,Component::TYPE_DISPLAY);
		k->comp_display->add_texture_center(tex,scale);

		entities.component_add(k,Component::TYPE_SHAPE);

		k->comp_shape->add_quad_center(tex.get_size()*scale);
		entities.attribute_add(k,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entities.attribute_add(k,Entity::ATTRIBUTE_FRIENDLY);
		entities.attribute_add(k,Entity::ATTRIBUTE_INTEGRATE_POSITION);

		k->comp_shape->collision_group=CompShape::COLLISION_GROUP_PLAYER;
		k->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_ENEMY|
				CompShape::COLLISION_GROUP_ENEMY_BULLET/*|
				CompShape::COLLISION_GROUP_TERRAIN*/);

		k->comp_shape->bounce=true;
		k->comp_shape->terrain_bounce=true;
		k->player_side=true;

		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(k,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_BLINK;

		entity_show_on_minimap(k,Color(0,0,1,1));

		entity_add_health(k,100);

		CompAI* ai=(CompAI*)entities.component_add(k,Component::TYPE_AI);
		ai->ai_type=CompAI::AI_FOLLOWER;

		CompGun* gun=(CompGun*)entities.component_add(k,Component::TYPE_GUN);
		gun->texture=Loader::get_texture("player ships/Engineer/helper1proj.png");
		gun->bullet_speed=300;
		gun->fire_timeout=0.3;

		/*
		float aim_dist=200;
		if(dir==0) {
			ai->target_offset=sf::Vector2f(0,aim_dist);
			gun->angle=0;
		}
		else if(dir==1) {
			ai->target_offset=sf::Vector2f(0,-aim_dist);
			gun->angle=180;
		}
		else if(dir==2) {
			float g_dir=Utils::rand_sign();
			ai->target_offset=sf::Vector2f(aim_dist*g_dir,0);
			gun->angle=(g_dir>0 ? 270 : 90);
		}
		*/

		return k;
	}

	Entity* create_missile(const Graphic& g,Entity* target,bool player_side) {
		float scale=2.0;

		Entity* missile=entities.entity_create();
		entities.component_add(missile,Component::TYPE_DISPLAY);
		missile->comp_display->add_graphic_center(g,scale);

		entities.component_add(missile,Component::TYPE_SHAPE);
		missile->comp_shape->add_quad_center(g.get_texture().get_size()*scale);

		if(player_side) {
			missile->comp_shape->collision_group=CompShape::COLLISION_GROUP_PLAYER_BULLET;
			missile->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_ENEMY);
		}
		else {
			missile->comp_shape->collision_group=CompShape::COLLISION_GROUP_ENEMY;
			missile->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_PLAYER |
					CompShape::COLLISION_GROUP_PLAYER_BULLET);
		}
		missile->player_side=player_side;

		entities.attribute_add(missile,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entity_add_timeout(missile,CompTimeout::ACTION_REMOVE_ENTITY,10.0f);
		entities.attribute_add(missile,Entity::ATTRIBUTE_INTEGRATE_POSITION);

		CompAI* ai=(CompAI*)entities.component_add(missile,Component::TYPE_AI);
		ai->ai_type=CompAI::AI_MISSILE;
		if(target) {
			ai->target=entities.entity_ref_create(target);
			ai->my_refs.push_back(ai->target);
		}

		CompEngine* eng=(CompEngine*)entities.component_add(missile,Component::TYPE_ENGINE);
		eng->set(graphic_engine,sf::Vector2f(0,g.get_texture().get_size().y*scale*0.5));

		entity_add_health(missile,10);

		return missile;
	}

	Entity* create_enemy(const Graphic& g,float scale=2.0) {

		Entity* k=entities.entity_create();
		entities.component_add(k,Component::TYPE_DISPLAY);
		GraphicNode* main_node=k->comp_display->add_graphic_center(g,scale);
		main_node->repeat=true;

		entities.component_add(k,Component::TYPE_SHAPE);
		k->comp_shape->add_quad_center(g.get_texture().get_size()*scale);
		entities.attribute_add(k,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entities.attribute_add(k,Entity::ATTRIBUTE_ENEMY);

		k->comp_shape->collision_group=CompShape::COLLISION_GROUP_ENEMY;
		k->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
				CompShape::COLLISION_GROUP_PLAYER|
				CompShape::COLLISION_GROUP_PLAYER_BULLET);
		k->comp_shape->bounce=true;
		k->comp_shape->terrain_bounce=true;

		entities.attribute_add(k,Entity::ATTRIBUTE_INTEGRATE_POSITION);

		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(k,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_BLINK;

		entity_show_on_minimap(k,Color(1,0,0,1));

		entity_add_health(k,100);
		return k;
	}
	Entity* create_enemy_melee(const Graphic& g) {
		Entity* k=create_enemy(g);
		CompAI* ai=(CompAI*)entities.component_add(k,Component::TYPE_AI);
		ai->ai_type=CompAI::AI_MELEE;
		return k;
	}
	Entity* create_enemy_suicide(const Graphic& g) {
		Entity* k=create_enemy_melee(g);
		k->tmp_damage=100.0f;
		k->comp_health->reset(5.0f);
		k->comp_shape->take_damage_terrain_mult=0.01;
		return k;
	}
	Entity* create_enemy_shooter(const Graphic& g,int dir/*0=up,1=down,2=side*/) {
		Entity* k=create_enemy(g);
		CompAI* ai=(CompAI*)entities.component_add(k,Component::TYPE_AI);
		ai->ai_type=CompAI::AI_SHOOTER;

		CompGun* gun=(CompGun*)entities.component_add(k,Component::TYPE_GUN);
		gun->texture=Loader::get_texture("general assets/proj2.png");
		gun->bullet_speed=300;
		gun->fire_timeout=0.3;

		float aim_dist=200;

		if(dir==0) {
			ai->target_offset=sf::Vector2f(0,aim_dist);
			gun->angle=0;
		}
		else if(dir==1) {
			ai->target_offset=sf::Vector2f(0,-aim_dist);
			gun->angle=180;
		}
		else if(dir==2) {
			float g_dir=Utils::rand_sign();
			ai->target_offset=sf::Vector2f(aim_dist*g_dir,0);
			gun->angle=(g_dir>0 ? 270 : 90);
		}


		return k;
	}


	//walrus
	Entity* create_walrus_boss(int split_level) {
		Entity* e;

		if(split_level==0) {
			Graphic graphic(Animation(Loader::get_texture("enemies/Walrus/boss/walrus boss.png"),92,72,0.1));
			e=create_enemy(graphic);

			GraphicNode::Ptr& node=e->comp_display->nodes[0];
			node->repeat=false;
			node->speed=0;

			CompShowDamage* damage=(CompShowDamage*)entities.component_add(e,Component::TYPE_SHOW_DAMAGE);
			damage->damage_type=CompShowDamage::DAMAGE_TYPE_ANIMATION_PROGRESS;
			damage->animation_progress_node=node.get();
		}
		else if(split_level==1) {
			e=create_enemy(Graphic(Loader::get_texture("enemies/Walrus/boss/split1.png")));
		}
		else if(split_level==2) {
			e=create_enemy(Graphic(Loader::get_texture("enemies/Walrus/boss/split2.png")));
		}
		else if(split_level==3) {
			e=create_enemy(Graphic(Loader::get_texture("enemies/Walrus/boss/split3.png")));
		}

		float walrus_power=1.0f/std::pow(2.0,(float)split_level);
		e->comp_health->reset(1000.0f*walrus_power);

		CompAI2* ai=(CompAI2*)entities.component_add(e,Component::TYPE_AI2);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SAFE_FOLLOW);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SPAWN_MINIONS);
		ai->spawn_ids.push_back("enemy/walrus/random");
		ai->spawn_timeout=Utils::rand_range(1.5,3);
		ai->spawn_interval=3.0/walrus_power;
		ai->safe_follow_distance=Utils::rand_range(280,320);

		e->comp_shape->terrain_bounce=false;
		e->comp_shape->take_damage_terrain_mult=0.0f;

		if(split_level==0) {
			ai->death_spawn_ids.push_back(std::make_pair("enemy/walrus/boss_split1",2));
		}
		else if(split_level==1) {
			ai->death_spawn_ids.push_back(std::make_pair("enemy/walrus/boss_split2",2));
		}
		else if(split_level==2) {
			ai->death_spawn_ids.push_back(std::make_pair("enemy/walrus/boss_split3",2));
		}

		entities.attribute_add(e,Entity::ATTRIBUTE_BOSS);

		return e;
	}
	Entity* create_walrus_random() {
		std::stringstream ss;
		ss<<"enemies/Walrus/walrus"<<Utils::rand_range_i(1,8)<<".png";
		Graphic g(Loader::get_texture(ss.str()));
		return create_enemy_melee(g);
	}

	Entity* create_octopuss_boss() {
		Graphic graphic(Animation(Loader::get_texture("enemies/Ocean/boss/octopuss.png"),99,126,0.1));
		Entity* e=create_enemy(graphic);

		e->comp_health->reset(1000.0f);
		e->comp_shape->terrain_bounce=false;
		e->comp_shape->take_damage_terrain_mult=0.0f;
		entities.attribute_add(e,Entity::ATTRIBUTE_BOSS);

		CompAI2* ai=(CompAI2*)entities.component_add(e,Component::TYPE_AI2);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SAFE_FOLLOW);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SPAWN_MINIONS);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_AIM_SHOOT);
		ai->spawn_ids.push_back("enemy/ocean/random");
		ai->spawn_timeout=3.0;
		ai->spawn_interval=3.0;
		ai->safe_follow_distance=300;

		CompGun* gun=(CompGun*)entities.component_add(e,Component::TYPE_GUN);
		gun->texture=Loader::get_texture("enemies/Ocean/boss/projectile.png");
		gun->bullet_speed=400;
		gun->fire_timeout=2.0;

		for(int i=1;i<=8;i++) {
			std::stringstream ss;
			ss<<"enemies/Ocean/boss/splashscreen"<<i<<".png";
			gun->splatter_textures.push_back(Loader::get_texture(ss.str()));
		}

		return e;
	}
	Entity* create_ocean_random() {
		static std::vector<std::tuple<int,std::string,int>> defs;
		if(defs.empty()) {
				defs.push_back(std::make_tuple(0,"Melee/ocean1.png",0));
				defs.push_back(std::make_tuple(0,"Melee/ocean8.png",0));
				defs.push_back(std::make_tuple(1,"Shooter/ocean4.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/ocean7.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/ocean9.png",2));
				defs.push_back(std::make_tuple(2,"Suicide/ocean2.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/ocean3.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/ocean5.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/ocean6.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/ocean10.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/ocean11.png",0));
		}
		return create_from_defs(defs,"enemies/Ocean/");

		/*
		int type=Utils::rand_range_i(1,11);
		if(type==1) {
			return create_enemy_melee(Graphic(Loader::get_texture("enemies/Ocean/Melee/ocean1.png")));
		}
		else if(type==2) {
			return create_enemy_melee(Graphic(Loader::get_texture("enemies/Ocean/Melee/ocean8.png")));
		}
		else if(type==3) {
			return create_enemy_shooter(Graphic(Loader::get_texture("enemies/Ocean/Shooter/ocean4.png")),1);
		}
		else if(type==4) {
			return create_enemy_shooter(Graphic(Loader::get_texture("enemies/Ocean/Shooter/ocean7.png")),1);
		}
		else if(type==5) {
			return create_enemy_shooter(Graphic(Loader::get_texture("enemies/Ocean/Shooter/ocean9.png")),2);
		}
		else if(type==6) {
			return create_enemy_suicide(Graphic(Loader::get_texture("enemies/Ocean/Suicide/ocean2.png")));
		}
		else if(type==7) {
			return create_enemy_suicide(Graphic(Loader::get_texture("enemies/Ocean/Suicide/ocean3.png")));
		}
		else if(type==8) {
			return create_enemy_suicide(Graphic(Loader::get_texture("enemies/Ocean/Suicide/ocean5.png")));
		}
		else if(type==9) {
			return create_enemy_suicide(Graphic(Loader::get_texture("enemies/Ocean/Suicide/ocean6.png")));
		}
		else if(type==10) {
			return create_enemy_suicide(Graphic(Loader::get_texture("enemies/Ocean/Suicide/ocean10.png")));
		}
		else {
			return create_enemy_suicide(Graphic(Loader::get_texture("enemies/Ocean/Suicide/ocean11.png")));
		}
		*/
	}

	Entity* create_crock_boss() {

		Graphic graphic(Animation(Loader::get_texture("enemies/Reptiles/boss/crock.png"),66,119,0.5));
		Entity* e=create_enemy(graphic);

		GraphicNode::Ptr& node=e->comp_display->nodes[0];
		node->speed=0;

		e->comp_health->reset(1000.0f);
		e->comp_shape->terrain_bounce=false;
		e->comp_shape->take_damage_terrain_mult=0.0f;
		entities.attribute_add(e,Entity::ATTRIBUTE_BOSS);

		CompAI2* ai=(CompAI2*)entities.component_add(e,Component::TYPE_AI2);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SAFE_FOLLOW);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SHOOT);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SPAWN_MINIONS);
		ai->spawn_ids.push_back("enemy/reptile/random");
		ai->spawn_timeout=3.0;
		ai->spawn_interval=3.0;
		ai->safe_follow_distance=300;
		ai->spawn_node=e->comp_display->nodes[0].get();
		ai->spawn_node->speed=0;
		ai->spawn_vel=sf::Vector2f(0,300);

		for(int i=0;i<2;i++) {
			CompGun* gun=(CompGun*)entities.component_add(e,Component::TYPE_GUN);
			gun->texture=Loader::get_texture("general assets/beam.png");
			gun->gun_type=CompGun::GUN_LASER;
			gun->angle=180;
			gun->pos=sf::Vector2f(-12+i*24,-48);
		}

		return e;
	}
	Entity* create_reptile_random() {
		static std::vector<std::tuple<int,std::string,int>> defs;
		if(defs.empty()) {
				defs.push_back(std::make_tuple(0,"Melee/reptile1.png",0));
				defs.push_back(std::make_tuple(0,"Melee/reptile2.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/reptile3.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/reptile4.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/reptile5.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/reptile6.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/reptile7.png",0));
		}
		return create_from_defs(defs,"enemies/Reptiles/");
	}


	Entity* create_barn_boss() {
		Graphic graphic(Animation(Loader::get_texture("enemies/Farm/boss/barn.png"),62,114,0.1));
		Entity* e=create_enemy(graphic);

		GraphicNode::Ptr& node=e->comp_display->nodes[0];
		node->speed=0;

		e->comp_display->nodes[0]->pos=-sf::Vector2f(62,92);

		e->comp_health->reset(1000.0f);
		e->comp_shape->quads[0]=Quad(-sf::Vector2f(62,92),sf::Vector2f(62,92));
		e->comp_shape->terrain_bounce=false;
		e->comp_shape->take_damage_terrain_mult=0.0f;
		entities.attribute_add(e,Entity::ATTRIBUTE_BOSS);

		CompAI2* ai=(CompAI2*)entities.component_add(e,Component::TYPE_AI2);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SAFE_FOLLOW);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SPAWN_MINIONS);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_AIM_SHOOT);
		ai->spawn_ids.push_back("enemy/farm/random");
		ai->spawn_timeout=3.0;
		ai->spawn_interval=3.0;
		ai->safe_follow_distance=300;

		ai->spawn_node=e->comp_display->nodes[0].get();
		ai->spawn_node->speed=0;
		ai->spawn_vel=sf::Vector2f(0,300);

		CompGun* gun=(CompGun*)entities.component_add(e,Component::TYPE_GUN);
		gun->texture=Loader::get_texture("enemies/Farm/boss/egg.png");
		gun->bullet_speed=400;
		gun->fire_timeout=2.0;
		gun->splatter_textures.push_back(Loader::get_texture("enemies/Farm/boss/eggsplash.png"));

		return e;
	}
	Entity* create_farm_random() {
		static std::vector<std::tuple<int,std::string,int>> defs;
		if(defs.empty()) {
				defs.push_back(std::make_tuple(1,"Shooter/cow1.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/cow2.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/cow3.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/cow4.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/cow5.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/cow6.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/cow7.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/cow8.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig1.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig2.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig3.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig4.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig5.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig6.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig7.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/pig8.png",1));
				defs.push_back(std::make_tuple(2,"Suicide/chicken1.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/chicken2.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/chicken3.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/chicken4.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/chicken5.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/chicken6.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/chicken7.png",0));
					defs.push_back(std::make_tuple(2,"Suicide/chicken8.png",0));
		}
		return create_from_defs(defs,"enemies/Farm/");
	}

	Entity* create_luna_boss() {
		Graphic graphic(Animation(Loader::get_texture("enemies/Fish/boss/rockethatch.png"),60,57,0.5));
		Entity* e=create_enemy(graphic);

		GraphicNode::Ptr& node=e->comp_display->nodes[0];
		node->speed=0;

		e->comp_health->reset(1000.0f);
		e->comp_shape->terrain_bounce=false;
		e->comp_shape->take_damage_terrain_mult=0.0f;
		entities.attribute_add(e,Entity::ATTRIBUTE_BOSS);

		CompAI2* ai=(CompAI2*)entities.component_add(e,Component::TYPE_AI2);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SAFE_FOLLOW);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SHOOT);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SPAWN_MINIONS);
		ai->spawn_ids.push_back("enemy/fish/rocket");
		ai->spawn_timeout=3.0;
		ai->spawn_interval=3.0;
		ai->safe_follow_distance=300;
		ai->spawn_node=e->comp_display->nodes[0].get();
		ai->spawn_node->speed=0;
		ai->spawn_vel=sf::Vector2f(0,300);
		ai->spawn_pos=sf::Vector2f(12,4);
		ai->spawn_angle=90;

		CompAI2* ai2=(CompAI2*)entities.component_add(e,Component::TYPE_AI2);
		ai2->behaviors.push_back(CompAI2::AI_BEHAVIOR_SPAWN_MINIONS);
		ai2->spawn_ids.push_back("enemy/fish/minion");
		ai2->spawn_timeout=2.0;
		ai2->spawn_interval=4.0;
		ai2->spawn_pos=sf::Vector2f(0,52);
		ai2->spawn_vel=sf::Vector2f(0,300);


		CompGun* gun=(CompGun*)entities.component_add(e,Component::TYPE_GUN);
		gun->texture=Loader::get_texture("general assets/beam.png");
		gun->gun_type=CompGun::GUN_LASER;
		gun->angle=180;
		gun->pos=sf::Vector2f(-10,32);

		CompSpawnPieces* pieces=(CompSpawnPieces*)entities.component_add(e,Component::TYPE_SPAWN_PIECES);
		pieces->texture=Loader::get_texture("enemies/Fish/boss/pieces.png");
		const int piece_map[][6]={
				{35,11,12,30,0,0},
				{14,0,36,13,12,0},
				{51,8,3,26,48,0},
				{47,10,5,28,51,0},
				{54,5,6,25,56,0},
				{17,11,8,4,0,30},
				{0,5,7,19,8,30},
				{3,9,7,24,15,30},
				{8,5,6,32,22,30},
				{11,12,6,31,28,30},
				{13,14,7,34,34,30},
				{15,15,20,42,41,30},
				{35,34,14,23,0,72},
				{25,11,12,30,14,72},
				{31,25,9,11,26,72}
		};
		for(int i=0;i<15;i++) {
			CompSpawnPieces::Piece piece;
			piece.pos.x=piece_map[i][0];
			piece.pos.y=piece_map[i][1];
			piece.map_size.x=piece_map[i][2];
			piece.map_size.y=piece_map[i][3];
			piece.map_pos.x=piece_map[i][4];
			piece.map_pos.y=piece_map[i][5];
			pieces->pieces.push_back(std::move(piece));
		}

		return e;
	}
	Entity* create_fish_rocket() {
		return create_missile(Graphic(Loader::get_texture("enemies/Fish/boss/rocket.png")),player,false);
	}
	Entity* create_fish_minion() {
		return create_enemy_melee(Graphic(Loader::get_texture("enemies/Fish/boss/lunajunior.png")));
	}
	Entity* create_fish_random() {
		static std::vector<std::tuple<int,std::string,int>> defs;
		if(defs.empty()) {
				defs.push_back(std::make_tuple(0,"Melee/fish10.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish11.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish12.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish13.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish14.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish15.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish16.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish17.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish18.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish20.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish21.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish22.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish23.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish24.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish25.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish26.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish27.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish28.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish29.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish30.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish31.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish32.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish33.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish34.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish35.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish36.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish37.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish38.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish39.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish3.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish40.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish41.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish42.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish43.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish44.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish45.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish46.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish47.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish48.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish49.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish4.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish5.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish7.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish8.png",0));
				defs.push_back(std::make_tuple(0,"Melee/fish9.png",0));
				defs.push_back(std::make_tuple(1,"Shooter/fish50.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish51.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish52.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish53.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish54.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish55.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish56.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish57.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish58.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish59.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish60.png",1));
				defs.push_back(std::make_tuple(1,"Shooter/fish61.png",1));
				defs.push_back(std::make_tuple(2,"Suicide/fish62.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish63.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish64.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish65.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish66.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish67.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish68.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish69.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish70.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish71.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish72.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish73.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish74.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish75.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish76.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish78.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish79.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish80.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish81.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish82.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish83.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish84.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish85.png",0));
				defs.push_back(std::make_tuple(2,"Suicide/fish86.png",0));
		}
		return create_from_defs(defs,"enemies/Fish/");
	}

	Entity* create_platypus_boss() {
		Graphic graphic(Loader::get_texture("enemies/Platypus/boss/phase1.png"));
		Entity* e=create_enemy(graphic);

		e->comp_health->reset(1000.0f);

		e->comp_shape->quads[0].p1=sf::Vector2f(-52,-31)*2.0f;
		e->comp_shape->quads[0].p2=sf::Vector2f(52,38)*2.0f;
		e->comp_shape->terrain_bounce=false;
		e->comp_shape->take_damage_terrain_mult=0.0f;
		entities.attribute_add(e,Entity::ATTRIBUTE_BOSS);

		CompAI2* ai=(CompAI2*)entities.component_add(e,Component::TYPE_AI2);
		ai->behaviors.push_back(CompAI2::AI_BEHAVIOR_SAFE_FOLLOW);
		ai->safe_follow_distance=300;

		Graphic graphic_engine(Animation(Loader::get_texture("enemies/Platypus/boss/engine fire.png"),22,24,0.1));
		for(int i=0;i<2;i++) {
			CompEngine* eng=(CompEngine*)entities.component_add(e,Component::TYPE_ENGINE);
			eng->set(graphic_engine,sf::Vector2f(-44+i*88,32)*2.0f);
		}

		entities.component_add(e,Component::TYPE_PLATYPUS_BOSS);

		CompSpawnPieces* pieces=(CompSpawnPieces*)entities.component_add(e,Component::TYPE_SPAWN_PIECES);
		pieces->texture=Loader::get_texture("enemies/Platypus/boss/pieces.png");
		const int piece_map[][6]={
				{16,72,5,18,0,0},
				{84,72,5,18,5,0},
				{19,37,16,17,10,0},
				{70,37,16,17,26,0},
				{54,21,12,7,42,0},
				{39,21,16,7,54,0},
				{0,60,16,24,70,0},
				{89,60,16,24,86,0},
				{20,85,27,19,0,24},
				{58,85,27,19,27,24},
				{70,54,19,16,54,24},
				{72,0,13,27,73,24},
				{16,54,19,16,86,24},
				{35,54,35,16,0,51},
				{40,25,25,24,35,51},
				{47,81,11,9,60,51},
				{74,27,9,19,71,51},
				{47,70,11,13,80,51},
				{19,66,28,5,91,51},
				{58,66,28,5,0,75}
		};
		for(int i=0;i<20;i++) {
			CompSpawnPieces::Piece piece;
			piece.pos.x=piece_map[i][0];
			piece.pos.y=piece_map[i][1];
			piece.map_size.x=piece_map[i][2];
			piece.map_size.y=piece_map[i][3];
			piece.map_pos.x=piece_map[i][4];
			piece.map_pos.y=piece_map[i][5];
			pieces->pieces.push_back(std::move(piece));
		}

		return e;
	}


	Entity* create_platypus_random() {
		static std::vector<std::tuple<int,std::string,int>> defs;
		if(defs.empty()) {
			defs.push_back(std::make_tuple(0,"Melee/platypus1.png",0));
			defs.push_back(std::make_tuple(0,"Melee/platypus2.png",0));
			defs.push_back(std::make_tuple(3,"Suicide/platypus3.png",0));
			defs.push_back(std::make_tuple(3,"Suicide/platypus4.png",0));
			defs.push_back(std::make_tuple(3,"Suicide/platypus5.png",0));
			defs.push_back(std::make_tuple(3,"Suicide/platypus6.png",0));
			defs.push_back(std::make_tuple(3,"Suicide/platypus7.png",0));
			defs.push_back(std::make_tuple(3,"Suicide/platypus8.png",0));
		}
		return create_from_defs(defs,"enemies/Platypus/");
	}


	Entity* create_from_defs(const std::vector<std::tuple<int,std::string,int>>& defs,const std::string prepend) {
		if(defs.empty()) {
			return nullptr;
		}
		const std::tuple<int,std::string,int> selected=Utils::vector_rand(defs);
		std::string tex_filename=prepend+std::get<1>(selected);
		if(std::get<0>(selected)==0) {
			return create_enemy_melee(Graphic(Loader::get_texture(tex_filename)));
		}
		else if(std::get<0>(selected)==1) {
			return create_enemy_shooter(Graphic(Loader::get_texture(tex_filename)),std::get<2>(selected));
		}
		else {
			return create_enemy_suicide(Graphic(Loader::get_texture(tex_filename)));
		}
	}
	Entity* create_by_id(const std::string& id) {
		if(entity_create_map.count(id)>0) {
			return entity_create_map[id]();
		}
		printf("WARN: can't create entity %s\n",id.c_str());
		return nullptr;
	}

	sf::Vector2f target_lead(sf::Vector2f target_pos,sf::Vector2f target_vel,float bullet_vel) {
		  float a = bullet_vel*bullet_vel - Utils::vec_dot(target_vel,target_vel);
		  float b = -2.0f*Utils::vec_dot(target_vel,target_pos);
		  float c = -Utils::vec_dot(target_pos,target_pos);
		  sf::Vector2f p=target_pos+Utils::largest_root_of_quadratic_equation(a,b,c)*target_vel;
		  return Utils::vec_normalize(p)*bullet_vel;
	}

	/*
	void add_swarm(int count,int pattern) {
		SwarmManager* swarm=new SwarmManager();
		swarm->pattern=pattern;

		//sf::Vector2f start_pos=player->pos+Utils::vec_for_angle_deg(Utils::rand_range(270,360+90),1000);
		sf::Vector2f start_pos=player->pos+Utils::vec_for_angle_deg(Utils::rand_range(0,180)+180,1000);

		for(int i=0;i<count;i++) {
			Entity* e=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_down),1);
			Component* ai=entities.component_get(e,Component::TYPE_AI);
			if(ai) {
				entities.component_remove(ai);
			}
			swarm->entities.push_back(entities.entity_ref_create(e));
			e->pos=start_pos;
			entity_add(e);
		}
		//for(int i=0;i<count;i++) {
		//	sf::Vector2f p=swarm->get_entity_pos(i).pos;
		//	entity_teleport(swarm->entities[i]->entity,player->pos+p);
		//}

		swarms.push_back(swarm);
	}
	*/

	void add_big_explosion(sf::Vector2f pos,bool player_side) {
		float radius=150.0f;
		float radius2=radius*radius;

		/*
		for(int i=0;i<120;i++) {
			add_decal(Utils::vector_rand(graphic_explosion),
					pos+Utils::vec_for_angle_deg(Utils::rand_range(0,360),Utils::rand_range(0,radius)));
		}
		*/
		add_decal_explosion(pos,1.0);

		sf::Vector2f d_size=sf::Vector2f(1,1)*radius*1.5f;
		sf::Vector2f d_pos=pos-d_size*0.5f;
		terrain.damage_area(sf::FloatRect(d_pos,d_size),100);

		for(Component* comp : entities.component_list(Component::TYPE_SHAPE)) {
			if(((CompShape*)comp)->enabled && comp->entity->player_side!=player_side) {
				float dist=Utils::vec_length_fast(comp->entity->pos-pos);

				if(dist<radius2) {
					entity_damage(comp->entity,Utils::lerp(500,0,dist/radius2));
				}
			}
		}

	}

	Entity* add_splatter(const Texture& texture) {
		float duration=6.0;
		float scale=4.0f;

		Entity* e=entities.entity_create();
		CompSplatter* splatter=(CompSplatter*)entities.component_add(e,Component::TYPE_SPLATTER);
		Node* node=splatter->add_texture(texture);
		node->scale=sf::Vector2f(scale,scale);
		splatter->set_duration(duration);

		e->pos=sf::Vector2f(Utils::rand_range(0,size.x-texture.get_size().x*scale),Utils::rand_range(0,size.y-texture.get_size().y*scale));

		entity_add_timeout(e,CompTimeout::ACTION_REMOVE_ENTITY,duration);
		entity_add(e);

		return e;
	}

	Entity* add_decal(const Graphic& grap,sf::Vector2f pos) {
		Entity* e=entities.entity_create();
		entities.component_add(e,Component::TYPE_DISPLAY);
		e->comp_display->add_graphic_center(grap);

		if(grap.is_animated()) {
			entity_add_timeout(e,CompTimeout::ACTION_REMOVE_ENTITY,grap.animation.duration);
		}
		else {
			entity_add_timeout(e,CompTimeout::ACTION_REMOVE_ENTITY,1.0f);
		}
		e->pos=pos;
		entity_add(e);
		return e;
	}
	Entity* add_decal(const std::vector<Graphic>& grap,sf::Vector2f pos) {
		if(grap.size()==0) {
			return nullptr;
		}
		return add_decal(Utils::vector_rand(grap),pos);
	}
	void add_decal_explosion(sf::Vector2f pos,float size) {	//size 0-1
		if(size<=0.1) {
			add_decal(graphic_explosion,pos);
			return;
		}

		int count=std::min(10,(int)(size*10));
		for(int i=0;i<count;i++) {
			Entity* e=entities.entity_create();
			entity_add_timeout(e,CompTimeout::ACTION_REMOVE_ENTITY,Utils::rand_range(0.5,0.8)*size);
			e->pos=pos;
			e->vel=Utils::vec_for_angle(Utils::rand_angle(),Utils::rand_range(250,350));
			entities.component_add(e,Component::TYPE_FLAME_DAMAGE);
			entities.attribute_add(e,Entity::ATTRIBUTE_INTEGRATE_POSITION);
			entity_add(e);
		}
	}

	//"events"
	void entity_damage(Entity* e,float dmg) {
		if(dmg<=0.0f) {
			return;
		}
		if(!e->comp_health) {
			return;
		}
		if(entities.component_get(e,Component::TYPE_SHIELD)) {
			return;
		}

		CompHealth* health=e->comp_health;

		health->health-=dmg;

		/*
		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_get(e,Component::TYPE_SHOW_DAMAGE);
		if(c_show_damage) {
			c_show_damage->timer.reset();
			entities.event_add(c_show_damage,Component::EVENT_FRAME);

			if(c_show_damage->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
				use_shader_damage=true;
			}
		}
		*/
		//XXX make fast
		for(Component* comp : e->components) {
			if(comp->type!=Component::TYPE_SHOW_DAMAGE) {
				continue;
			}
			CompShowDamage* damage=(CompShowDamage*)comp;
			if(damage->damage_type==CompShowDamage::DAMAGE_TYPE_ANIMATION_PROGRESS) {
				if(damage->animation_progress_node) {
					damage->animation_progress_node->set_animation_progress(1.0f-e->comp_health->health/e->comp_health->health_max);
				}
			}
			else if(damage->damage_type==CompShowDamage::DAMAGE_TYPE_BLINK) {
				damage->timer.reset();
				entities.event_add(damage,Component::EVENT_FRAME);
			}
			else if(damage->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
				damage->timer.reset();
				entities.event_add(damage,Component::EVENT_FRAME);
				use_shader_damage=true;
			}
		}


		if(health->health/health->health_max<0.3 && entities.component_get(e,Component::TYPE_FLAME_DAMAGE)==nullptr) {
			entities.component_add(e,Component::TYPE_FLAME_DAMAGE);
		}

		if(health->health<=0 && health->alive) {
			health->alive=false;

			if(entities.attribute_has(e,Entity::ATTRIBUTE_REMOVE_ON_DEATH)) {
				if(e->tmp_damage>90.0) {
					add_big_explosion(e->pos,e->player_side);
				}
				else {

					float shape_area=0;
					if(e->comp_shape) {
						for(const Quad& quad : e->comp_shape->quads) {
							sf::Vector2f size=quad.size();
							shape_area+=size.x*size.y;
						}
					}

					//printf("area %f\n",shape_area);

					add_decal_explosion(e->pos,shape_area*0.0001f);
					//add_decal(graphic_explosion,e->pos);
				}
				entity_remove(e);
			}

			//XXX make fast (events?)
			for(Component* comp : e->components) {
				if(comp->type!=Component::TYPE_AI2) {
					continue;
				}
				CompAI2* ai=(CompAI2*)comp;
				for(const std::pair<std::string,int>& spawn : ai->death_spawn_ids) {
					for(int count=0;count<spawn.second;count++) {
						Entity* s=create_by_id(spawn.first);
						if(s) {
							s->pos=e->pos+Utils::rand_vec(-20,20);
							entity_add(s);
						}
					}
				}
			}

			for(Component* comp : e->components) {
				if(comp->type!=Component::TYPE_SPAWN_PIECES) {
					continue;
				}
				CompSpawnPieces* pieces=(CompSpawnPieces*)comp;
				for(const CompSpawnPieces::Piece& piece : pieces->pieces) {
					float scale=2.0f;

					sf::Vector2f e_size=e->comp_shape->quads[0].size();

					Entity* pe=entities.entity_create();
					Texture t=pieces->texture;

					t.rect=sf::IntRect(piece.map_pos.x,piece.map_pos.y,piece.map_size.x,piece.map_size.y);
					entities.component_add(pe,Component::TYPE_DISPLAY);
					pe->comp_display->add_texture_center(t,scale);

					entities.component_add(pe,Component::TYPE_SHAPE);
					pe->comp_shape->add_quad_center(t.get_size()*scale);

					pe->comp_shape->collision_group=CompShape::COLLISION_GROUP_ENEMY;
					pe->comp_shape->collision_mask=CompShape::COLLISION_GROUP_TERRAIN |
							CompShape::COLLISION_GROUP_PLAYER_BULLET;
					pe->comp_shape->bounce=true;
					pe->comp_shape->terrain_bounce=true;

					entity_add_health(pe,5);
					entities.attribute_add(pe,Entity::ATTRIBUTE_REMOVE_ON_DEATH);

					sf::Vector2f rel_pos=piece.pos*scale-e_size*0.5f;
					pe->pos=e->pos+rel_pos;

					pe->vel=Utils::vec_normalize(rel_pos)*100.0f;
					entities.attribute_add(pe,Entity::ATTRIBUTE_INTEGRATE_POSITION);
					//entity_add_timeout(pe,CompTimeout::ACTION_REMOVE_ENTITY,Utils::rand_range(3.0,4.0));
					entity_add(pe);
				}
			}

		}
	}
	CompShowOnMinimap* entity_show_on_minimap(Entity* e,Color color) {
		CompShowOnMinimap* c=(CompShowOnMinimap*)entities.component_add(e,Component::TYPE_SHOW_ON_MINIMAP);
		c->node.type=Node::TYPE_SOLID;
		c->node.color=color;
		c->node.scale=sf::Vector2f(4,4);
		c->node.origin=sf::Vector2f(2,2);
		return c;
	}

	void launch_missiles(int count,sf::Vector2f pos,bool player_side) {

		const std::vector<Entity*>& targets=entities.attribute_list_entities(
				player_side ? Entity::ATTRIBUTE_ENEMY : Entity::ATTRIBUTE_FRIENDLY);

		std::vector<Entity*> targeted;

		for(Entity * e : targets) {
			if(Utils::vec_length_fast(pos-e->pos)<300*300) {
				targeted.push_back(e);
			}
		}

		for(int i=0;i<count;i++) {
			Entity* target=NULL;
			if(targeted.size()>0) {
				target=Utils::vector_rand(targeted);
			}

			Entity* e=create_missile(Utils::vector_rand(graphic_missile),target,player_side);
			e->angle=(float)i/(float)count*360.0f;
			e->pos=pos;
			entity_add(e);
		}
	}

	sf::Vector2f screen_to_game_pos(const sf::Vector2f& p) {
		sf::Vector2f r;
		r.x=(p.x-node_game.pos.x)/node_game.scale.x;
		r.y=(p.y-node_game.pos.y)/node_game.scale.y;
		return r;
	}
	/*
	void wave_manager_frame(float dt) {
		if(selected_level==0) {
			return;
		}

		if(!swarms.empty()) {
			return;
		}

		wave_manager.timeout-=dt;
		if(wave_manager.timeout>0) {
			return;
		}

		wave_manager.timeout=2.0;

		add_swarm(Utils::rand_range_i(5,12),Utils::rand_range_i(0,1));


		for(int i=0;i<8;i++) {
			Entity* enemy;

			if(i%5==0) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_up),0);
			else if(i%5==1) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_down),1);
			else if(i%5==2) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_side),2);
			else enemy=create_enemy_suicide(Utils::vector_rand(graphic_enemy_suicide));

			enemy->pos=sf::Vector2f(Utils::rand_vec(-4000,4000));
			entity_add(enemy);
		}
	}
	*/

	void event_frame(float dt) override {
		if(!node.visible || !player) return;

		//operating in seconds!
		dt*=0.001f;
		dt*=time_scale;

		sf::Vector2f game_size=sf::Vector2f(size.x/node_game.scale.x,size.y/node_game.scale.y);


		//systems

		//input
		for(Entity* e : entities.attribute_list_entities(Entity::ATTRIBUTE_PLAYER_CONTROL)) {
			/*
			e->vel=(pointer_pos-size*0.5f)*3.0f;
			e->vel=Utils::vec_cap_length(e->vel,0,200);
			*/
			e->fire_gun[0]=pressed;
			e->fire_gun[1]=pressed_right;
			e->fire_gun[2]=sf::Keyboard::isKeyPressed(sf::Keyboard::R);



			if(player_ship==3 && sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
				CompElectricity* el=(CompElectricity*)entities.component_get(player,Component::TYPE_ELECTRICITY);
				if(!el){
					el=(CompElectricity*)entities.component_add(player,Component::TYPE_ELECTRICITY);
				}
				el->player_side=player->player_side;
				el->potential=0.9;
				el->is_source=true;
				el->decay_speed=1.8f;
			}

			/*
			for(int i=0;i<8;i++) {
				if(e->fire_gun[i]) {
					for(int i2=i+1;i2<8;i2++) {
						e->fire_gun[i2]=false;
					}
					break;
				}
			}
			*/

			float max_vel=200;
			sf::Vector2f vel;
			if(controls.move_up) vel.y-=1.0;
			if(controls.move_down) vel.y+=1.0;
			if(controls.move_left) vel.x-=1.0;
			if(controls.move_right) vel.x+=1.0;
			Utils::vec_normalize(vel);
			vel*=max_vel;

			e->vel=vel;

			float angle=270;

			if(player_ship!=0) {
				angle=Utils::rad_to_deg(Utils::vec_angle(size*0.5f-pointer_pos));
			}

			e->angle=angle;

			if(e->comp_health) {
				player_health.set_progress(e->comp_health->health/e->comp_health->health_max);
			}

		}

		for(Component* ccomp : entities.component_list(Component::TYPE_DISPLAY)) {
			CompDisplay* display=(CompDisplay*)ccomp;

			for(std::size_t i=0;i<display->node_timeouts.size();) {
				std::pair<float,Node*>& timeout=display->node_timeouts[i];
				timeout.first-=dt;
				if(timeout.first<=0) {
					display->root_node.remove_child(timeout.second);
					display->node_timeouts.erase(display->node_timeouts.begin()+i);
				}
				else {
					i++;
				}
			}
			for(GraphicNode::Ptr& d : display->nodes) {
				d->update(dt);
			}
		}

		const std::vector<Component*>& list_shape=entities.component_list(Component::TYPE_SHAPE);
		for(std::size_t shape_i1=0;shape_i1<list_shape.size();shape_i1++) {
			CompShape* shape=(CompShape*)list_shape[shape_i1];
			Entity* e=shape->entity;

			if(!shape->enabled) {
				continue;
			}
			for(const Quad& q : shape->quads) {
				Quad q2=q;
				q2.translate(e->pos);

				//terrain collision
				if(shape->collision_mask&CompShape::COLLISION_GROUP_TERRAIN) {
					sf::Vector2f q_size=q2.p2-q2.p1;

					sf::FloatRect r(q2.p1,q_size);
					sf::Vector2f col_normal;
					if(terrain.check_collision(r,col_normal)) {
						terrain.damage_area(r,20);

						entity_damage(e,10.0f*shape->take_damage_terrain_mult);

						if(shape->terrain_bounce) {
							CompBounce* c_bounce=(CompBounce*)entities.component_add(e,Component::TYPE_BOUNCE);
							c_bounce->timer.reset(0.3);
							c_bounce->vel=Utils::vec_reflect(e->vel,col_normal);
						}
					}
				}

				//brute-force collisions
				for(std::size_t shape_i2=shape_i1+1;shape_i2<list_shape.size();shape_i2++) {
					CompShape* shape2=(CompShape*)list_shape[shape_i2];
					Entity* ce=shape2->entity;

					if(ce==e || !shape->enabled) {
						continue;
					}

					if( (shape->collision_group&shape2->collision_mask)==0 ||
							(shape2->collision_group&shape->collision_mask)==0) {
						continue;
					}

					for(const Quad& cq : shape2->quads) {

						Quad cq2=cq;
						cq2.translate(ce->pos);

						if(q2.intersects(cq2)) {
							entity_damage(e,shape2->hit_damage);
							entity_damage(ce,shape->hit_damage);

							if(shape->bounce && shape2->bounce) {
								sf::Vector2f b_vel=Utils::vec_normalize(e->pos-ce->pos)*100.0f;

								//sf::Vector2f tangent=Utils::vec_normalize(e->pos-ce->pos);
								//sf::Vector2f normal(tangent.y,-tangent.x);

								CompBounce* c_bounce1=(CompBounce*)entities.component_add(e,Component::TYPE_BOUNCE);
								c_bounce1->timer.reset(0.3);
								c_bounce1->vel=b_vel;
								//c_bounce1->vel=Utils::vec_reflect(e->vel,normal)*100.0f;

								CompBounce* c_bounce2=(CompBounce*)entities.component_add(ce,Component::TYPE_BOUNCE);
								c_bounce2->timer.reset(0.3);
								c_bounce2->vel=-b_vel;
							}

							if(e==player && shape2->hit_splatter.tex) {
								add_splatter(shape2->hit_splatter);
							}
							else if(ce==player && shape->hit_splatter.tex) {
								add_splatter(shape->hit_splatter);
							}

						}
					}
				}
			}
		}

		for(Component* ccomp : entities.component_list(Component::TYPE_ENGINE)) {
			CompEngine* comp=(CompEngine*)ccomp;
			if(Utils::vec_length_fast(comp->entity->vel)>50*50) {
				comp->node.visible=true;
				comp->node.update(dt);
			}
			else {
				comp->node.visible=false;
			}
		}

		std::vector<std::pair<sf::Vector2f,bool> > timeout_explosions;
		for(Component* ccomp : entities.component_list(Component::TYPE_TIMEOUT)) {
			CompTimeout* comp=(CompTimeout*)ccomp;

			comp->timeout-=dt;
			if(comp->timeout<=0) {
				if(comp->action==CompTimeout::ACTION_REMOVE_ENTITY) {
					entity_remove(comp->entity);
				}
				else if(comp->action==CompTimeout::ACTION_BIG_EXPLOSION) {
					timeout_explosions.push_back(std::make_pair(comp->entity->pos,comp->entity->player_side));
					entity_remove(comp->entity);
				}
				entities.component_remove(comp);
			}
		}
		//XXX: temp. adding components (to other entities) while traversing component_list crashes
		for(std::size_t i=0;i<timeout_explosions.size();i++) {
			add_big_explosion(timeout_explosions[i].first,timeout_explosions[i].second);
		}


		const std::vector<Entity*> attractors=entities.attribute_list_entities(Entity::ATTRIBUTE_ATTRACT);
		for(Component* ccomp : entities.component_list(Component::TYPE_AI)) {
			CompAI* comp=(CompAI*)ccomp;

			if(comp->stun_timeout>0) {
				comp->stun_timeout-=dt;
				comp->entity->fire_gun[0]=false;
				//comp->entity->vel=sf::Vector2f(0,0);
				continue;
			}

			float min_dist=0;
			Entity* closest_attractor=nullptr;

			for(std::size_t i=0;i<attractors.size();i++) {
				Entity* attractor=attractors[i];

				if(attractor->player_side!=comp->entity->player_side) {
					 float dist=Utils::vec_length_fast(attractor->pos-comp->entity->pos);
					 if(!closest_attractor || dist<min_dist) {
						 min_dist=dist;
						 closest_attractor=attractor;
					 }
				}
			}

			if(closest_attractor) {

				float speed=150.0f;
				float acc=200.0f;
				sf::Vector2f diff=closest_attractor->pos-comp->entity->pos;
				float dist=Utils::dist(diff);

				if(dist<comp->engage_distance) {
					diff+=comp->rand_offset*dist*0.3f;
					sf::Vector2f vel=diff/dist*speed;

					comp->entity->vel.x=Utils::num_move_towards(comp->entity->vel.x,vel.x,dt*acc);
					comp->entity->vel.y=Utils::num_move_towards(comp->entity->vel.y,vel.y,dt*acc);

					continue;
				}
			}


			if(comp->ai_type==CompAI::AI_FOLLOWER) {
				Entity* e=comp->entity;
				float player_dist=Utils::dist(player->pos-e->pos);
				float wanted_dist=35;

				if(player_dist>wanted_dist) {
					e->pos=player->pos+Utils::vec_normalize(e->pos-player->pos)*wanted_dist;
				}

				const std::vector<Entity*>& targets=entities.attribute_list_entities(Entity::ATTRIBUTE_ENEMY);
				std::vector<Entity*> targeted;
				for(Entity * t : targets) {
					if(Utils::vec_length_fast(e->pos-t->pos)<300*300) {
						targeted.push_back(t);
					}
				}

				Entity* target=nullptr;
				if(targeted.size()>0) {
					target=targeted[0];
					float min_dist=Utils::vec_length_fast(targeted[0]->pos-e->pos);
					for(std::size_t i=1;i<targeted.size();i++) {
						float dist=Utils::vec_length_fast(targeted[i]->pos-e->pos);
						if(dist<min_dist) {
							min_dist=dist;
							target=targeted[i];
						}
					}
				}

				if(target) {
					e->fire_gun[0]=true;
					//const std::vector<Component*> guns=entities.component_list(Component::TYPE_GUN);
					for(Component* gun_comp : e->components) {
						if(gun_comp->type!=Component::TYPE_GUN) continue;
						CompGun* gun=(CompGun*)gun_comp;
						gun->angle=Utils::rad_to_deg(Utils::vec_angle(target->pos-e->pos))-90;
					}
				}
				else {
					e->fire_gun[0]=false;
				}
			}
			else if(comp->ai_type==CompAI::AI_MELEE || comp->ai_type==CompAI::AI_SHOOTER) {
				Entity* target=player;

				sf::Vector2f aim_pos=target->pos+target->vel*0.5f;

				//movement
				float speed=150.0f;
				float acc=200.0f;
				sf::Vector2f diff=aim_pos+comp->target_offset-comp->entity->pos;
				float dist=Utils::dist(diff);

				if(comp->engage_distance>0.0f && dist>comp->engage_distance) {
					comp->entity->vel.x=0;
					comp->entity->vel.y=0;
					comp->entity->fire_gun[0]=false;
					continue;
				}

				diff+=comp->rand_offset*dist*0.3f;
				sf::Vector2f vel=diff/dist*speed;

				comp->entity->vel.x=Utils::num_move_towards(comp->entity->vel.x,vel.x,dt*acc);
				comp->entity->vel.y=Utils::num_move_towards(comp->entity->vel.y,vel.y,dt*acc);

				//other
				if(comp->ai_type==CompAI::AI_MELEE) {

				}
				else if(comp->ai_type==CompAI::AI_SHOOTER) {
					sf::Vector2f diff=aim_pos+comp->target_offset-comp->entity->pos;
					comp->entity->fire_gun[0]=(std::abs(diff.x)<70 && std::abs(diff.y)<70);
				}
			}
			else if(comp->ai_type==CompAI::AI_MISSILE) {
				float angle_vel=M_PI*1.0;
				float vel=250;

				float angle=Utils::deg_to_rad(comp->entity->angle);
				if(comp->target && comp->target->entity) {
					float angle_delta=Utils::vec_angle(comp->entity->pos-comp->target->entity->pos);
					angle=Utils::angle_normalize(Utils::angle_move_towards(angle,angle_delta,dt*angle_vel));
				}
				comp->entity->angle=Utils::rad_to_deg(angle);
				comp->entity->vel=Utils::vec_for_angle(angle,vel);
			}
			else if(comp->ai_type==CompAI::AI_AVOIDER) {
				Entity* target=player;

				if(comp->engage_distance>0.0f &&
						Utils::vec_length_fast(target->pos-comp->entity->pos)>comp->engage_distance*comp->engage_distance) {
					continue;
				}
				float wanted_dist=300;

				//float dist=Utils::dist(diff);
				sf::Vector2f wanted_pos=target->pos+Utils::vec_normalize(comp->entity->pos-target->pos)*wanted_dist;
				sf::Vector2f diff=wanted_pos-comp->entity->pos;

				float speed=150.0f;
				float acc=200.0f;

				sf::Vector2f vel=Utils::vec_normalize(diff)*speed;
				comp->entity->vel.x=Utils::num_move_towards(comp->entity->vel.x,vel.x,dt*acc);
				comp->entity->vel.y=Utils::num_move_towards(comp->entity->vel.y,vel.y,dt*acc);
			}

		}


		for(Component* ccomp : entities.component_list(Component::TYPE_AI2)) {
			CompAI2* comp=(CompAI2*)ccomp;

			if(comp->stun_timeout>0) {
				comp->stun_timeout-=dt;
				comp->entity->fire_gun[0]=false;
				//comp->entity->vel=sf::Vector2f(0,0);
				continue;
			}

			float min_dist=0;
			Entity* closest_attractor=nullptr;

			for(std::size_t i=0;i<attractors.size();i++) {
				Entity* attractor=attractors[i];

				if(attractor->player_side!=comp->entity->player_side) {
					 float dist=Utils::vec_length_fast(attractor->pos-comp->entity->pos);
					 if(!closest_attractor || dist<min_dist) {
						 min_dist=dist;
						 closest_attractor=attractor;
					 }
				}
			}
			if(closest_attractor) {
				float speed=150.0f;
				float acc=200.0f;
				sf::Vector2f diff=closest_attractor->pos-comp->entity->pos;
				float dist=Utils::dist(diff);
				//diff+=dist*0.3f;
				sf::Vector2f vel=diff/dist*speed;
				comp->entity->vel.x=Utils::num_move_towards(comp->entity->vel.x,vel.x,dt*acc);
				comp->entity->vel.y=Utils::num_move_towards(comp->entity->vel.y,vel.y,dt*acc);
				continue;
			}

			Entity* target=player;

			float speed=150.0f;
			float acc=200.0f;

			bool is_idle=Utils::vec_length_fast(target->pos-comp->entity->pos)>comp->idle_distance*comp->idle_distance;

			if(is_idle) {
				comp->entity->vel.x=Utils::num_move_towards(comp->entity->vel.x,0,dt*acc);
				comp->entity->vel.y=Utils::num_move_towards(comp->entity->vel.y,0,dt*acc);
				comp->entity->fire_gun[0]=false;
				continue;
			}

			for(const CompAI2::AIBehavior& behavior : comp->behaviors) {
				if(behavior==CompAI2::AI_BEHAVIOR_SAFE_FOLLOW) {
					if(is_idle) {
						continue;
					}
					sf::Vector2f wanted_pos=target->pos+Utils::vec_normalize(comp->entity->pos-target->pos)*comp->safe_follow_distance;
					wanted_pos+=comp->rand_offset*20.0f;

					sf::Vector2f diff=wanted_pos-comp->entity->pos;
					sf::Vector2f vel=Utils::vec_normalize(diff)*speed;
					comp->entity->vel.x=Utils::num_move_towards(comp->entity->vel.x,vel.x,dt*acc);
					comp->entity->vel.y=Utils::num_move_towards(comp->entity->vel.y,vel.y,dt*acc);
				}
				else if(behavior==CompAI2::AI_BEHAVIOR_SPAWN_MINIONS) {
					if(comp->spawn_ids.empty()) {
						continue;
					}
					comp->spawn_timeout-=dt;

					if(comp->spawn_node) {
						float animation_duration=1.0;
						float anim=1.0f-std::min(1.0f,comp->spawn_timeout/animation_duration);
						comp->spawn_node->set_animation_progress(anim);
					}


					if(comp->spawn_timeout>0) {
						continue;
					}
					comp->spawn_timeout=comp->spawn_interval;

					Entity* e=create_by_id(Utils::vector_rand(comp->spawn_ids));
					if(e) {
						e->pos=comp->entity->pos+comp->spawn_pos;
						e->vel=comp->spawn_vel;
						e->angle=comp->spawn_angle;
						CompAI* e_ai=(CompAI*)entities.component_get(e,Component::TYPE_AI);
						if(e_ai) {
							e_ai->engage_distance=-1.0f;
						}
						entity_add(e);
					}
				}
				else if(behavior==CompAI2::AI_BEHAVIOR_AIM_SHOOT) {
					if(is_idle) {
						continue;
					}
					for(Component* gun_comp : comp->entity->components) {
						if(gun_comp->type!=Component::TYPE_GUN) continue;
						CompGun* gun=(CompGun*)gun_comp;

						//gun->angle=Utils::rad_to_deg(Utils::vec_angle(target->pos-comp->entity->pos))-90;

						sf::Vector2f bullet_vel=target_lead(target->pos-comp->entity->pos,target->vel,gun->bullet_speed);
						gun->angle=Utils::rad_to_deg(Utils::vec_angle(bullet_vel))-90;

					}
					comp->entity->fire_gun[0]=true;
				}
				else if(behavior==CompAI2::AI_BEHAVIOR_SHOOT) {

					if(is_idle) {
						comp->entity->fire_gun[0]=false;
						continue;
					}
					if(comp->shoot_timer.is_done()) {
						comp->shoot_idle_timer.update(dt);
						if(comp->shoot_idle_timer.is_done()) {
							comp->shoot_timer.reset();
							comp->shoot_idle_timer.reset();
						}
					}
					else {
						comp->shoot_timer.update(dt);
						comp->entity->fire_gun[0]=!comp->shoot_timer.is_done();
					}

				}
			}


		}

		/*
		for(std::size_t i=0;i<swarms.size();i++) {
			SwarmManager* swarm=swarms[i];

			swarm->anim+=dt;

			int alive_count=0;
			for(std::size_t ei=0;ei<swarm->entities.size();ei++) {
				Entity* e=swarm->entities[ei]->entity;
				if(!e) {
					continue;
				}
				alive_count++;

				float min_dist=0;
				Entity* closest_attractor=nullptr;

				for(std::size_t i=0;i<attractors.size();i++) {
					Entity* attractor=attractors[i];

					if(attractor->player_side!=e->player_side) {
						 float dist=Utils::vec_length_fast(attractor->pos-e->pos);
						 if(!closest_attractor || dist<min_dist) {
							 min_dist=dist;
							 closest_attractor=attractor;
						 }
					}
				}

				if(closest_attractor) {

					float speed=150.0f;
					float acc=200.0f;
					sf::Vector2f diff=closest_attractor->pos-e->pos;
					float dist=Utils::dist(diff);
					sf::Vector2f vel=diff/dist*speed;

					e->vel.x=Utils::num_move_towards(e->vel.x,vel.x,dt*acc);
					e->vel.y=Utils::num_move_towards(e->vel.y,vel.y,dt*acc);

					continue;
				}





				SwarmManagerCtrl ctrl=swarm->get_entity_pos(ei);
				ctrl.pos+=player->pos;

				if(false) {
					if(Utils::vec_length_fast(e->pos-player->pos)<100.0f*100.0f) {
						ctrl.pos=Utils::vec_normalize(e->pos-player->pos)*100.0f;
					}
				}

				//movement

				//e->vel=Utils::vec_normalize(ctrl.pos-e->pos)*150.0f;

				e->vel=(ctrl.pos-e->pos)*10.0f;

				e->vel=Utils::vec_cap_length(e->vel,0,400.0f);

				e->fire_gun[0]=ctrl.fire;
			}

			if(alive_count==0) {
				for(std::size_t ei=0;ei<swarm->entities.size();ei++) {
					entities.entity_ref_delete(swarm->entities[ei]);
				}
				delete(swarm);
				swarms.erase(swarms.begin()+i);
				i--;
			}

		}

		wave_manager_frame(dt);
		*/


		for(Component* ccomp : entities.component_list(Component::TYPE_GUN)) {
			CompGun* g=(CompGun*)ccomp;

			if(g->gun_type==CompGun::GUN_LASER) {
				//laser
				if(!g->entity->fire_gun[g->group]) {
					if(g->laser_entity && g->laser_entity->entity) {
						g->laser_entity->entity->comp_display->root_node.visible=false;
					}
					continue;
				}

				if(!g->laser_entity) {
					Entity* e=entities.entity_create();
					entities.component_add(e,Component::TYPE_DISPLAY);
					g->laser_node=e->comp_display->add_texture_center(g->texture);
					g->laser_node->pos.y=0.0f;
					g->laser_node->scale.y=100;
					g->laser_node->texture.tex->setRepeated(true);
					g->laser_node->shader.shader=Loader::get_shader("shader/laser.frag");

					g->laser_entity=entities.entity_ref_create(e);
					g->my_refs.push_back(g->laser_entity);
					entity_add(e);
				}
				if(!g->laser_entity->entity) {
					printf("error: no laser entity!\n");
					continue;
				}

				sf::Vector2f pos=g->entity->pos+entity_rotate_vector(g->entity,g->pos);
				float angle=g->entity->angle+g->angle;

				g->laser_entity->entity->pos=pos;
				g->laser_entity->entity->angle=angle+180;
				g->laser_entity->entity->comp_display->root_node.visible=true;

			}
			else if(g->gun_type==CompGun::GUN_HOOK) {
				float hook_speed=500;

				if(g->hook.fire_timeout>0.0f) {
					g->hook.fire_timeout-=dt;
				}

				if(g->hook.idle) {
					if(g->entity->fire_gun[g->group] && g->hook.fire_timeout<=0.0f) {

						if(g->hook.pull_mode && g->hook.grab_entity && g->hook.grab_entity->entity) {
							Entity* g_e=g->hook.grab_entity->entity;
							entities.entity_ref_delete(g->hook.grab_entity);
							g->hook.grab_entity=nullptr;

							g_e->vel=Utils::vec_for_angle_deg(g->entity->angle,300);
							g_e->comp_shape->collision_mask|=CompShape::COLLISION_GROUP_TERRAIN;
							g_e->comp_shape->collision_group|=CompShape::COLLISION_GROUP_GRAB;
							if(g_e->comp_health) {
								g_e->comp_health->health=1;
							}
							g->hook.fire_timeout=0.5f;
						}
						else {
							g->hook.idle=false;
							g->hook.extending=true;
							g->hook.grabbed=false;
							g->hook.world_pos=g->entity->pos+entity_rotate_vector(g->entity,g->pos);
							g->hook.angle=g->entity->angle+g->angle;
							g->hook.position=0;
						}
					}
				}
				if(!g->hook.idle) {

					if(!g->entity->fire_gun[g->group]) {
						if(g->hook.extending) {
							g->hook.position=Utils::vec_length(g->hook.world_pos-g->entity->pos);
						}
						g->hook.extending=false;
						//g->hook.grabbed=false;
					}

					if(g->hook.extending) {
						g->hook.world_pos+=Utils::vec_for_angle_deg(g->hook.angle,hook_speed*dt);
						float dist2=Utils::vec_length_fast(g->hook.world_pos-g->entity->pos);
						if(dist2>g->hook.max_length*g->hook.max_length) {
							g->hook.extending=false;
							g->hook.position=sqrtf(dist2);
						}
					}
					else {
						g->hook.position-=hook_speed*dt;
						if(g->hook.position<=0) {
							g->hook.idle=true;
							g->hook.position=0;
						}

					}
				}
			}
			else if(g->gun_type==CompGun::GUN_GUN) {
				//gun
				if(g->cur_fire_timeout>0) {
					g->cur_fire_timeout-=dt;
				}
				if(!g->entity->fire_gun[g->group] || g->cur_fire_timeout>0) {
					continue;
				}
				g->cur_fire_timeout=g->fire_timeout;

				for(int i=0;i<g->bullet_count;i++) {
					float angle=g->angle;
					if(g->bullet_count>1) {
						angle=g->angle-g->angle_spread*0.5+(float)i/((float)g->bullet_count-1)*g->angle_spread;
					}

					Entity* bullet=create_bullet(g->texture,g->entity->player_side, (g->entity->player_side ? 1.0 : 2.0) );
					bullet->pos=g->entity->pos+entity_rotate_vector(g->entity,g->pos);
					bullet->angle=g->entity->angle+angle;
					bullet->vel=Utils::vec_for_angle_deg(bullet->angle,g->bullet_speed);
					if(!g->splatter_textures.empty()) {
						bullet->comp_shape->hit_splatter=Utils::vector_rand(g->splatter_textures);
					}
					entity_add(bullet);
				}
			}
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_TELEPORTATION)) {
			CompTeleportation* tele=(CompTeleportation*)ccomp;

			tele->anim+=dt;
			if(tele->anim>=tele->anim_duration) {
				node_ships.remove_child(&tele->origin_node);
				entities.component_remove(tele);

				tele->entity->node_main.visible=true;	//scale=sf::Vector2f(1,1);
				if(tele->entity->comp_shape) {
					tele->entity->comp_shape->enabled=true;
				}
				continue;
			}

			float a=Easing::inOutCubic(tele->anim/tele->anim_duration);
			//float a1=1.0f-a;
			float s=1.0f-sin(a*M_PI);

			tele->origin_node.pos=tele->origin;

			sf::Vector2f c_pos=Utils::vec_lerp(tele->origin,tele->destination,a);

			tele->origin_node.pos=c_pos;
			tele->origin_node.rotation=Utils::rad_to_deg(Utils::vec_angle(tele->destination-tele->origin))-90;
			tele->origin_node.scale=sf::Vector2f(s,2.0f-s);
			float c=s*s*s;
			tele->origin_node.color_add.set(c,c,c,0);

			tele->entity->node_main.visible=false;//scale=sf::Vector2f(a,2.0f-a);
			tele->entity->pos=c_pos;

			if(tele->entity->comp_shape) {
				tele->entity->comp_shape->enabled=false;
			}
		}
		for(Component* ccomp : entities.event_list_components(Component::TYPE_SHOW_DAMAGE,Component::EVENT_FRAME)) {
			CompShowDamage* comp=(CompShowDamage*)ccomp;

			comp->timer.update(dt);
			if(comp->timer.is_done()) {
				entities.event_remove(comp,Component::EVENT_FRAME);

				if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_BLINK) {
					comp->entity->node_main.color_add.set(0,0,0,0);
				}
				else if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
					use_shader_damage=false;
				}

				continue;
			}

			float a=1.0f-comp->timer.get_percentage();

			if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_BLINK) {
				comp->entity->node_main.color_add.set(a,a,a,0);
			}
			else if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
				shader_damage.set_param("amount",a*0.01);
				cam_shake.start();
			}
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_BOUNCE)) {
			CompBounce* comp=(CompBounce*)ccomp;
			comp->timer.update(dt);
			if(comp->timer.is_done()) {
				comp->entity->bounce_vel=sf::Vector2f(0,0);
				entities.component_remove(comp);
				continue;
			}
			comp->entity->vel=comp->vel*(1.0f-comp->timer.get_percentage());
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_FLAME_DAMAGE)) {
			CompFlameDamage* comp=(CompFlameDamage*)ccomp;

			comp->timer.update(dt);
			if(comp->timer.is_done()) {
				comp->timer.reset(0.03);

				add_decal(Utils::vector_rand(graphic_explosion),comp->entity->pos+
						sf::Vector2f(Utils::rand_range(-1,1),Utils::rand_range(-1,1))*20.0f);
			}
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_SHOW_ON_MINIMAP)) {
			CompShowOnMinimap* comp=(CompShowOnMinimap*)ccomp;
			comp->node.pos=comp->entity->pos;
		}

		for(Component* ccomp : entities.component_list(Component::TYPE_GRAVITY_FORCE)) {
			CompGravityForce* comp=(CompGravityForce*)ccomp;
			if(!comp->enabled) {
				continue;
			}

			float r2=comp->radius*comp->radius;

			Graphic decal_graphic(Loader::get_texture("general assets/gravity_force.png"));
			for(int i=0;i<1;i++) {
				float angle=Utils::rand_range(0,360);
				sf::Vector2f dir=Utils::vec_for_angle_deg(angle,1);
				float dist=Utils::rand_range(0,comp->radius);
				Entity* e=add_decal(decal_graphic,comp->entity->pos+dir*dist);
				entities.attribute_add(e,Entity::ATTRIBUTE_INTEGRATE_POSITION);
				e->angle=angle;
				e->vel=-dir*Utils::lerp(comp->power_center,comp->power_edge,dist/comp->radius);
			}

			for(Entity* e : entities.attribute_list_entities(Entity::ATTRIBUTE_ENEMY)) {
				float dist=Utils::vec_length_fast(e->pos-comp->entity->pos);
				if(dist<r2) {
					e->vel=Utils::vec_normalize(comp->entity->pos-e->pos)*
							Utils::lerp(comp->power_center,comp->power_edge,dist/r2);
				}
			}
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_SHIELD)) {
			CompShield* comp=(CompShield*)ccomp;
			comp->anim+=dt;
			float speeds[]={1,-0.5,0.5};
			for(std::size_t i=0;i<comp->layers.size();i++) {
				comp->layers[i]->rotation=comp->anim*speeds[i]*300.0f;
			}
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_HAMMER)) {
			CompHammer* comp=(CompHammer*)ccomp;
			if(!comp->enabled) {
				continue;
			}

			float duration=0.5;

			comp->anim+=dt/duration;

			if(comp->anim>=1.0f) {
				comp->anim=0.0f;
				comp->enabled=false;
				comp->my_gravity_force->enabled=false;
			}

			comp->hammer_node->rotation=Utils::lerp(0,360,comp->anim);
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_SPLATTER)) {
			CompSplatter* splatter=(CompSplatter*)ccomp;
			splatter->anim+=splatter->speed*dt;
			splatter->entity->pos.y+=Easing::linear(splatter->anim)*100.0f*dt;
			splatter->splatter_node.pos=splatter->entity->pos;
			splatter->splatter_node.color.a=std::min(1.0f,(1.0f-splatter->anim)*10.0f);
		}

		for(Component* ccomp : entities.component_list(Component::TYPE_PLATYPUS_BOSS)) {
			CompPlatypusBoss* boss=(CompPlatypusBoss*)ccomp;

			int prev_anim_frame=boss->anim.anim_current_frame;
			boss->anim.update(dt);
			boss->timer.update(dt);

			boss->spawn_timer.update(dt);
			for(int i=0;i<boss->spawn_timer.event_list.size();i++) {
				int event=boss->spawn_timer.event_list[i];
				if(event==CompPlatypusBoss::SPAWN_EVENT_OPEN) {
					boss->entity->comp_display->nodes[0]->texture=boss->texture_state_open;
				}
				else if(event==CompPlatypusBoss::SPAWN_EVENT_CLOSE) {
					boss->entity->comp_display->nodes[0]->texture=boss->texture_state_closed;
				}
				else if(event==CompPlatypusBoss::SPAWN_EVENT_SPAWN1) {
					Entity* e=create_platypus_random();
					e->pos=boss->entity->pos+sf::Vector2f(20,35)*2.0f;
					e->vel=sf::Vector2f(0,200);
					entity_add(e);
				}
				else if(event==CompPlatypusBoss::SPAWN_EVENT_SPAWN2) {
					Entity* e=create_platypus_random();
					e->pos=boss->entity->pos+sf::Vector2f(-20,35)*2.0f;
					e->vel=sf::Vector2f(0,200);
					entity_add(e);
				}
			}

			if(boss->shooting) {
				static std::array<int,3> shoot_frames={6,8,10};

				for(int i=prev_anim_frame+1;i<=boss->anim.anim_current_frame;i++) {

					for(std::size_t c=0;c<shoot_frames.size();c++) {
						if(shoot_frames[c]==i) {

							Entity* target=player;

							Entity* b=create_bullet(boss->texture_bullet,false,2.0);
							b->pos=boss->entity->pos+sf::Vector2f(boss->cur_x,boss->floor_y);
							b->vel=target_lead(target->pos-b->pos,target->vel,400);
							entity_add(b);

							break;
						}
					}

				}

				//boss->anim.anim_current_frame

			}

			//if(boss->anim.)

			if(boss->timer.is_done()) {
				if(boss->shown) {
					if(boss->shots_remaining>0) {
						boss->shots_remaining--;
						boss->anim.set_graphic(boss->anim_belly_shot);
						boss->anim.pos.x=boss->cur_x-boss->offset_belly_shot;
						boss->anim.pos.y=boss->floor_y-boss->anim.graphic.get_texture().get_size().y;
						boss->timer.reset(boss->anim.graphic.animation.duration+1.0f);
						boss->shooting=true;
					}
					else {
						boss->anim.set_graphic(boss->anim_dissapear);
						boss->anim.pos.x=boss->cur_x-boss->offset_dissapear;
						boss->anim.pos.y=boss->floor_y-boss->anim.graphic.get_texture().get_size().y;
						boss->timer.reset(boss->anim.graphic.animation.duration+Utils::rand_range(1,3));
						boss->shown=false;
						boss->shooting=false;
					}
				}
				else {
					boss->cur_x=Utils::rand_range_i(-10,10)*2.0f;
					boss->anim.set_graphic(boss->anim_appear);
					boss->anim.pos.x=boss->cur_x-boss->offset_appear;
					boss->anim.pos.y=boss->floor_y-boss->anim.graphic.get_texture().get_size().y;
					boss->timer.reset(boss->anim.graphic.animation.duration+1.0f);
					boss->shots_remaining=1;
					boss->shown=true;
				}
			}
		}

		for(Component* ccomp : entities.component_list(Component::TYPE_FIGHTER_SHIP)) {
			CompFighterShip* fighter=(CompFighterShip*)ccomp;

			fighter->ctrl_slash=fighter->entity->fire_gun[0];

			sf::Vector2f blade1_pos(-24,-3);
			sf::Vector2f blade2_pos(24,-3);

			uint8_t hit_mask;
			if(fighter->entity->player_side) {
				hit_mask=CompShape::COLLISION_GROUP_PLAYER_BULLET;
			}
			else {
				hit_mask=CompShape::COLLISION_GROUP_ENEMY_BULLET;
			}

			if(!fighter->slash_timer.is_done()) {
				fighter->slash_timer.update(dt);

				float anim=fighter->slash_timer.get_percentage();
				sf::Vector2f offset(0,-sin(anim*M_PI)*30.0f);

				for(int event_i=0;event_i<fighter->slash_timer.event_list.size();event_i++) {

					sf::Vector2f slash_p1=fighter->entity->pos+entity_rotate_vector(fighter->entity,blade1_pos+sf::Vector2f(0,-44-30));
					sf::Vector2f slash_p2=fighter->entity->pos+entity_rotate_vector(fighter->entity,blade2_pos+sf::Vector2f(0,-44-30));

					for(Component* ccomp : entities.component_list(Component::TYPE_SHAPE)) {
						CompShape* shape=(CompShape*)ccomp;
						if(!shape->enabled || (shape->collision_mask&hit_mask)==0) {
							continue;
						}
						for(const Quad& q : shape->quads) {
							if(q.contains(slash_p1-shape->entity->pos) || q.contains(slash_p2-shape->entity->pos)) {
								entity_damage(shape->entity,50);
								shape->entity->vel=Utils::vec_normalize(shape->entity->pos-fighter->entity->pos)*200.0f;
								break;
							}
						}
					}

				}
				fighter->node_blade1->pos=blade1_pos+offset;
				fighter->node_blade2->pos=blade2_pos+offset;
				fighter->node_blade1->rotation=0;
				fighter->node_blade2->rotation=0;
			}
			else if(fighter->ctrl_rotating) {
				fighter->rotating_anim+=dt;

				Node* nodes[2]={fighter->node_blade1,fighter->node_blade2};
				for(int i=0;i<2;i++) {
					float angle=fighter->rotating_anim*360.0f*3.0f+(float)i*180.0f;
					nodes[i]->pos=Utils::vec_for_angle_deg(angle,30);
					nodes[i]->rotation=angle+45;
				}

				for(Component* ccomp : entities.component_list(Component::TYPE_SHAPE)) {
					CompShape* shape=(CompShape*)ccomp;
					if(!shape->enabled || (shape->collision_mask&hit_mask)==0) {
						continue;
					}
					for(const Quad& q : shape->quads) {
						if(q.intersects_circle(fighter->entity->pos-shape->entity->pos,100)) {
							entity_damage(shape->entity,2000*dt);
							shape->entity->vel=Utils::vec_normalize(shape->entity->pos-fighter->entity->pos)*200.0f;
							break;
						}
					}
				}
			}
			else {
				fighter->node_blade1->pos=blade1_pos;
				fighter->node_blade2->pos=blade2_pos;
				fighter->node_blade1->rotation=0;
				fighter->node_blade2->rotation=0;
			}

			if(fighter->ctrl_slash && fighter->slash_timer.is_done()) {
				fighter->slash_timer.reset();
			}
		}

		std::vector<std::tuple<Entity*,bool,float> > electricity_to_add;
		for(Component* ccomp : entities.component_list(Component::TYPE_ELECTRICITY)) {
			CompElectricity* el=(CompElectricity*)ccomp;

			float potential_threshold=0.5;

			float max_dist=230;
			uint32_t max_connections=2;
			float damage=100;

			el->potential-=el->decay_speed*dt;
			if(el->potential<=0) {
				entities.component_remove(el);
				printf("electricity done\n");
				continue;
			}

			if(el->node_flare) {
				float s=Utils::rand_range(0.8,1.2)*0.75f;
				el->node_flare->scale=sf::Vector2f(s,s);
				el->node_flare->origin=el->node_flare->texture.get_size()*0.5f*s;
				el->node_flare->rotation=Utils::rand_angle_deg();
			}

			for(std::size_t i=0;i<el->connections.size();) {
				CompElectricity::Connection& c=el->connections[i];
				c.timeout-=dt;

				float dist=0;
				sf::Vector2f diff;

				if(c.entity->entity) {
					diff=c.entity->entity->pos-el->entity->pos;
					dist=Utils::vec_length(diff);
				}

				if(c.timeout<=0 || !c.entity->entity || dist>max_dist) {
					entities.entity_ref_delete(c.entity);
					entity_remove(c.bolt_entity);
					el->connections.erase(el->connections.begin()+i);
					printf("remove connection\n");

				}
				else {
					c.bolt_entity->pos=el->entity->pos;
					c.bolt_node->rotation=Utils::rad_to_deg(Utils::vec_angle(diff))+90.0f;
					c.bolt_node->scale.y=dist/c.bolt_node->texture.get_size().y;
					c.bolt_node->scale.x=0.5f;

					entity_damage(c.entity->entity,damage*dt);

					i++;
				}
			}

			if(el->connections.size()<max_connections) {

				uint8_t hit_mask;
				if(el->player_side) {
					hit_mask=CompShape::COLLISION_GROUP_PLAYER_BULLET;
				}
				else {
					hit_mask=CompShape::COLLISION_GROUP_ENEMY_BULLET;
				}

				for(Component* ccomp : entities.component_list(Component::TYPE_SHAPE)) {
					CompShape* shape=(CompShape*)ccomp;
					if(!shape->enabled || (shape->collision_mask&hit_mask)==0) {
						continue;
					}
					sf::Vector2f diff=shape->entity->pos-el->entity->pos;
					if(std::fabs(diff.x)>max_dist ||
							std::fabs(diff.y)>max_dist) {
						continue;
					}
					if(Utils::vec_length_fast(diff)>max_dist*max_dist) {
						continue;
					}

					bool existing=false;
					for(std::size_t i=0;i<el->connections.size();i++) {
						if(el->connections[i].entity->entity==shape->entity) {
							existing=true;
							break;
						}
					}
					if(existing) {
						continue;
					}


					CompElectricity* el2=(CompElectricity*)entities.component_get(shape->entity,Component::TYPE_ELECTRICITY);
					if(el2) {
						if(el2->potential>el->potential-potential_threshold) {
							continue;
						}
						el2->potential=std::max(el2->potential,el->potential-1.0f);
					}
					else {
						//el2=(CompElectricity*)entities.component_add(shape->entity,Component::TYPE_ELECTRICITY);
						//el2->player_side=el->player_side;
						electricity_to_add.push_back(std::make_tuple(shape->entity,el->player_side,el->potential-1.0f));
					}
					//el2->potential=std::max(el2->potential,el->potential-1.0f);


					printf("add connection\n");
					CompElectricity::Connection c;
					c.timeout=0.5;
					c.bolt_entity=entities.entity_create();
					entities.component_add(c.bolt_entity,Component::TYPE_DISPLAY);
					c.bolt_node=c.bolt_entity->comp_display->add_graphic(Graphic(Animation(
							Loader::get_texture("general assets/bolt.png"),18,64,0.1)),sf::Vector2f(0,0));
					c.entity=entities.entity_ref_create(shape->entity);
					entity_add(c.bolt_entity);

					el->connections.push_back(std::move(c));

					if(el->connections.size()>=max_connections) {
						break;
					}

				}

			}
		}
		for(std::tuple<Entity*,bool,float>& a : electricity_to_add) {
			CompElectricity* el2=(CompElectricity*)entities.component_add(std::get<0>(a),Component::TYPE_ELECTRICITY);
			el2->player_side=std::get<1>(a);
			el2->potential=std::get<2>(a);
		}


		for(Entity* e : entities.attribute_list_entities(Entity::ATTRIBUTE_INTEGRATE_POSITION)) {
			e->pos+=e->vel*dt;
		}

		//update lasers and hooks
		for(Component* ccomp : entities.component_list(Component::TYPE_GUN)) {
			CompGun* g=(CompGun*)ccomp;

			if(g->gun_type==CompGun::GUN_LASER) {
				if(!g->laser_entity || !g->laser_entity->entity) {
					continue;
				}
				if(!g->laser_entity->entity->comp_display->root_node.visible) {
					continue;
				}

				float angle=g->entity->angle+g->angle;
				sf::Vector2f pos=g->entity->pos+entity_rotate_vector(g->entity,g->pos);

				g->laser_entity->entity->pos=pos;

				//damage detection

				float laser_range=1000.0f;

				sf::Vector2f laser_p1=pos;
				sf::Vector2f laser_p2=pos+Utils::vec_for_angle_deg(angle,laser_range);

				//terrain
				Terrain::RayQuery query=terrain.query_ray(laser_p1,laser_p2);

				//ship collision
				uint8_t laser_collision_group;
				uint8_t laser_collision_mask;

				if(g->entity->player_side) {
					laser_collision_group=CompShape::COLLISION_GROUP_PLAYER_BULLET;
					laser_collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
							CompShape::COLLISION_GROUP_ENEMY);
				}
				else {
					laser_collision_group=CompShape::COLLISION_GROUP_ENEMY_BULLET;
					laser_collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
							CompShape::COLLISION_GROUP_PLAYER);
				}

				Quad laser_bbox(laser_p1,laser_p2);
				laser_bbox.sort_points();

				Entity* hit_ship=nullptr;
				float laser_hit_pos=1.0f;

				if(query.hit) {
					laser_hit_pos=query.hit_position;
					//laser_p2=pos+Utils::vec_for_angle_deg(angle,laser_hit_pos);
				}

				//ships
				for(Component* ccomp : entities.component_list(Component::TYPE_SHAPE)) {
					CompShape* shape=(CompShape*)ccomp;
					if(!shape->enabled) {
						continue;
					}
					if( (laser_collision_group&shape->collision_mask)==0 ||
						(shape->collision_group&laser_collision_mask)==0) {
						continue;
					}

					bool skip=false;
					for(Quad q : shape->quads) {
						q.translate(shape->entity->pos);
						bool p1_inside=false;
						float hit_pos=1.0;

						if(Utils::line_quad_intersection(laser_p1,laser_p2,q,hit_pos,p1_inside)) {
							if(p1_inside) {
								laser_hit_pos=0.0f;
								hit_ship=shape->entity;
								skip=true;
								break;
							}
							if(hit_pos<laser_hit_pos) {
								laser_hit_pos=hit_pos;
								hit_ship=shape->entity;
							}
						}

					}
					if(skip) {
						break;
					}
				}

				float laser_length=laser_range*laser_hit_pos;

				if(hit_ship) {
					entity_damage(hit_ship,dt*300);
				}
				else {
					if(query.hit) {
						terrain.damage_ray(query,dt*100);
					}
				}

				g->laser_node->scale.y=laser_length/g->texture.get_size().y;

				g->laser_node->shader.set_param("time",game_time);
				g->laser_node->shader.set_param("y_scale",g->laser_node->scale.y);
			}
			else if(g->gun_type==CompGun::GUN_HOOK) {

				if(g->hook.idle) {
					g->hook.node_chain.visible=false;
					g->hook.node_root.pos=g->pos;
					g->hook.node_root.rotation=g->angle;

					if(g->hook.pull_mode && g->hook.grab_entity) {
						if(!g->hook.grab_entity->entity) {
							entities.entity_ref_delete(g->hook.grab_entity);
							g->hook.grab_entity=nullptr;
						}
						else {
							sf::Vector2f world_pos=g->entity->pos+entity_rotate_vector(
									g->entity,g->pos-sf::Vector2f(0,g->hook.node_hook.texture.get_size().y*0.5f));
							g->hook.grab_entity->entity->pos=world_pos;
							g->hook.grab_entity->entity->angle=g->entity->angle+g->hook.grab_angle;
						}
					}

				}
				else {
					g->hook.node_chain.visible=true;

					sf::Vector2f local_pos=entity_rotate_vector(g->entity,g->pos);


					if(g->hook.extending) {
						if(!g->hook.grabbed) {
							if(terrain.check_collision(sf::FloatRect(g->hook.world_pos,sf::Vector2f(0,0)))) {
								if(g->hook.pull_mode) {
									g->hook.grabbed=true;
									g->hook.extending=false;
									g->hook.position=Utils::vec_length(g->hook.world_pos-g->entity->pos);
									terrain.damage_area(sf::FloatRect(g->hook.world_pos,sf::Vector2f(0,0)),1000);
									Entity* e=create_bullet(Loader::get_texture("general assets/rock.png"),g->entity->player_side);

									entities.component_remove(entities.component_get(e,Component::TYPE_TIMEOUT));

									if(g->entity->player_side) {
										e->comp_shape->collision_group=CompShape::COLLISION_GROUP_PLAYER_BULLET;
										e->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_ENEMY);
									}
									else {
										e->comp_shape->collision_group=CompShape::COLLISION_GROUP_ENEMY_BULLET;
										e->comp_shape->collision_mask=(CompShape::COLLISION_GROUP_PLAYER);
									}

									e->pos=g->hook.world_pos;
									entity_add(e);

									if(g->hook.grab_entity) {
										entities.entity_ref_delete(g->hook.grab_entity);
									}
									g->hook.grab_entity=entities.entity_ref_create(e);
								}
								else {
									g->hook.grabbed=true;
									if(g->hook.grab_entity) {
										entities.entity_ref_delete(g->hook.grab_entity);
									}
									g->hook.grab_entity=nullptr;
									g->hook.extending=false;
									g->hook.position=Utils::vec_length(g->hook.world_pos-g->entity->pos);
								}
							}
							else {

								uint8_t collision_mask;
								if(g->entity->player_side) {
									collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
											CompShape::COLLISION_GROUP_ENEMY|
											CompShape::COLLISION_GROUP_GRAB);
								}
								else {
									collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
											CompShape::COLLISION_GROUP_PLAYER|
											CompShape::COLLISION_GROUP_GRAB);
								}

								for(Component * comp : entities.component_list(Component::TYPE_SHAPE)) {
									CompShape* shape=(CompShape*)comp;
									if(!shape->enabled) continue;
									if( /*(collision_group&shape->collision_mask)==0 ||*/
										(shape->collision_group&collision_mask)==0) {
										continue;
									}

									bool skip=false;
									for(const Quad& q : shape->quads) {
										if(q.contains(g->hook.world_pos-shape->entity->pos)) {

											if(entities.attribute_has(shape->entity,Entity::ATTRIBUTE_BOSS)) {
												continue;
											}

											g->hook.grabbed=true;
											g->hook.extending=false;
											if(g->hook.grab_entity) {
												entities.entity_ref_delete(g->hook.grab_entity);
											}
											g->hook.grab_entity=entities.entity_ref_create(shape->entity);
											g->hook.position=Utils::vec_length(shape->entity->pos-g->entity->pos);

											if(g->hook.pull_mode) {
												if(g->entity->player_side) {
													shape->collision_group=CompShape::COLLISION_GROUP_PLAYER_BULLET;
													shape->collision_mask=(CompShape::COLLISION_GROUP_ENEMY);
												}
												else {
													shape->collision_group=CompShape::COLLISION_GROUP_ENEMY_BULLET;
													shape->collision_mask=(CompShape::COLLISION_GROUP_PLAYER);
												}
											}
											if(shape->entity->comp_health) {
												shape->entity->comp_health->health=1;
											}
											shape->take_damage_terrain_mult=1.0f;

											Component* c_ai=entities.component_get(shape->entity,Component::TYPE_AI);
											if(c_ai) {
												entities.component_remove(c_ai);
											}
											g->hook.grab_angle=shape->entity->angle-g->entity->angle;


											skip=true;
											break;
										}
									}
									if(skip) {
										break;
									}

								}
							}
						}
					}
					else {
						if(g->hook.grabbed) {
							if(g->hook.pull_mode) {
								//pull to self
								if(g->hook.grab_entity) {
									Entity* e_pull=g->hook.grab_entity->entity;
									if(e_pull) {
										e_pull->pos=g->entity->pos+local_pos+
												Utils::vec_cap_length(e_pull->pos-g->entity->pos-local_pos,0,g->hook.position);
										g->hook.world_pos=e_pull->pos;
									}
									else {
										g->hook.grabbed=false;
									}
								}
							}
							else {
								//grapple towards terrain/ship
								if(g->hook.grab_entity) {
									if(g->hook.grab_entity->entity) {
										g->hook.world_pos=g->hook.grab_entity->entity->pos;
									}
									else {
										g->hook.grabbed=false;
									}
								}
								g->entity->pos=g->hook.world_pos+
										Utils::vec_cap_length(g->entity->pos-g->hook.world_pos,0,g->hook.position);
							}
						}
						else {
							//retract hook
							g->hook.world_pos=g->entity->pos+local_pos+
									Utils::vec_cap_length(g->hook.world_pos-g->entity->pos-local_pos,0,g->hook.position);
						}
					}
					sf::Vector2f diff=g->hook.world_pos-(g->entity->pos+local_pos);
					float len=Utils::vec_length(diff);

					g->hook.node_root.pos=entity_unrotate_vector(g->entity,diff+local_pos);
					g->hook.node_root.rotation=Utils::rad_to_deg(Utils::vec_angle(diff))-g->entity->angle+180;
					g->hook.node_chain.scale=sf::Vector2f(1,len/g->hook.node_chain.texture.get_size().y);

					g->hook.node_chain.shader.set_param("y_scale",g->hook.node_chain.scale.y);

				}

			}

		}

		use_shader_blast=false;
		for(Component* comp : entities.component_list(Component::TYPE_STUN_BLAST)) {
			CompStunBlast* blast=(CompStunBlast*)comp;
			use_shader_blast=true;

			blast->anim+=dt;

			shader_blast.set_param("size",Utils::clamp(0,1,blast->anim/blast->duration)*blast->range);

			if(blast->anim>blast->duration) {
				entities.component_remove(blast);
			}

			float blast_range=blast->range*blast->anim/blast->duration;

			for(Component* comp_shape : entities.component_list(Component::TYPE_SHAPE)) {
				CompShape* shape=(CompShape*)comp_shape;
				for(const Quad& quad : shape->quads) {
					if(!quad.intersects_circle(blast->entity->pos-shape->entity->pos,blast_range)) {
						continue;
					}
					CompAI* ai=(CompAI*)entities.component_get(shape->entity,Component::TYPE_AI);
					if(ai) {
						if(ai->stun_timeout<1.0) {
							ai->stun_timeout=3.0;
							if(shape->entity->comp_display) {
								Node* n=shape->entity->comp_display->add_texture_center(Loader::get_texture("general assets/stunned.png"));
								n->pos.y-=40;
								shape->entity->comp_display->add_node_timeout(n,3.0);
							}
						}
					}
					CompAI2* ai2=(CompAI2*)entities.component_get(shape->entity,Component::TYPE_AI2);
					if(ai2) {
						if(ai2->stun_timeout<1.0) {
							ai2->stun_timeout=3.0;
							if(shape->entity->comp_display) {
								Node* n=shape->entity->comp_display->add_texture_center(Loader::get_texture("general assets/stunned.png"));
								n->pos.y-=40;
								shape->entity->comp_display->add_node_timeout(n,3.0);
							}
						}
					}

					shape->entity->vel=Utils::vec_normalize(shape->entity->pos-blast->entity->pos)*100.0f;
					break;
				}
			}

		}

		entities.update();

		for(Component* comp : entities.component_list(Component::TYPE_DISPLAY)) {
			Entity* e=comp->entity;
			e->node_main.pos=e->pos;
			e->node_main.rotation=e->angle+90;
		}


		//camera
		cam_shake.frame(dt);

		sf::Vector2f camera_pos=player->pos+cam_shake.offset; //-player->vel*0.1f;

		//camera_pos.x=std::floor(camera_pos.x);
		//camera_pos.y=std::floor(camera_pos.y);

		sf::Vector2f camera_offset=camera_pos-game_size*0.5f;

		node_game.pos=-camera_offset*node_game.scale.x;

		sf::FloatRect camera_rect=sf::FloatRect(camera_pos.x-game_size.x/2.0f,camera_pos.y-game_size.y/2.0f,game_size.x,game_size.y);

		/*
		cam_border[0].pos=sf::Vector2f(camera_rect.left,camera_rect.top);
		cam_border[1].pos=sf::Vector2f(camera_rect.left+camera_rect.width,camera_rect.top);
		cam_border[2].pos=sf::Vector2f(camera_rect.left+camera_rect.width,camera_rect.top+camera_rect.height);
		cam_border[3].pos=sf::Vector2f(camera_rect.left,camera_rect.top+camera_rect.height);
		*/


		background.update(camera_rect);
		terrain.update_visual(camera_rect);
		minimap.update(camera_pos);

		//post process
		node.post_process_shaders.clear();
		if(use_shader_damage) {
			node.post_process_shaders.push_back(shader_damage);
			shader_damage.set_param("time",game_time*0.4);
		}
		if(use_shader_blast) {
			node.post_process_shaders.push_back(shader_blast);
		}

		for(int i=0;i<10;i++) {
			if(player->tmp_timeout[i]>=0.0) {
				player->tmp_timeout[i]-=dt;
			}
		}

		if(!level_won) {
			if(entities.attribute_list_entities(Entity::ATTRIBUTE_BOSS).size()==0) {
				printf("level won!\n");
				node_level_complete.visible=true;
				level_won=true;
			}
		}

		game_time+=dt;
	}
	bool event_input(const MenuEvent& event) override {

		if(event.event.type==sf::Event::KeyPressed || event.event.type==sf::Event::KeyReleased) {
			sf::Keyboard::Key c=event.event.key.code;
			bool pressed=(event.event.type==sf::Event::KeyPressed);

			/*
			if(c==sf::Keyboard::W) {
				player->fire_gun[2]=pressed;
			}
			*/

			//movement
			if(c==sf::Keyboard::A) {
				controls.move_left=pressed;
				if(pressed) controls.move_right=false;
			}
			else if(c==sf::Keyboard::D) {
				controls.move_right=pressed;
				if(pressed) controls.move_left=false;
			}
			else if(c==sf::Keyboard::W) {
				controls.move_up=pressed;
				if(pressed) controls.move_down=false;
			}
			else if(c==sf::Keyboard::S) {
				controls.move_down=pressed;
				if(pressed) controls.move_up=false;
			}

			else if(c==sf::Keyboard::E) {

				if(player_ship==0) {
					CompGravityForce* g=(CompGravityForce*)entities.component_get(player,Component::TYPE_GRAVITY_FORCE);
					if(g) {
						g->enabled=pressed;
					}
				}
				else if(player_ship==3) {
					CompFighterShip* f=(CompFighterShip*)entities.component_get(player,Component::TYPE_FIGHTER_SHIP);
					if(f) {
						f->ctrl_rotating=pressed;
					}
				}
			}
			else if(c==sf::Keyboard::Q) {
				if(player_ship==0) {
					if(pressed) {
						if(entities.component_get(player,Component::TYPE_SHIELD)==nullptr) {
							entity_add_shield(player);
						}
					}
					else {
						Component* shield=entities.component_get(player,Component::TYPE_SHIELD);
						if(shield) {
							entities.component_remove(shield);
						}
					}
				}
			}

			//other
			else if(c==sf::Keyboard::Space) {
				if(player_ship==0) {

				}
				else if(player_ship==1) {

				}
				else if(player_ship==2) {
					time_scale= (pressed ? 0.3 : 1.0);
				}
			}
		}


		if(event.event.type==sf::Event::KeyPressed) {
			sf::Keyboard::Key c=event.event.key.code;

			if(c==sf::Keyboard::Escape) {
				trigger_message(action_esc);
				return true;
			}
			else if(c==sf::Keyboard::Q) {	//missiles

				if(player_ship==1) {
					if(player->tmp_timeout[0]<=0.0) {
						player->tmp_timeout[0]=0.5;
						Entity* m=create_mine();
						m->pos=player->pos;
						entity_add(m);
					}
				}
				else if(player_ship==2) {
					if(player->tmp_timeout[0]<=0.0) {
						player->tmp_timeout[0]=0.5;
						launch_missiles(3,player->pos,player->player_side);
					}
				}
				else if(player_ship==3) {
					if(player->tmp_timeout[0]<=0.0) {
						player->tmp_timeout[0]=1.0;
						//launch_missiles(3,player->pos,player->player_side);

						CompStunBlast* blast=(CompStunBlast*)entities.component_add(player,Component::TYPE_STUN_BLAST);
						blast->duration=0.5;
						blast->range=600;

					}
				}
			}
			else if(c==sf::Keyboard::E) {	//teleport
				if(player_ship==1) {
					Entity* m=create_follower();
					m->pos=player->pos;
					entity_add(m);
				}
				else if(player_ship==2) {
					entity_teleport(player,screen_to_game_pos(pointer_pos));
				}
			}
			else if(c==sf::Keyboard::R) {
				if(player_ship==0) {
					Entity* m=create_candy_mine();
					m->pos=player->pos;
					entity_add(m);
				}
				else if(player_ship==1) {
					if(!entities.component_get(player,Component::TYPE_ELECTRICITY)) {
						CompElectricity* el=(CompElectricity*)entities.component_add(player,Component::TYPE_ELECTRICITY);
						el->player_side=player->player_side;
						el->potential=3.0;
						el->is_source=true;
						el->decay_speed=0.5;
						printf("electricity enable\n");
					}
				}
			}
			else if(c==sf::Keyboard::Space) {

				if(player_ship==0) {
					CompHammer* hammer=(CompHammer*)entities.component_get(player,Component::TYPE_HAMMER);
					if(hammer && !hammer->enabled) {
						hammer->anim=0;
						hammer->enabled=true;
						hammer->my_gravity_force->enabled=true;
					}
				}
				else if(player_ship==1) {
					if(player->tmp_timeout[0]<=0.0) {
						player->tmp_timeout[0]=0.5;
						Entity* m=create_mine();
						m->pos=player->pos;
						CompGravityForce* grav=(CompGravityForce*)entities.component_add(m,Component::TYPE_GRAVITY_FORCE);
						grav->enabled=true;
						grav->radius=400;
						grav->power_center=200;
						grav->power_edge=200;

						CompTimeout* t=(CompTimeout*)entities.component_get(m,Component::TYPE_TIMEOUT);
						t->timeout=5.0f;
						t->action=CompTimeout::ACTION_BIG_EXPLOSION;

						entity_add(m);
					}
				}
			}

			else if(c==sf::Keyboard::Z) {	//zoom mode
				zoom_mode=(zoom_mode+1)%2;

				if(zoom_mode==0) {
					node_game.scale=sf::Vector2f(1,1);
				}
				else {
					node_game.scale=sf::Vector2f(2,2);
				}
				background.scale=node_game.scale;
			}
		}

		return false;
	}
	void event_resize() override {
		node_game.scale.x=node_game.scale.y=(zoom_mode+1);
		background.scale=node_game.scale;

		minimap.pos.x=0;
		minimap.pos.y=size.y-minimap.size.y;

		node_level_complete.pos=sf::Vector2f(size.x*0.5f,size.y-80);
		player_health.node.pos=sf::Vector2f((size.x-player_health.size.x)*0.5f,size.y-40);
	}
};

#endif
