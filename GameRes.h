#ifndef _GameRes_H_
#define _GameRes_H_

#include <SFML/Graphics.hpp>

#include "Utils.h"
#include "Anim.h"

#include <string.h>
#include <cmath>
#include <tr1/unordered_map>

class Ship;

class ImageDataFloat {
public:
	unsigned int data_size;
	float* data;
	sf::Vector2u size;
	ImageDataFloat() {
		data=NULL;
	}
	~ImageDataFloat() {
		delete(data);
	}
	void create(int width,int height) {
		delete(data);
		size=sf::Vector2u(width,height);
		data_size=width*height;
		data=new float[data_size];
		memset(data,0,data_size);
	}
	//x,y=normalized [0-1]
	float lookup(float x,float y) {
		x*=size.x;
		y*=size.y;
		int xi=std::floor(x);
		int yi=std::floor(y);

		x-=xi;	//norm
		y-=yi;

		//p1 - p2
		// |   |
		//p3 - p4

		float p1=lookup_pixel(xi,yi);
		float p2=lookup_pixel(xi+1,yi);
		float p3=lookup_pixel(xi,yi+1);
		float p4=lookup_pixel(xi+1,yi+1);

		return Utils::lerp(
				Utils::lerp(p1,p2,x),
				Utils::lerp(p3,p4,x),
				y);
	}
	float lookup_pixel(int x,int y) {
		if(x<0 || y<0 || x>=size.x || y>=size.y) {
			return 0;
		}
		return data[y*size.x+x];
	}
};

class ImageData {
public:
	unsigned int data_size;
	sf::Uint8* data;
	sf::Vector2u size;

	ImageData() {
		data=NULL;
		data_size=0;
	}
	~ImageData() {
		delete(data);
	}
	void unload() {
		data_size=0;
		delete(data);
		data=NULL;
		size=sf::Vector2u(0,0);
	}
	void from_image(sf::Image image) {
		delete(data);

		size=image.getSize();

		data_size=size.x*size.y*4;
		data=new sf::Uint8[data_size];
		memcpy(data,image.getPixelsPtr(),data_size);
	}
	void create(int width,int height) {
		delete(data);

		size=sf::Vector2u(width,height);
		data_size=size.x*size.y*4;
		data=new sf::Uint8[data_size];
		memset(data,0,data_size);
	}


};

class GameRes {

	static std::tr1::unordered_map<std::string,sf::Texture*> textures;
	static std::tr1::unordered_map<std::string,SpriteAnimData*> anims;
	static std::tr1::unordered_map<std::string,Ship*> ships;
	static std::tr1::unordered_map<std::string,Atlas*> atlases;

	static sf::Texture* load_texture(std::string filename);
	static ImageData* load_imagedata(std::string filename);
	static SpriteAnimData* load_anim(std::string filename);
	static Atlas* load_atlas(std::string filename);
	static Ship* load_ship(std::string filename);

	static std::vector<sf::Texture*> get_sequence(std::string path,int count);

	static std::tr1::unordered_map<std::string, std::vector<Ship*> > ship_groups;

public:

	static std::vector<std::vector<sf::Texture*> > bg_items;
	static sf::Texture* player_ship;

	static sf::Texture* projectile;
	static sf::Texture* projectile_2;
	static sf::Texture* projectile_penguin;
	static sf::Texture* projectile_shark;
	static sf::Texture* projectile_starfish;

	static ImageData* terrain_texture;

	//static std::vector<sf::Texture*> enemies_kamikaze;
	//static std::vector<SpriteAnimData*> enemies_kamikaze_anim;
	static std::vector<sf::Texture*> enemies_melee;
	//static std::vector<sf::Texture*> enemies_shooter;
	//static std::vector<SpriteAnimData*> enemies_shooter_anim;
	//static std::vector<SpriteAnimData*> enemies_shooter_anim_fire;

	static std::vector<sf::Texture*> rockets;
	//static std::vector<sf::Texture*> drones;

	static SpriteAnimData* anim_explosion;
	static std::vector<SpriteAnimData*> anim_explosions;
	static SpriteAnimData* anim_rocket_fire;

	static Cache<sf::Sprite*> cache_sprite;

	static sf::Shader shader_damage;
	static sf::Shader shader_blast;
	static sf::Shader shader_blink;

	static sf::Font font;

	static void init();

	static sf::Texture* get_texture(std::string id);
	static SpriteAnimData* get_anim(std::string id);
	static Atlas* get_atlas(std::string id);
	static Ship* get_ship(std::string id);

	static int ship_group_count(std::string group);
	static Ship* ship_group_get(std::string group,int i);
	static Ship* ship_group_get_rand(std::string group);

	static void insert_ship(std::string id,Ship* ship);


};

#endif

