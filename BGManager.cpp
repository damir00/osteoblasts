#include "BGManager.h"

#include "GameRes.h"
#include "Utils.h"

#include <stdio.h>
#include <cmath>

BGSpawner::BGSpawner(float border_size,float _spacing) {
	border=border_size;
	spacing=_spacing;
}
void BGSpawner::set_spacing(float _spacing) {
	spacing=_spacing;
	for(std::tr1::unordered_map<std::string,BGSpawnerItem*>::iterator it=items_map.begin();it!=items_map.end();it++) {
		delete(it->second);
	}
	items_map.clear();
	//bitmaps.clear();
	current_area=sf::IntRect(0,0,0,0);
}
float BGSpawner::get_spacing() {
	return spacing;
}
void BGSpawner::frame(sf::Vector2f pos,sf::FloatRect area) {

	sf::IntRect area_norm=norm_rect(pos,area);

	//remove old
	float remove_space=spacing*2;
	for(std::tr1::unordered_map<std::string,BGSpawnerItem*>::iterator it=items_map.begin();it!=items_map.end();) {
		BGSpawnerItem* item=it->second;

		if(item->pos.x<pos.x-remove_space || item->pos.x>pos.x+area.width+remove_space ||
			item->pos.y<pos.y-remove_space || item->pos.y>pos.y+area.height+remove_space) {

			GameRes::cache_sprite.put(item->sprite);
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

			//if(items_map[hash]!=NULL) continue;
			if(items_map.count(hash)>0) continue;

			BGSpawnerItem* item=new BGSpawnerItem();
			item->sprite=GameRes::cache_sprite.get();
			item->pos.x=(float)x*spacing+Utils::rand_range(-r,r);
			item->pos.y=(float)y*spacing+Utils::rand_range(-r,r);
			item->sprite->setTexture(*Utils::vector_rand(bitmaps),true);

			items_map[hash]=item;
		}
	}

	current_area=area_norm;
}

std::string BGSpawner::item_hash(int x,int y) {
	char buf[100];
	sprintf(buf,"%d:%d",x,y);
	return buf;
}
sf::IntRect BGSpawner::norm_rect(sf::Vector2f p,sf::FloatRect rect) {
	sf::IntRect r;
	r.left=std::floor(p.x/spacing);
	r.top=std::floor(p.y/spacing);
	r.width=std::ceil(rect.width/spacing);
	r.height=std::ceil(rect.height/spacing);
	return r;
}

void BGSpawner::draw(const NodeState& state) {
	//sf::RenderStates mystate=combine_render_state(state);

	for(std::tr1::unordered_map<std::string,BGSpawnerItem*>::iterator it=items_map.begin();it!=items_map.end();it++) {
		BGSpawnerItem* item=it->second;

		sf::Transform tr=state.render_state.transform;
		tr.translate(offset+item->pos);
		//mystate.transform=state.transform;
		//mystate.transform.translate(offset+item->pos);

		state.render_target->draw(*item->sprite,tr);
	}
}


BGManagerLayer::BGManagerLayer(float spacing) : spawner(10,spacing) {

}

BGManager::BGManager() {
	float layer_mult[]={0.8,0.3,0.1,0.05};
	float layer_spacing[]={800,400,200,80};
	
	for(int i=GameRes::bg_items.size()-1;i>=0;i--) {
		add_layer(GameRes::bg_items[i],layer_mult[i],layer_spacing[i]);
	}

	sf::RectangleShape* rect=new sf::RectangleShape(sf::Vector2f(10000,10000));
	rect->setFillColor(Utils::hex_to_color(0xff272c37));

	bg=new NodeContainer(rect);
	background_color=true;
}

void BGManager::add_layer(std::vector<sf::Texture*> bitmaps,float mult,float spacing) {
	BGManagerLayer layer(spacing);
	layer.mult=mult;
	layer.spawner.bitmaps=bitmaps;
	layers.push_back(layer);
}

void BGManager::frame(sf::Vector2f pos,sf::FloatRect area) {
	for(int i=0;i<layers.size();i++) {
		sf::Vector2f t_pos=pos*layers[i].mult;
		layers[i].spawner.frame(t_pos,area);
		layers[i].spawner.offset=-t_pos;
	}
}
void BGManager::draw(const NodeState& state) {
	if(background_color) {
		state.render_target->draw(*bg->item,state.render_state);
	}
	for(int i=0;i<layers.size();i++) {
		layers[i].spawner.draw(state);
	}
}



