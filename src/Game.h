
#ifndef _BGA_GAME_H_
#define _BGA_GAME_H_

#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <functional>

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
public:
	Graphic graphic;
	int anim_current_frame;
	float anim_time;
	bool repeat;

	GraphicNode() {
		anim_time=0;
		anim_current_frame=0;
		repeat=false;
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

		anim_time+=dt;
		if(repeat) {
			anim_time=fmod(anim_time,graphic.animation.duration);
		}
		else {
			anim_time=std::min(anim_time,graphic.animation.duration-0.001f);
		}

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
		TYPE_HEALTH,
		TYPE_GUN,
		TYPE_TIMEOUT,
		TYPE_AI,
		TYPE_ENGINE,
		TYPE_TELEPORTATION,
		TYPE_SHOW_DAMAGE,
		TYPE_BOUNCE,
		TYPE_FLAME_DAMAGE,
		TYPE_SHOW_ON_MINIMAP,
		TYPE_GRAVITY_FORCE,
		TYPE_SHIELD,
		TYPE_HAMMER
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
		COLLISION_GROUP_ENEMY=1<<4
	};

	uint8_t collision_group;
	uint8_t collision_mask;
	bool bounce;

	Quad bbox;
	bool enabled;

	CompShape() {
		collision_group=1;
		collision_mask=0xff;
		enabled=true;
		bounce=false;
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

	CompGun() {
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
		AI_FOLLOWER
	};
	AIType ai_type;

	EntityRef* target;

	sf::Vector2f rand_offset;	//normalized [-1,1]
	sf::Vector2f target_offset;	//world-space

	CompAI() {
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

	CompTeleportation() {
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
	SimpleTimer timer;

	CompShowDamage() {
		damage_type=DAMAGE_TYPE_BLINK;
		timer.reset(0.5);
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
		ATTRIBUTE_ATTRACT		//candy mine
	};

	//XXX move all members to comps/attrs
	//display
	Node node_main;
	CompDisplay display;

	//dynamics
	sf::Vector2f pos;	//center
	sf::Vector2f vel;
	sf::Vector2f bounce_vel;	//XXX remove
	float angle;	//deg

	//collisions
	CompShape shape;

	//health
	CompHealth health;

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

		tmp_damage=10;
		for(int i=0;i<10;i++) tmp_timeout[i]=0;
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
		if(comp==NULL) {
			return comp;
		}
		comp->entity=entity;
		Utils::vector_fit_size(component_map,type+1);
		component_map[type].push_back(comp);
		entity->components.push_back(comp);

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

				//xxx temp
				e->display.entity=e;
				e->display.type=Component::TYPE_DISPLAY;
				e->shape.entity=e;
				e->shape.type=Component::TYPE_SHAPE;
				e->health.entity=e;
				e->health.type=Component::TYPE_HEALTH;
				if(on_component_added) {
					on_component_added(&e->display);
					on_component_added(&e->shape);
					on_component_added(&e->health);
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

				//xxx temp
				if(on_component_removed) {
					on_component_removed(&e->display);
					on_component_removed(&e->shape);
					on_component_removed(&e->health);
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
			float h=300;

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
			duration=0.2;
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
	WaveManager wave_manager;

	std::vector<SwarmManager*> swarms;

	Node help_text_node;

	Minimap minimap;

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

	}

	Game() {

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
		node.add_child(&help_text_node);
		node_game.add_child(&terrain);
		node_game.add_child(&node_ships);


		help_text_node.type=Node::TYPE_TEXT;
		help_text_node.text.setFont(*Loader::get_menu_font());
		help_text_node.text.setCharacterSize(22);
		help_text_node.pos=sf::Vector2f(10,10);


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

		const char* help_texts[]={
				"Bastion\nQ - shield\nE - attract\nR - candy mine\nSPACE - hammer",
				"Engineer\nQ - mine\nE - helper\nSPACE - blackhole mine",
				"Assaulter\nQ - missiles\nE - teleport\nSPACE - bullet time",
				"Fighter\nunfinished"
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

		help_text_node.text.setString(help_texts[player_ship]);


		entity_add(player);

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

		player->shape.collision_group=CompShape::COLLISION_GROUP_PLAYER;
		player->shape.collision_mask=(CompShape::COLLISION_GROUP_ENEMY|
				CompShape::COLLISION_GROUP_ENEMY_BULLET|
				CompShape::COLLISION_GROUP_TERRAIN);
		player->shape.bounce=true;

		entities.attribute_add(player,Entity::ATTRIBUTE_PLAYER_CONTROL);
		entities.attribute_add(player,Entity::ATTRIBUTE_FRIENDLY);

		player->player_side=true;
		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(player,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT;
		player->health.reset(100);
		entity_show_on_minimap(player,Color(1,1,1,1));

		return player;
	}

	Entity* create_player_assaulter() {
		Entity* player=create_player();

		Texture tex=Loader::get_texture("player ships/Assaulter/assaulter.png");
		player->display.add_texture_center(tex);
		player->shape.add_quad_center(tex.get_size());

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

		return player;
	}

	Entity* create_player_bastion() {
		Entity* player=create_player();

		Texture tex_hammer=Loader::get_texture("player ships/Hammership/hammer.png");
		Node* hammer_node=player->display.add_texture(tex_hammer,sf::Vector2f(0,0));
		hammer_node->pos=sf::Vector2f(0,0);
		hammer_node->origin=sf::Vector2f(tex_hammer.get_size().x*0.5f,-18);

		Texture tex=Loader::get_texture("player ships/Hammership/ship1.png");
		player->display.add_texture_center(tex);
		player->shape.add_quad_center(tex.get_size());

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

		player->display.add_texture(Loader::get_texture("player ships/Slasher/blade1.png"),-tex.get_size()*0.5f+sf::Vector2f(5,-26));
		player->display.add_texture(Loader::get_texture("player ships/Slasher/blade2.png"),-tex.get_size()*0.5f+sf::Vector2f(68-5-11,-26));

		player->display.add_texture_center(tex);
		player->shape.add_quad_center(tex.get_size());

		for(int i=0;i<2;i++) {
			CompEngine* e=(CompEngine*)entities.component_add(player,Component::TYPE_ENGINE);
			e->set(graphic_engine,sf::Vector2f(-tex.get_size().x*0.5+16+i*37,tex.get_size().y*0.5));
		}

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

		return player;
	}

	Entity* create_player_engineer() {
		Entity* player=create_player();
		Texture tex=Loader::get_texture("player ships/Engineer/engineer.png");
		player->display.add_texture_center(tex);
		player->shape.add_quad_center(tex.get_size());

		Texture tex_hook=Loader::get_texture("player ships/Engineer/hook.png");

		player->display.add_texture(tex_hook,-tex.get_size()*0.5f+sf::Vector2f(1,-15));
		player->display.add_texture(tex_hook,-tex.get_size()*0.5f+sf::Vector2f(53-19-1,-15));

		CompEngine* e=(CompEngine*)entities.component_add(player,Component::TYPE_ENGINE);
		e->set(graphic_engine,sf::Vector2f(0,tex.get_size().y*0.5));

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
	Entity* create_mine() {
		Entity* e=create_bullet(Loader::get_texture("player ships/Hammership/sticky bomb.png"),true);
		CompTimeout* t=(CompTimeout*)entities.component_get(e,Component::TYPE_TIMEOUT);
		t->timeout=20.0f;
		e->tmp_damage=100.0f;
		return e;
	}
	Entity* create_candy_mine() {
		Entity* e=create_mine();
		CompTimeout* t=(CompTimeout*)entities.component_get(e,Component::TYPE_TIMEOUT);
		t->timeout=2.0f;
		t->action=CompTimeout::ACTION_BIG_EXPLOSION;
		e->tmp_damage=200.0f;
		e->shape.enabled=false;

		entities.attribute_add(e,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entities.attribute_add(e,Entity::ATTRIBUTE_ATTRACT);

		return e;
	}
	Entity* create_follower() {

		float scale=1.0;

		Entity* k=entities.entity_create();

		Texture tex=Loader::get_texture("player ships/Engineer/helper1.png");
		k->display.add_texture_center(tex,scale);

		k->shape.add_quad_center(tex.get_size()*scale);
		entities.attribute_add(k,Entity::ATTRIBUTE_REMOVE_ON_DEATH);
		entities.attribute_add(k,Entity::ATTRIBUTE_FRIENDLY);

		k->shape.collision_group=CompShape::COLLISION_GROUP_PLAYER;
		k->shape.collision_mask=(CompShape::COLLISION_GROUP_ENEMY|
				CompShape::COLLISION_GROUP_ENEMY_BULLET/*|
				CompShape::COLLISION_GROUP_TERRAIN*/);

		k->shape.bounce=true;

		k->player_side=true;


		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(k,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_BLINK;

		entity_show_on_minimap(k,Color(0,0,1,1));

		k->health.reset(100);

		CompAI* ai=(CompAI*)entities.component_add(k,Component::TYPE_AI);
		ai->ai_type=CompAI::AI_FOLLOWER;

		CompGun* gun=(CompGun*)entities.component_add(k,Component::TYPE_GUN);
		gun->texture=Loader::get_texture("player ships/Engineer/helper1proj.png");
		gun->bullet_speed=300;
		gun->fire_timeout=0.3;

		float aim_dist=200;
		/*
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
		k->shape.bounce=true;


		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_add(k,Component::TYPE_SHOW_DAMAGE);
		c_show_damage->damage_type=CompShowDamage::DAMAGE_TYPE_BLINK;

		entity_show_on_minimap(k,Color(1,0,0,1));

		k->health.reset(100);
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
		for(int i=0;i<count;i++) {
			sf::Vector2f p=swarm->get_entity_pos(i).pos;
			//entity_teleport(swarm->entities[i]->entity,player->pos+p);
		}

		swarms.push_back(swarm);
	}

	void add_big_explosion(sf::Vector2f pos) {
		float radius=150.0f;
		float radius2=radius*radius;


		for(int i=0;i<120;i++) {
			add_decal(Utils::vector_rand(graphic_explosion),
					pos+Utils::vec_for_angle_deg(Utils::rand_range(0,360),Utils::rand_range(0,radius)));
		}

		sf::Vector2f d_size=sf::Vector2f(1,1)*radius*1.5f;
		sf::Vector2f d_pos=pos-d_size*0.5f;
		terrain.damage_area(sf::FloatRect(d_pos,d_size),100);

		for(Entity * e : entities.entities) {
			if(e->shape.enabled && !e->player_side) {
				float dist=Utils::vec_length_fast(e->pos-pos);

				if(dist<radius2) {
					entity_damage(e,Utils::lerp(500,0,dist/radius2));
				}
			}
		}

	}
	Entity* add_decal(const Graphic& grap,sf::Vector2f pos) {
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
		return e;
	}
	Entity* add_decal(const std::vector<Graphic>& grap,sf::Vector2f pos) {
		if(grap.size()==0) {
			return nullptr;
		}
		return add_decal(Utils::vector_rand(grap),pos);
	}

	//"events"
	void entity_damage(Entity* e,float dmg) {

		if(entities.component_get(e,Component::TYPE_SHIELD)) {
			return;
		}

		e->health.health-=dmg;

		CompShowDamage* c_show_damage=(CompShowDamage*)entities.component_get(e,Component::TYPE_SHOW_DAMAGE);
		if(c_show_damage) {
			c_show_damage->timer.reset();
			entities.event_add(c_show_damage,Component::EVENT_FRAME);

			if(c_show_damage->damage_type==CompShowDamage::DAMAGE_TYPE_SCREEN_EFFECT) {
				use_shader_damage=true;
			}
		}

		if(e->health.health/e->health.health_max<0.3 && entities.component_get(e,Component::TYPE_FLAME_DAMAGE)==nullptr) {
			entities.component_add(e,Component::TYPE_FLAME_DAMAGE);
		}

		if(e->health.health<=0 && e->health.alive) {
			e->health.alive=false;

			if(entities.attribute_has(e,Entity::ATTRIBUTE_REMOVE_ON_DEATH)) {

				if(e->tmp_damage>90.0) {
					add_big_explosion(e->pos);
				}
				else {
					add_decal(graphic_explosion,e->pos);
				}
				entity_remove(e);
			}
		}
	}
	void entity_show_on_minimap(Entity* e,Color color) {
		CompShowOnMinimap* c=(CompShowOnMinimap*)entities.component_add(e,Component::TYPE_SHOW_ON_MINIMAP);
		c->node.type=Node::TYPE_SOLID;
		c->node.color=color;
		c->node.scale=sf::Vector2f(4,4);
		c->node.origin=sf::Vector2f(2,2);
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

	void wave_manager_frame(float dt) {
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

			for(int i=0;i<8;i++) {
				if(e->fire_gun[i]) {
					for(int i2=i+1;i2<8;i2++) {
						e->fire_gun[i2]=false;
					}
					break;
				}
			}

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

					//terrain collision
					if(e->shape.collision_mask&CompShape::COLLISION_GROUP_TERRAIN) {
						sf::Vector2f q_size=q2.p2-q2.p1;

						sf::FloatRect r(q2.p1,q_size);
						sf::Vector2f col_normal;
						if(terrain.check_collision(r,col_normal)) {
							terrain.damage_area(r,20);
							entity_damage(e,10);
							CompBounce* c_bounce=(CompBounce*)entities.component_add(e,Component::TYPE_BOUNCE);
							c_bounce->timer.reset(0.3);
							c_bounce->vel=Utils::vec_reflect(e->vel,col_normal);
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
								entity_damage(e,ce->tmp_damage);
								entity_damage(ce,e->tmp_damage);

								if(e->shape.bounce && ce->shape.bounce) {
									sf::Vector2f b_vel=Utils::vec_normalize(e->pos-ce->pos)*100.0f;

									CompBounce* c_bounce1=(CompBounce*)entities.component_add(e,Component::TYPE_BOUNCE);
									c_bounce1->timer.reset(0.3);
									c_bounce1->vel=b_vel;

									CompBounce* c_bounce2=(CompBounce*)entities.component_add(ce,Component::TYPE_BOUNCE);
									c_bounce2->timer.reset(0.3);
									c_bounce2->vel=-b_vel;
								}
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

		std::vector<sf::Vector2f> timeout_explosions;
		for(Component* ccomp : entities.component_list(Component::TYPE_TIMEOUT)) {
			CompTimeout* comp=(CompTimeout*)ccomp;

			comp->timeout-=dt;
			if(comp->timeout<=0) {
				if(comp->action==CompTimeout::ACTION_REMOVE_ENTITY) {
					entity_remove(comp->entity);
				}
				else if(comp->action==CompTimeout::ACTION_BIG_EXPLOSION) {
					timeout_explosions.push_back(comp->entity->pos);
					entity_remove(comp->entity);
				}
				entities.component_remove(comp);
			}
		}
		//XXX: temp. adding components (to other entities) while traversing component_list crashes
		for(int i=0;i<timeout_explosions.size();i++) {
			add_big_explosion(timeout_explosions[i]);
		}


		const std::vector<Entity*> attractors=entities.attribute_list_entities(Entity::ATTRIBUTE_ATTRACT);
		for(Component* ccomp : entities.component_list(Component::TYPE_AI)) {
			CompAI* comp=(CompAI*)ccomp;


			float min_dist=0;
			Entity* closest_attractor=nullptr;

			for(int i=0;i<attractors.size();i++) {
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
				diff+=comp->rand_offset*dist*0.3f;
				sf::Vector2f vel=diff/dist*speed;

				comp->entity->vel.x=Utils::num_move_towards(comp->entity->vel.x,vel.x,dt*acc);
				comp->entity->vel.y=Utils::num_move_towards(comp->entity->vel.y,vel.y,dt*acc);

				continue;
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
					for(int i=1;i<targeted.size();i++) {
						float dist=Utils::vec_length_fast(targeted[i]->pos-e->pos);
						if(dist<min_dist) {
							min_dist=dist;
							target=targeted[i];
						}
					}
				}

				if(target) {
					e->fire_gun[0]=true;
					const std::vector<Component*> guns=entities.component_list(Component::TYPE_GUN);

					for(int i=0;i<guns.size();i++) {
						CompGun* g=(CompGun*)guns[i];
						g->angle=Utils::rad_to_deg(Utils::vec_angle(target->pos-e->pos))-90;
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



		for(int i=0;i<swarms.size();i++) {
			SwarmManager* swarm=swarms[i];

			swarm->anim+=dt;

			int alive_count=0;
			for(int ei=0;ei<swarm->entities.size();ei++) {
				Entity* e=swarm->entities[ei]->entity;
				if(!e) {
					continue;
				}
				alive_count++;




				float min_dist=0;
				Entity* closest_attractor=nullptr;

				for(int i=0;i<attractors.size();i++) {
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
				/*
				float speed=1500.0f;
				float acc=2000.0f;
				sf::Vector2f diff=ctrl.pos-e->pos;
				float dist=Utils::dist(diff);
				sf::Vector2f vel=diff/dist*speed;

				e->vel.x=Utils::num_move_towards(e->vel.x,vel.x,dt*acc);
				e->vel.y=Utils::num_move_towards(e->vel.y,vel.y,dt*acc);
				*/

				//e->vel=Utils::vec_normalize(ctrl.pos-e->pos)*150.0f;

				e->vel=(ctrl.pos-e->pos)*10.0f;

				e->vel=Utils::vec_cap_length(e->vel,0,400.0f);

				e->fire_gun[0]=ctrl.fire;
			}

			if(alive_count==0) {
				for(int ei=0;ei<swarm->entities.size();ei++) {
					entities.entity_ref_delete(swarm->entities[ei]);
				}
				delete(swarm);
				swarms.erase(swarms.begin()+i);
				i--;
			}

		}

		wave_manager_frame(dt);


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
			float c=s*s*s;
			tele->origin_node.color_add.set(c,c,c,0);

			tele->entity->node_main.visible=false;//scale=sf::Vector2f(a,2.0f-a);
			tele->entity->pos=c_pos;

			tele->entity->shape.enabled=false;
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
			for(int i=0;i<comp->layers.size();i++) {
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


		entities.update();
		//xxx
		for(Entity* e : entities.entities) {
			e->pos+=e->vel*dt;
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

		for(int i=0;i<10;i++) {
			if(player->tmp_timeout[i]>=0.0) {
				player->tmp_timeout[i]-=dt;
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
				else {
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
	}
};

#endif
