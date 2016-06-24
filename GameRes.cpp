#include "GameRes.h"

#include <sstream>
#include <stdio.h>

#include "Parser.h"

sf::Sprite* allocate_sprite() {
	return new sf::Sprite();
}

using namespace std;

std::tr1::unordered_map<std::string,sf::Texture*> GameRes::textures;
std::tr1::unordered_map<std::string,SpriteAnimData*> GameRes::anims;
std::tr1::unordered_map<std::string,Ship*> GameRes::ships;
std::tr1::unordered_map<std::string,Atlas*> GameRes::atlases;

std::tr1::unordered_map<std::string, std::vector<Ship*> > GameRes::ship_groups;

std::vector<std::vector<sf::Texture*> > GameRes::bg_items;
sf::Texture* GameRes::player_ship;
sf::Texture* GameRes::projectile;
sf::Texture* GameRes::projectile_2;

sf::Texture* GameRes::projectile_penguin;
sf::Texture* GameRes::projectile_shark;
sf::Texture* GameRes::projectile_starfish;

ImageData* GameRes::terrain_texture;
std::vector<sf::Texture*> GameRes::rockets;
SpriteAnimData* GameRes::anim_explosion;
std::vector<SpriteAnimData*> GameRes::anim_explosions;
SpriteAnimData* GameRes::anim_rocket_fire;

std::vector<sf::Texture*> GameRes::enemies_melee;

Cache<sf::Sprite*> GameRes::cache_sprite(allocate_sprite);
sf::Font GameRes::font;

sf::Shader GameRes::shader_damage;
sf::Shader GameRes::shader_blast;
sf::Shader GameRes::shader_blink;

template<class T>
std::vector<T> to_vec(T item) {
	std::vector<T> v;
	v.push_back(item);
	return v;
}
std::vector<SpriteAnimData*> anims_expand(SpriteAnimData* anim) {
	std::vector<SpriteAnimData*> anims;

	anim->scale=2;

	anims.push_back(anim);
	for(int i=1;i<anim->sections.size();i++) {
		SpriteAnimData* d=new SpriteAnimData();
		*d=*anim;
		d->frame_start=d->sections[i];
		d->frame_end=( d->sections.size()>(i+1) ? d->sections[i+1] : d->frames.size());
		anims.push_back(d);
	}

	return anims;
}

void GameRes::init() {
		printf("Assets init\n");
        bg_items.push_back(get_sequence("background/a",23));
        bg_items.push_back(get_sequence("background/asteroid",5));
        bg_items.push_back(to_vec(get_texture("background/star_big.png")));
        bg_items.push_back(to_vec(get_texture("background/star_small.png")));

        player_ship=get_texture("hero/hero_idle.png");
        projectile=get_texture("hero/projectile_1.png");
        projectile_2=get_texture("hero/projectile_2.png");

        projectile_penguin=get_texture("enemies/shooter/proj_penguin.png");
        projectile_shark=get_texture("enemies/shooter/proj_shark.png");
        projectile_starfish=get_texture("enemies/shooter/proj_starfish.png");

        terrain_texture=load_imagedata("terrain/texture.png");

        rockets=get_sequence("hero/rocket",6);

        anim_explosion=get_anim("explosion.png");
        anim_explosions=anims_expand(get_anim("explosions.png"));
        //anim_explosions.push_back(anim_explosion);

        ParserJSON::parse_ship_groups("assets/ships.json",ship_groups);

        font.loadFromFile("assets/Verdana.ttf");

    	bool shader_enabled=sf::Shader::isAvailable();
    	if(shader_enabled) {
    		if(shader_damage.loadFromFile("assets/shader/damage.frag",sf::Shader::Fragment)) {
    			shader_damage.setParameter("texture", sf::Shader::CurrentTexture);
    		}
    		else {
    			printf("damage shader failed to compile\n");
    		}
    		if(shader_blast.loadFromFile("assets/shader/blast.frag",sf::Shader::Fragment)) {
    			shader_blast.setParameter("texture", sf::Shader::CurrentTexture);
    		}
    		else {
    			printf("blast shader failed to compile\n");
    		}

    		shader_blink.loadFromFile("assets/shader/blink.frag",sf::Shader::Fragment);
    		shader_blink.setParameter("texture", sf::Shader::CurrentTexture);
    	}
}

