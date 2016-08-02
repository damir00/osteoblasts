
#ifndef _BGA_GAME_H_
#define _BGA_GAME_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>

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
	void set(const Texture& tex,int _frame_w,int _frame_h,float _delay) {
		texture=tex;
		frame_w=_frame_w;
		frame_h=_frame_h;
		delay=_delay;
		frames.clear();

		if(tex.tex==NULL) {
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
		Texture t=texture;
		if(frames.size()>0) {
			t.rect=frames[frame%frames.size()];
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
	}
	void set_animation(const Animation& anim) {
		animation=anim;
	}
	bool is_animated() const {
		return (texture.tex==NULL && animation.texture.tex!=NULL);
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
public:
	Graphic graphic;
	int anim_current_frame;
	float anim_time;

	GraphicNode() {
		anim_time=0;
		anim_current_frame=0;
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

	void update(float dt) {
		if(!graphic.is_animated()) {
			return;
		}
		if(graphic.animation.duration==0.0f) {
			return;
		}

		anim_time=fmod(anim_time+dt,graphic.animation.duration);
		int next_frame=(anim_time/graphic.animation.delay);
		if(anim_current_frame!=next_frame) {
			texture=graphic.get_texture(next_frame);
			anim_current_frame=next_frame;
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
		TYPE_GUN,
		TYPE_TIMEOUT,
		TYPE_AI,
		TYPE_ENGINE,
		TYPE_TELEPORTATION,
		TYPE_SHOW_DAMAGE
	};
	enum Event {
		EVENT_FRAME,
		EVENT_DAMAGED
	};

	Entity* entity;
	Type type;

	std::vector<EntityRef*> my_refs;	//will automatically get released when comp is removed
	std::unordered_set<Event,std::hash<short> > events;

	Component(Entity* e) {
		entity=e;
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
		COLLISION_GROUP_ENEMY=1<<4
	};

	uint8_t collision_group;
	uint8_t collision_mask;

	Quad bbox;
	bool enabled;

	CompShape(Entity* e) : Component(e) {
		collision_group=1;
		collision_mask=0xff;
		enabled=true;
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

	CompDisplay(Entity* e);

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

	void remove() override {
		root_node.clear_children();
	}

	void remove_node(GraphicNode* node) {
		root_node.remove_child(node);
		for(int i=0;nodes.size();i++) {
			if(nodes[i].get()==node) {
				nodes.erase(nodes.begin()+i);
				return;
			}
		}
	}
};
class CompGun : public Component {
public:
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

	CompGun(Entity* e) :  Component(e) {
		//fire=false;
		group=0;
		bullet_speed=700;
		angle=0;
		fire_timeout=0.1;
		cur_fire_timeout=0;

		angle_spread=0;
		bullet_count=1;
	}
};
class CompEngine : public Component {
public:
	GraphicNode node;
	CompEngine(Entity* e);
	void set(const Graphic& g,sf::Vector2f pos) {
		node.set_graphic(g);
		node.pos=pos-sf::Vector2f(g.get_texture().get_size().x*0.5f,0.0);
	}
	void remove() override;
};

class CompHealth : public Component {
public:
	bool alive;
	float health;

	CompHealth(Entity* e) : Component(e) {
		alive=true;
		health=10;
	}
};

class CompInventory : public Component {
public:
	std::vector<Entity*> items;

	CompInventory(Entity* e) : Component(e) {

	}
};
class CompTimeout : public Component {
public:
	enum Action {
		ACTION_REMOVE_ENTITY,
	};
	Action action;
	float timeout;
	CompTimeout(Entity* e) : Component(e) {
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
		AI_MISSILE
	};
	AIType ai_type;

	EntityRef* target;

	sf::Vector2f rand_offset;	//normalized [-1,1]
	sf::Vector2f target_offset;	//world-space

	CompAI(Entity* e) : Component(e) {
		ai_type=AI_SUICIDE;
		rand_offset=Utils::rand_vec(-1,1);
		target=NULL;
	}
};
class CompTeleportation : public Component {
public:
	sf::Vector2f origin;
	sf::Vector2f destination;
	Node origin_node;
	float anim;
	float anim_duration;

	CompTeleportation(Entity* e) : Component(e) {
		anim=0.0f;
		anim_duration=0.0f;
	}
};
class CompShowDamage : public Component {
public:
	enum DamageType {
		DAMAGE_TYPE_BLINK,
		DAMAGE_TYPE_SCREEN_EFFECT
	};
	DamageType damage_type;

	float time_max;
	float time;

	CompShowDamage(Entity* e) : Component(e) {
		damage_type=DAMAGE_TYPE_BLINK;
		time_max=0.5f;
		time=0.0f;
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
	};

	//XXX move all members to comps/attrs
	//display
	Node node_main;
	CompDisplay display;

	//dynamics
	sf::Vector2f pos;	//center
	sf::Vector2f vel;
	float angle;	//deg

	//collisions
	CompShape shape;

	//health
	CompHealth health;

	bool player_side;
	bool fire_gun[8];

	//Component* components_indexed[COMPONENT_INDEX_SIZE];
	//std::vector<Component*> components_unindexed;
	std::vector<Component*> components;
	std::vector<Attribute> attributes;

	std::vector<EntityRef*> refs;

	Entity() :
		display(this),
		shape(this),
		health(this) {
		/*
		for(int i=0;i<COMPONENT_INDEX_SIZE;i++) {
			components_indexed[i]=NULL;
		}
		*/
		angle=270;	//point up by default
		for(int i=0;i<8;i++) fire_gun[i]=false;
		player_side=false;
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

	Component* component_create(Component::Type type,Entity* e) {
		Component *c=NULL;
		if(type==Component::TYPE_DISPLAY) {
			c=new CompDisplay(e);
		}
		else if(type==Component::TYPE_SHAPE) {
			c=new CompShape(e);
		}
		else if(type==Component::TYPE_GUN) {
			c=new CompGun(e);
		}
		else if(type==Component::TYPE_TIMEOUT) {
			c=new CompTimeout(e);
		}
		else if(type==Component::TYPE_AI) {
			c=new CompAI(e);
		}
		else if(type==Component::TYPE_ENGINE) {
			c=new CompEngine(e);
		}
		else if(type==Component::TYPE_TELEPORTATION) {
			c=new CompTeleportation(e);
		}
		else if(type==Component::TYPE_SHOW_DAMAGE) {
			c=new CompShowDamage(e);
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
	SimpleList<std::pair<Component*,Component::Event> > events_to_add;
	SimpleList<std::pair<Component*,Component::Event> > events_to_remove;


public:
	std::vector<Entity*> entities;

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
		Component* comp=component_create(type,entity);
		if(comp==NULL) {
			return comp;
		}
		Utils::vector_fit_size(component_map,type+1);
		component_map[type].push_back(comp);
		entity->components.push_back(comp);
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
		return NULL;
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
				entities.push_back(entities_to_add[i]);
				for(Component* c : entities_to_add[i]->components) {
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

				delete(e);
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

				for(Component::Event event : c->events) {
					component_events[c->type][event].erase(c);
				}

				if(c->entity) {
					c->remove();
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
		}
	}
};

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

class Game : public Menu {
	SpaceBackground background;
	Node node_game;
	Node node_ships;

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

	float time_scale;
	Controls controls;

	float game_time;

public:

	int action_esc;

	int zoom_mode;
	//bool snap_sprites_to_pixels;

	Game() {
		action_esc=0;
		zoom_mode=1;
		time_scale=1.0;
		//snap_sprites_to_pixels=true;

		//init
		background.create_default();
		node.add_child(&background);
		node.add_child(&node_game);
		node_game.add_child(&terrain);
		node_game.add_child(&node_ships);


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
		use_shader_damage=false;

		//cleanup
		for(Entity* e : entities.entities) {
			entity_remove(e);
		}
		entities.update();

		//load
		player=create_player();
		entity_add(player);

		for(int i=0;i<100;i++) {
			Entity* enemy;

			if(i%5==0) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_up),0);
			else if(i%5==1) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_down),1);
			else if(i%5==2) enemy=create_enemy_shooter(Utils::vector_rand(graphic_enemy_shooter_side),2);
			else enemy=create_enemy_suicide(Utils::vector_rand(graphic_enemy_suicide));

			enemy->pos=sf::Vector2f(Utils::rand_vec(-4000,4000));
			entity_add(enemy);
		}

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
	//entity local to entity local
	sf::Vector2f entity_rotate_vector(const Entity* e,const sf::Vector2f p) {
		float theta = Utils::deg_to_rad(e->angle+90);	//XXX get rid of 90deg offset
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

	Entity* create_player() {
		Entity* player=entities.entity_create();
		Texture tex=Loader::get_texture("player ships/Assaulter/assaulter.png");
		player->display.add_texture_center(tex);
		player->shape.add_quad_center(tex.get_size());

		player->shape.collision_group=CompShape::COLLISION_GROUP_PLAYER;
		player->shape.collision_mask=(CompShape::COLLISION_GROUP_ENEMY|
				CompShape::COLLISION_GROUP_ENEMY_BULLET|
				CompShape::COLLISION_GROUP_TERRAIN);

		entities.attribute_add(player,Entity::ATTRIBUTE_PLAYER_CONTROL);
		entities.attribute_add(player,Entity::ATTRIBUTE_FRIENDLY);

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
		g->texture=Loader::get_texture("player ships/Assaulter/railgun.png");
		g->pos.x=0;
		g->pos.y=-19;
		g->bullet_speed=800;
		g->fire_timeout=0.02;
		g->angle=0;
		g->group=2;

		player->player_side=true;

		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(player,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT;
		entities.event_add(c_show_damage,Component::EVENT_DAMAGED);

		return player;
	}
	Entity* create_bullet(const Texture& tex,bool player_side) {
		float scale=1.0;

		if(!player_side) {
			scale=2.0;
		}

		Entity* bullet=entities.entity_create();
		bullet->display.add_texture_center(tex,scale);
		bullet->shape.add_quad_center(tex.get_size()*scale);

		if(player_side) {
			bullet->shape.collision_group=CompShape::COLLISION_GROUP_PLAYER_BULLET;
			bullet->shape.collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_ENEMY);
		}
		else {
			bullet->shape.collision_group=CompShape::COLLISION_GROUP_ENEMY_BULLET;
			bullet->shape.collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_PLAYER);
		}
		bullet->player_side=player_side;

		entities.attribute_add(bullet,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entity_add_timeout(bullet,CompTimeout::ACTION_REMOVE_ENTITY,2.0f);
		return bullet;
	}
	Entity* create_missile(const Graphic& g,Entity* target,bool player_side) {
		float scale=2.0;

		Entity* missile=entities.entity_create();
		missile->display.add_graphic_center(g,scale);
		missile->shape.add_quad_center(g.get_texture().get_size()*scale);

		if(player_side) {
			missile->shape.collision_group=CompShape::COLLISION_GROUP_PLAYER_BULLET;
			missile->shape.collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_ENEMY);
		}
		else {
			missile->shape.collision_group=CompShape::COLLISION_GROUP_ENEMY_BULLET;
			missile->shape.collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
					CompShape::COLLISION_GROUP_PLAYER);
		}
		missile->player_side=player_side;

		entities.attribute_add(missile,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entity_add_timeout(missile,CompTimeout::ACTION_REMOVE_ENTITY,10.0f);

		CompAI* ai=(CompAI*)entities.component_add(missile,Component::TYPE_AI);
		ai->ai_type=CompAI::AI_MISSILE;
		if(target) {
			ai->target=entities.entity_ref_create(target);
			ai->my_refs.push_back(ai->target);
		}

		CompEngine* eng=(CompEngine*)entities.component_add(missile,Component::TYPE_ENGINE);
		eng->set(graphic_engine,sf::Vector2f(0,g.get_texture().get_size().y*scale*0.5));

		return missile;
	}

	Entity* create_enemy(const Graphic& g) {
		float scale=2.0;

		Entity* k=entities.entity_create();
		k->display.add_graphic_center(g,scale);

		k->shape.add_quad_center(g.get_texture().get_size()*scale);
		entities.attribute_add(k,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entities.attribute_add(k,Entity::ATTRIBUTE_ENEMY);

		k->shape.collision_group=CompShape::COLLISION_GROUP_ENEMY;
		k->shape.collision_mask=(CompShape::COLLISION_GROUP_TERRAIN|
				CompShape::COLLISION_GROUP_PLAYER|
				CompShape::COLLISION_GROUP_PLAYER_BULLET);


		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(k,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_BLINK;
		entities.event_add(c_show_damage,Component::EVENT_DAMAGED);

		k->health.health=100;
		return k;
	}
	Entity* create_enemy_suicide(const Graphic& g) {
		Entity* k=create_enemy(g);
		CompAI* ai=(CompAI*)entities.component_add(k,Component::TYPE_AI);
		ai->ai_type=CompAI::AI_SUICIDE;
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

	void add_decal(const Graphic& grap,sf::Vector2f pos) {
		Entity* e=entities.entity_create();
		e->display.add_graphic_center(grap);

		if(grap.is_animated()) {
			entity_add_timeout(e,CompTimeout::ACTION_REMOVE_ENTITY,grap.animation.duration);
		}
		else {
			entity_add_timeout(e,CompTimeout::ACTION_REMOVE_ENTITY,1.0f);
		}
		e->pos=pos;
		entity_add(e);
	}
	void add_decal(const std::vector<Graphic>& grap,sf::Vector2f pos) {
		if(grap.size()==0) {
			return;
		}
		add_decal(Utils::vector_rand(grap),pos);
	}

	//"events"
	void entity_damage(Entity* e,float dmg) {
		e->health.health-=dmg;

		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_get(e,Component::TYPE_SHOW_DAMAGE);
		if(c_show_damage) {
			c_show_damage->time=0.0;
			entities.event_add(c_show_damage,Component::EVENT_FRAME);

			if(c_show_damage->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
				use_shader_damage=true;
			}
		}

		if(e->health.health<=0 && e->health.alive) {
			e->health.alive=false;

			if(entities.attribute_has(e,Entity::ATTRIBUTE_REMOVE_ON_DEATH)) {
				add_decal(graphic_explosion,e->pos);
				entity_remove(e);
			}
		}
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

	void event_frame(float dt) override {
		if(!node.visible || !player) return;

		//operating in seconds!
		dt*=0.001f;
		dt*=time_scale;

		sf::Vector2f game_size=sf::Vector2f(size.x/node_game.scale.x,size.y/node_game.scale.y);

		//input
		for(Entity* e : entities.attribute_list_entities(Entity::ATTRIBUTE_PLAYER_CONTROL)) {
			/*
			e->vel=(pointer_pos-size*0.5f)*3.0f;
			e->vel=Utils::vec_cap_length(e->vel,0,200);
			*/
			e->fire_gun[0]=pressed;
			e->fire_gun[1]=pressed_right;

			float max_vel=200;
			sf::Vector2f vel;
			if(controls.move_up) vel.y-=1.0;
			if(controls.move_down) vel.y+=1.0;
			if(controls.move_left) vel.x-=1.0;
			if(controls.move_right) vel.x+=1.0;
			Utils::vec_normalize(vel);
			vel*=max_vel;

			e->vel=vel;

			float angle=Utils::rad_to_deg(Utils::vec_angle(size*0.5f-pointer_pos));
			e->angle=angle;

		}

		//XXX sim
		for(Entity* e : entities.entities) {
			//display
			for(GraphicNode::Ptr& d : e->display.nodes) {
				d->update(dt);
			}

			//shape
			if(e->shape.enabled) {
				for(const Quad& q : e->shape.quads) {
					Quad q2=q;
					q2.translate(e->pos);

					if(e->shape.collision_mask&CompShape::COLLISION_GROUP_TERRAIN) {
						sf::Vector2f q_size=q2.p2-q2.p1;

						sf::FloatRect r(q2.p1,q_size);
						if(terrain.check_collision(r)) {
							terrain.damage_area(r);
							entity_damage(e,10);
						}
					}

					//brute-force collisions
					for(Entity* ce : entities.entities) {
						if(ce==e || !ce->shape.enabled) {
							continue;
						}

						if( (e->shape.collision_group&ce->shape.collision_mask)==0 ||
								(ce->shape.collision_group&e->shape.collision_mask)==0) {
							continue;
						}

						for(const Quad& cq : ce->shape.quads) {

							Quad cq2=cq;
							cq2.translate(ce->pos);

							if(q2.intersects(cq2)) {
								entity_damage(e,10);
								entity_damage(ce,10);
							}
						}
					}
				}
			}
		}

		//systems
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
		for(Component* ccomp : entities.component_list(Component::TYPE_TIMEOUT)) {
			CompTimeout* comp=(CompTimeout*)ccomp;
			comp->timeout-=dt;
			if(comp->timeout<=0) {
				if(comp->action==CompTimeout::ACTION_REMOVE_ENTITY) {
					entity_remove(comp->entity);
				}
				entities.component_remove(comp);
			}
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_AI)) {
			CompAI* comp=(CompAI*)ccomp;

			if(comp->ai_type==CompAI::AI_MELEE || comp->ai_type==CompAI::AI_SHOOTER) {
				Entity* target=player;

				sf::Vector2f aim_pos=target->pos+target->vel*0.5f;

				//movement
				float speed=150.0f;
				float acc=200.0f;
				sf::Vector2f diff=aim_pos+comp->target_offset-comp->entity->pos;
				float dist=Utils::dist(diff);
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

		}
		for(Component* ccomp : entities.component_list(Component::TYPE_GUN)) {
			CompGun* g=(CompGun*)ccomp;

			if(g->cur_fire_timeout>0) {
				g->cur_fire_timeout-=dt;
			}
			if(!g->entity->fire_gun[g->group] || g->cur_fire_timeout>0) continue;
			g->cur_fire_timeout=g->fire_timeout;

			for(int i=0;i<g->bullet_count;i++) {
				float angle=g->angle;
				if(g->bullet_count>1) {
					angle=g->angle-g->angle_spread*0.5+(float)i/((float)g->bullet_count-1)*g->angle_spread;
				}

				Entity* bullet=create_bullet(g->texture,g->entity->player_side);
				bullet->pos=g->entity->pos+entity_rotate_vector(g->entity,g->pos);
				bullet->angle=g->entity->angle+angle;
				bullet->vel=Utils::vec_for_angle_deg(bullet->angle,g->bullet_speed);
				entity_add(bullet);
			}
		}
		for(Component* ccomp : entities.component_list(Component::TYPE_TELEPORTATION)) {
			CompTeleportation* tele=(CompTeleportation*)ccomp;

			tele->anim+=dt;
			if(tele->anim>=tele->anim_duration) {
				node_ships.remove_child(&tele->origin_node);
				entities.component_remove(tele);

				tele->entity->node_main.visible=true;	//scale=sf::Vector2f(1,1);
				tele->entity->shape.enabled=true;
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

			tele->entity->node_main.visible=false;//scale=sf::Vector2f(a,2.0f-a);
			tele->entity->pos=c_pos;

			tele->entity->shape.enabled=false;
		}
		for(Component* ccomp : entities.event_list_components(Component::TYPE_SHOW_DAMAGE,Component::EVENT_FRAME)) {
			CompShowDamage* comp=(CompShowDamage*)ccomp;

			comp->time+=dt;
			if(comp->time>=comp->time_max) {
				entities.event_remove(comp,Component::EVENT_FRAME);

				if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_BLINK) {
					comp->entity->node_main.color_add.set(0,0,0,0);
				}
				else if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
					use_shader_damage=false;
				}

				continue;
			}

			float a=1.0f-comp->time/comp->time_max;

			if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_BLINK) {
				comp->entity->node_main.color_add.set(a,a,a,0);
			}
			else if(comp->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
				shader_damage.set_param("amount",a*0.01);
			}
		}

		entities.update();
		//xxx
		for(Entity* e : entities.entities) {
			e->pos+=e->vel*dt;
			e->node_main.pos=e->pos;
			e->node_main.rotation=e->angle+90;
		}

		//camera

		sf::Vector2f camera_pos=player->pos; //-player->vel*0.1f;

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

		//post process
		node.post_process_shaders.clear();
		if(use_shader_damage) {
			node.post_process_shaders.push_back(shader_damage);
			shader_damage.set_param("time",game_time*0.4);
		}


		game_time+=dt;
	}
	bool event_input(const MenuEvent& event) {

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
			//other
			else if(c==sf::Keyboard::Space) {
				time_scale= (pressed ? 0.3 : 1.0);
			}
		}


		if(event.event.type==sf::Event::KeyPressed) {
			sf::Keyboard::Key c=event.event.key.code;

			if(c==sf::Keyboard::Escape) {
				trigger_message(action_esc);
				return true;
			}
			else if(c==sf::Keyboard::Q) {	//missiles
				launch_missiles(3,player->pos,player->player_side);
			}
			else if(c==sf::Keyboard::E) {	//teleport
				entity_teleport(player,screen_to_game_pos(pointer_pos));
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
	}
};

#endif
