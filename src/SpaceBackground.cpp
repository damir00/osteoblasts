#include <cmath>
#include <unordered_map>
#include <stdio.h>

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#include "SpaceBackground.h"
#include "Texture.h"
#include "Loader.h"
#include "Utils.h"

namespace {

class Spawner : public Node {
public:
	sf::IntRect current_area;

	std::unordered_map<std::string,Node*> items_map;

	float border;
	float spacing;

	std::vector<Texture> textures;

	static std::string item_hash(int x,int y) {
		char buf[50];
		sprintf(buf,"%d:%d",x,y);
		return buf;
	}
	sf::IntRect norm_rect(sf::Vector2f p,sf::FloatRect rect) {
		sf::IntRect r;
		r.left=std::floor(p.x/spacing);
		r.top=std::floor(p.y/spacing);
		r.width=std::ceil(rect.width/spacing);
		r.height=std::ceil(rect.height/spacing);
		return r;
	}

	Spawner(float border_size,float _spacing) {
		border=border_size;
		spacing=_spacing;
	}
	void frame(sf::Vector2f pos,sf::FloatRect area) {
		sf::IntRect area_norm=norm_rect(pos,area);

		//remove old
		float remove_space=spacing*2;
		for(std::unordered_map<std::string,Node*>::iterator it=items_map.begin();it!=items_map.end();) {
			Node* item=it->second;

			if(item->pos.x<pos.x-remove_space || item->pos.x>pos.x+area.width+remove_space ||
				item->pos.y<pos.y-remove_space || item->pos.y>pos.y+area.height+remove_space) {

				//GameRes::cache_sprite.put(item->sprite);
				remove_child(item);
				delete(item);
				it=items_map.erase(it);
				continue;
			}

			it++;
		}

		//add new
		float r=spacing*0.5;
		for(int xi=0;xi<=area_norm.width;xi++) {
			int x=xi+area_norm.left;

			bool x_inside=(x>=current_area.left && x<=(current_area.left+current_area.width) );

			for(int yi=0;yi<=area_norm.height;yi++) {
				int y=yi+area_norm.top;

				if(x_inside && y>=current_area.top && y<=(current_area.top+current_area.height)) {
					continue;
				}

				std::string hash=item_hash(x,y);

				if(items_map.count(hash)>0) continue;

				Node* item=new Node();	//memleak
				item->pos.x=(float)x*spacing+Utils::rand_range(-r,r);
				item->pos.y=(float)y*spacing+Utils::rand_range(-r,r);
				if(textures.size()>0) {
					item->texture=Utils::vector_rand(textures);
				}

				items_map[hash]=item;
				add_child(item);
			}
		}

		current_area=area_norm;
	}
};

}
class SpaceBackground::Layer : public Node {
public:
	Spawner spawner;
	float mult;
	Layer(float spacing) :
		spawner(10,spacing) {
		add_child(&spawner);
		//printf("child count: %d\n",children.size());
		mult=1;
	}
};
SpaceBackground::SpaceBackground() {
	bg.reset(new Node());
	bg->type=Node::TYPE_SOLID;
	bg->color=Color(0xff272c37);
	bg->scale=sf::Vector2f(100,100);
	add_child(bg.get());
}
SpaceBackground::~SpaceBackground() {
}

void SpaceBackground::set_background_visible(bool visible) {
	bg->visible=visible;
}
void SpaceBackground::add_layer(std::vector<Texture> bitmaps,float mult,float spacing) {
	Layer* layer=new Layer(spacing);
	layer->mult=mult;
	layer->spawner.textures=bitmaps;
	layers.push_back(std::unique_ptr<Layer>(layer));
	add_child(layer,1);
}

void SpaceBackground::update(sf::FloatRect area) {
	sf::Vector2f pos=Utils::area_center(area);
	for(std::size_t i=0;i<layers.size();i++) {
		sf::Vector2f t_pos=pos*layers[i]->mult;
		layers[i]->spawner.frame(t_pos,area);
		//layers[i]->spawner.offset=-t_pos;
		layers[i]->pos=-t_pos;
	}
	bg->scale=sf::Vector2f(area.width,area.height);
}

void SpaceBackground::create_default() {

	float layer_mult[]={0.8,0.3,0.1,0.05};
	//float layer_spacing[]={800,400,200,80};
	float layer_spacing[]={800/2,400/2,200/2,80/2};

	//SpaceBackground g;
    add_layer(Loader::get_texture_sequence("background/a",23),layer_mult[0],layer_spacing[0]);
    add_layer(Loader::get_texture_sequence("background/asteroid",5),layer_mult[1],layer_spacing[1]);
    add_layer(Utils::to_vec(Loader::get_texture("background/star_big.png")),layer_mult[2],layer_spacing[2]);
    add_layer(Utils::to_vec(Loader::get_texture("background/star_small.png")),layer_mult[3],layer_spacing[3]);
	//return g;
}