std::string transform_filename(std::string filename) {
	return "assets/"+filename;
}
sf::Texture* GameRes::get_texture(std::string filename) {
	if(textures.count(filename)>0) {
		return textures[filename];
	}
	sf::Texture* tex=load_texture(transform_filename(filename));
	textures[filename]=tex;
	return tex;
}
SpriteAnimData* GameRes::get_anim(std::string filename) {
	if(anims.count(filename)>0) {
		return anims[filename];
	}
	SpriteAnimData* anim=load_anim(transform_filename(filename));
	anims[filename]=anim;
	return anim;
}
Atlas* GameRes::get_atlas(std::string filename) {
	if(atlases.count(filename)>0) {
		return atlases[filename];
	}
	Atlas* a=load_atlas(filename);
	if(!a) {
		return NULL;
	}
	atlases[filename]=a;
	return a;
}
std::vector<sf::Texture*> GameRes::get_sequence(std::string path,int count) {
	std::vector<sf::Texture*> seq;

	for(int i=1;i<=count;i++) {
		std::ostringstream s;
		s<<path<<"_"<<i<<".png";
		seq.push_back(get_texture(s.str()));
	}

	return seq;
}
Ship* GameRes::get_ship(std::string filename) {
	if(ships.count(filename)>0) {
		return ships[filename]->clone();
	}
	Ship* ship=load_ship(transform_filename(filename));
	if(!ship) {
		return NULL;
	}
	ships[filename]=ship;
	return ship->clone();
}
int GameRes::ship_group_count(std::string group) {
	if(ship_groups.count(group)==0) {
		return 0;
	}
	return ship_groups[group].size();
}
Ship* GameRes::ship_group_get(std::string group,int i) {
	if(ship_groups.count(group)==0) return NULL;
	std::vector<Ship*> &ships=ship_groups[group];
	if(i>=ships.size()) return NULL;
	return ships[i]->clone();
}
Ship* GameRes::ship_group_get_rand(std::string group) {
	int c=ship_group_count(group);
	if(c==0) return NULL;
	return ship_group_get(group,Utils::rand_range_i(0,c-1))->clone();
}


ImageData* GameRes::load_imagedata(std::string filename) {
	sf::Image img;
	img.loadFromFile(transform_filename(filename));
	ImageData* data=new ImageData();
	data->from_image(img);
	return data;
}

sf::Texture* GameRes::load_texture(std::string filename) {
	sf::Texture* t=new sf::Texture();
	if(!t->loadFromFile(filename)) {
		printf("Can't load image %s\n",filename.c_str());
	}
	return t;
}

SpriteAnimData* GameRes::load_anim(std::string filename) {

	int frame_w, frame_h, delay_ms;
	std::vector<int> sections;

	if(!ParserJSON::parse_anim_info(Utils::file_swap_extension(filename,"json"),frame_w,frame_h,delay_ms,sections)) {
		return NULL;
	}

	SpriteAnimData* anim=new SpriteAnimData();

	anim->scale=1;
	anim->delay=delay_ms;
	anim->texture=load_texture(filename);
	sf::Vector2u tex_size=anim->texture->getSize();

	sf::Vector2u current_pos(0,0);

	while(current_pos.y+frame_h<=tex_size.y) {
		anim->frames.push_back(sf::IntRect(current_pos.x,current_pos.y,frame_w,frame_h));
		current_pos.x+=frame_w;
		if(current_pos.x+frame_w>tex_size.x) {
			current_pos.x=0;
			current_pos.y+=frame_h;
		}
	}

	anim->sections=sections;
	anim->frame_start=0;
	anim->frame_end=( sections.size()>1 ? sections[1] : anim->frames.size() );

	return anim;
}
Atlas* GameRes::load_atlas(std::string filename) {
	Atlas* a=ParserJSON::parse_atlas(ParserJSON::read_file(Utils::file_swap_extension(transform_filename(filename),"json")));
	if(!a) {
		return NULL;
	}
	a->texture=get_texture(filename);
	if(!a->texture) {
		delete(a);
		return NULL;
	}
	return a;
}

Ship* GameRes::load_ship(std::string filename) {
	return ParserJSON::parse_ship(filename);
}

void GameRes::insert_ship(std::string id,Ship* ship) {
	ships[id]=ship;
}
