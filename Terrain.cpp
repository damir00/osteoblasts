#include "Terrain.h"

#include <noise/noise.h>
#include <cmath>

ImageDataFloat* TerrainIsland::dist_map=NULL;
ImageDataFloat* TerrainIsland::noise_map=NULL;
TerrainLoader* TerrainIsland::loader=NULL;


class TerrainLoaderTaskUpdateArea : public TerrainLoaderTask {
	TerrainIsland* island;
	sf::IntRect update_rect;
public:
	TerrainLoaderTaskUpdateArea(TerrainIsland* _island,sf::IntRect _rect) {
		island=_island;
		update_rect=_rect;
	}
	void execute() {
		island->update_area(update_rect);
	}
};
class TerrainLoaderTaskInit : public TerrainLoaderTask {
	TerrainIsland* island;
public:
	TerrainLoaderTaskInit(TerrainIsland* _island) {
		island=_island;
	}
	void execute() {
		island->init();
	}
};


float TerrainIsland::image_get_light(const ImageData& data,int px,int py,int radius) {
	float light=1.0;
	for(int x=px-radius;x<=px+radius;x++) {
		if(x>0 && x<data.size.x) {
			int data_i=(py*data.size.x+x)*4;
			if(data.data[data_i+3]!=0) continue;
		}
		if(x>px) light-=1;
		else light+=1;
	}
	float limit=0.3;
	if(light>1.0+limit) light=1.0+limit;
	else if(light<1.0-limit) light=1.0-limit;
	return light;
}


//rect is in cell coordinate system
void TerrainIsland::update_area_cell(sf::IntRect rect) {

	int px1=rect.left*cell_size;
	int px2=(rect.left+rect.width)*cell_size;
	int py1=rect.top*cell_size;
	int py2=(rect.top+rect.height)*cell_size;

	update_area(sf::IntRect(px1,py1,px2-px1,py2-py1));
}

//rect is in texture coordinate system
void TerrainIsland::update_area(sf::IntRect rect) {

	if(!texture) {
		return;
	}

	load_mutex.lock();

	if(!loaded) {
		load_mutex.unlock();
		return;
	}

	int px1=Utils::clampi(0,gpu_texture->getSize().x,rect.left);
	int py1=Utils::clampi(0,gpu_texture->getSize().y,rect.top);
	int px2=Utils::clampi(0,gpu_texture->getSize().x,rect.left+rect.width);
	int py2=Utils::clampi(0,gpu_texture->getSize().y,rect.top+rect.height);

	if(px1==px2 || py1==py2) {
		load_mutex.unlock();
		return;
	}

	for(int x=px1;x<px2;x++) {
		for(int y=py1;y<py2;y++) {

			float norm_x=(float)x/(float)cell_size;
			float norm_y=(float)y/(float)cell_size;

			int map_x=std::floor(norm_x);
			int map_y=std::floor(norm_y);

			int noise_i=(( (y+noise_offset.y)%noise_map->size.y)*noise_map->size.x+((x+noise_offset.x)%noise_map->size.x));
			int data_i=(y*result.size.x+x)*4;

			int p1=map[((map_y+0)*w+(map_x+0))].active;
			int p2=map[((map_y+0)*w+std::min(w-1,map_x+1))].active;
			int p3=map[(std::min(h-1,map_y+1)*w+(map_x+0))].active;
			int p4=map[(std::min(h-1,map_y+1)*w+std::min(w-1,map_x+1))].active;

			float lx=norm_x-map_x;
			float ly=norm_y-map_y;

			float val=Utils::lerp(
					Utils::lerp(p1,p2,lx),
					Utils::lerp(p3,p4,lx),
					ly);

			val+=noise_map->data[noise_i];

			int color=(val>0.5 ?  255 : 0);

			result.data[data_i+3]=color;
		}
	}
	//texturing & shading
	if(texture->size.x>0 && texture->size.y>0) {
		for(int x=px1;x<px2;x++) {
			for(int y=py1;y<py2;y++) {

				int data_i=(y*result.size.x+x)*4;
				int tex_i=(
						((y*3)%texture->size.y)*texture->size.x+
						((x*3)%texture->size.x) )*4;

				if(result.data[data_i+3]==0) {
					continue;
				}

				float l=image_get_light(result,x,y,5);
				result.data[data_i+0]=Utils::clampi(0,255,(int)texture->data[tex_i+0]*l);
				result.data[data_i+1]=Utils::clampi(0,255,(int)texture->data[tex_i+1]*l);
				result.data[data_i+2]=Utils::clampi(0,255,(int)texture->data[tex_i+2]*l);

			}
		}
	}

	load_mutex.unlock();

	sf::IntRect(0,0,0,0);

	texture_mutex.lock();
	texture_updates.push_back(sf::IntRect(px1,py1,px2-px1,py2-py1));
	texture_mutex.unlock();
}
void TerrainIsland::update_texture(sf::IntRect rect) {

	if(rect.width==result.size.x) {
		//printf("full update\n");
		gpu_texture->update(result.data+rect.left*result.size.x*4,rect.width,rect.height,rect.left,rect.top);
	}
	//update delta only
	else {

		//printf("update %d %d %d %d texture %d %d\n",px1,py1,px2,py2,gpu_texture->getSize().x,gpu_texture->getSize().y);
		sf::Uint8* src=result.data+((rect.top*result.size.x)+rect.left)*4;
		sf::Uint8* dest=result_temp.data;

		int src_stride=result.size.x*4;
		int dest_stride=(rect.width)*4;

		int lines=rect.height;

		for(int i=0;i<lines;i++) {
			memcpy(dest,src,dest_stride);
			src+=src_stride;
			dest+=dest_stride;
		}
		gpu_texture->update(result_temp.data,rect.width,rect.height,rect.left,rect.top);
	}


	/*
	if(px2-px1==result.size.x) {
		//printf("full update\n");
		gpu_texture->update(result.data+py1*result.size.x*4,px2-px1,py2-py1,px1,py1);
	}
	//update delta only
	else {

		//printf("update %d %d %d %d texture %d %d\n",px1,py1,px2,py2,gpu_texture->getSize().x,gpu_texture->getSize().y);
		sf::Uint8* src=result.data+((py1*result.size.x)+px1)*4;
		sf::Uint8* dest=result_temp.data;

		int src_stride=result.size.x*4;
		int dest_stride=(px2-px1)*4;

		int lines=py2-py1;

		for(int i=0;i<lines;i++) {
			memcpy(dest,src,dest_stride);
			src+=src_stride;
			dest+=dest_stride;
		}
		gpu_texture->update(result_temp.data,px2-px1,py2-py1,px1,py1);
	}
	*/
}


TerrainIsland::TerrainIsland(BoundingBox _box) {

	box=_box;
	render_state.transform.translate(box.start);
	texture=GameRes::terrain_texture;

	if(!loader) {
		loader=new TerrainLoader();
	}

	if(!noise_map) {
		noise::module::Perlin texture_perlin;
		texture_perlin.SetFrequency(0.05*20);
		texture_perlin.SetOctaveCount(2);
		texture_perlin.SetSeed(rand());

		noise::model::Sphere sphere(texture_perlin);

		noise_map=new ImageDataFloat();
		noise_map->create(128,128);
		for(int x=0;x<noise_map->size.x;x++) {
			for(int y=0;y<noise_map->size.y;y++) {

				float ax=(float)x/(float)noise_map->size.x*360-180;
				float ay=(float)y/(float)noise_map->size.y*360-180;

				noise_map->data[y*noise_map->size.x+x]=sphere.GetValue(ax,ay)*0.3;
						//texture_perlin.GetValue(x,y,0)*0.4;
			}
		}
	}

	if(!dist_map) {
		dist_map=new ImageDataFloat();
		dist_map->create(11,11);

		int cx=dist_map->size.x/2+1;
		int cy=dist_map->size.y/2+1;
		float norm=Utils::dist(cx,cy);
		for(int x=0;x<dist_map->size.x;x++) {
			for(int y=0;y<dist_map->size.y;y++) {
				float d=Utils::dist(x-cx,y-cy)/norm;
				dist_map->data[y*dist_map->size.x+x]=Utils::clamp(0,1,1-d);
			}
		}
		/*
		for(int x=0;x<10;x++) {
			for(int y=0;y<10;y++) {
				float v=dist_map->lookup(
						((float)x)/9.0,
						((float)y)/9.0);
				printf("%0.2f ",v);
			}
			printf("\n");
		}
		*/
	}

	cell_size=16;
	w=box.getSize().x/cell_size/3;
	h=box.getSize().y/cell_size/3;

	//printf("Terrain size %dx%d\n",w,h);

	load_chunk_size=256;

	noise_offset=sf::Vector2i(
			rand()%noise_map->size.x,
			rand()%noise_map->size.y);

	gpu_texture=NULL;

	loaded=false;
	map=NULL;
}
void TerrainIsland::update_visual(sf::FloatRect rect) {

	if(!map) return;

	sf::FloatRect world_rect=rect;
	world_rect.left+=box.start.x;
	world_rect.top+=box.start.y;

	BoundingBox cam_bb(world_rect);

	bool intersecting=box.intersects(cam_bb);

	if(!loaded) {
		if(!intersecting) {
			return;
		}
		load();
	}
	if(!intersecting) {
		unload();
		return;
	}


	texture_mutex.lock();
	for(int i=0;i<texture_updates.size();i++) {
		update_texture(texture_updates[i]);
	}
	texture_updates.clear();
	texture_mutex.unlock();
	visible=true;


	sf::FloatRect rect2;
	rect2.left=rect.left/3;
	rect2.top=rect.top/3;
	rect2.width=rect.width/3;
	rect2.height=rect.height/3;

	IntBoundingBox chunk_rect=load_chunks.intersect(rect2);

	for(int x=0;x<load_chunks.size.x;x++) {
		for(int y=0;y<load_chunks.size.y;y++) {

			int i=y*load_chunks.size.x+x;

			//not visible
			if(x<chunk_rect.start.x || x>chunk_rect.end.x ||
				y<chunk_rect.start.y || y>chunk_rect.end.y) {
				continue;
			}

			//visible
			if(!load_chunks.chunks[i]) {
				//printf("load chunk %d %d\n",x,y);
				load_chunks.chunks[i]=true;

				sf::IntRect update_rect(
					x*load_chunks.chunk_size-5,
					y*load_chunks.chunk_size-5,
					load_chunks.chunk_size+10,
					load_chunks.chunk_size+10);

				/*
				update_area(update_rect);
				*/
				loader->add_task(new TerrainLoaderTaskUpdateArea(this,update_rect));
			}
		}
	}
}
void TerrainIsland::init() {
	map=new TerrainIslandPoint[w*h];

	noise::module::Perlin perlin;
	perlin.SetFrequency(0.2);
	perlin.SetOctaveCount(2);
	perlin.SetSeed(rand());

	for(int x=0;x<w;x++) {
		for(int y=0;y<h;y++) {
			int i=y*w+x;

			float noise=perlin.GetValue(x,y,0);
			noise=noise*0.5+1;
			noise*=dist_map->lookup(
					(float)x/(float)(w-1),
					(float)y/(float)(h-1)
					);

			if(noise<0.5) {
				map[i].active=false;
			}
			else {
				map[i].active=true;
				float d=cell_size/2;
				map[i].pos.x=x*cell_size+Utils::rand_range(-d,d);
				map[i].pos.y=y*cell_size+Utils::rand_range(-d,d);
				map[i].size=Utils::rand_range(cell_size*0.5,cell_size*0.75);
			}
		}
	}
}
void TerrainIsland::load() {
	if(!map || loaded) return;

	//printf("Load terrain\n");

	result.create(w*cell_size,h*cell_size);
	result_temp.create(w*cell_size,h*cell_size);

	gpu_texture=new sf::Texture();
	gpu_texture->create(result.size.x,result.size.y);
	gpu_texture->setSmooth(false);

	sprite.setTexture(*gpu_texture);
	sprite.setScale(3,3);
	//update_area_cell(sf::IntRect(0,0,w,h));

	load_chunks.for_size(sf::Vector2f(
				box.getSize().x/3,
				box.getSize().y/3),
			load_chunk_size);
	load_chunks.set_all(false);

	texture_updates.clear();

	/*
	printf("Terrain size %d %d, chunks size %d %d (%dx chunk)\n",
			result.size.x,result.size.y,
			load_chunks.size.x,load_chunks.size.y,
			load_chunks.chunk_size);
	*/

	visible=false;
	loaded=true;
}
void TerrainIsland::unload() {
	if(!map || !loaded) return;

	//printf("Unload terrain\n");

	load_mutex.lock();

	delete(gpu_texture);
	gpu_texture=NULL;

	result.unload();
	result_temp.unload();
	load_chunks.unload();

	loaded=false;

	load_mutex.unlock();
}


void TerrainIsland::damage_area(sf::FloatRect rect) {
	if(!map) return;

	float mult=1.0f/(cell_size*3.0);
	int x1=Utils::clampi(0,w,std::floor( rect.left *mult ));
	int y1=Utils::clampi(0,h,std::floor( rect.top  *mult ));
	int x2=Utils::clampi(0,w,std::ceil( (rect.left+rect.width) *mult ));
	int y2=Utils::clampi(0,h,std::ceil( (rect.top+rect.height) *mult ));

	//for(int i=0;i<w*h;i++) map[i].active=false;

	load_mutex.lock();

	for(int x=x1;x<x2;x++) {
		for(int y=y1;y<y2;y++) {
			int map_i=y*w+x;
			if(!map[map_i].active) continue;
			map[map_i].active=false;
		}
	}

	load_mutex.unlock();

	if(!loaded) {
		return;
	}

	update_area_cell(sf::IntRect(
				std::max(0,x1-1),
				std::max(0,y1-1),
				std::min(w,x2-x1+1),
				std::min(h,y2-y1+1)
			));
}
bool TerrainIsland::check_collision(sf::FloatRect rect) {
	if(!map) return false;

	float mult=1.0f/(cell_size*3.0);
	int x1=Utils::clampi(0,w,std::floor( rect.left *mult ));
	int y1=Utils::clampi(0,h,std::floor( rect.top  *mult ));
	int x2=Utils::clampi(0,w,std::ceil( (rect.left+rect.width) *mult ));
	int y2=Utils::clampi(0,h,std::ceil( (rect.top+rect.height) *mult ));

	for(int x=x1;x<x2;x++) {
		for(int y=y1;y<y2;y++) {
			int map_i=y*w+x;
			if(map[map_i].active) return true;
		}
	}
	return false;
}
bool TerrainIsland::check_collision(sf::Vector2f pos) {
	if(!map) return false;

	float mult=1.0f/(cell_size*3.0f);

	int x=std::floor(pos.x*mult);
	int y=std::floor(pos.y*mult);

	if(x<0 || y<0 || x>=w || y>=h) return false;

	return map[y*w+x].active;
}

void TerrainIsland::draw(const NodeState& state) {
	if(!loaded) return;
	sf::RenderStates s=state.render_state;
	s.transform.translate(offset);
	state.render_target->draw(sprite,s);
}



void Terrain::add_island(TerrainIsland* island) {
	childs.push_back(island);
	islands.push_back(island);

	TerrainIsland::loader->add_task(new TerrainLoaderTaskInit(island));
}

Terrain::Terrain() {

	/*
	sf::Vector2f size(
				Utils::rand_range(1000,2000),
				Utils::rand_range(1000,2000)
		);
	sf::Vector2f pos=sf::Vector2f(100,100);
	sf::Vector2f pos2=sf::Vector2f(2100,100);
	*/

	//add_island(new TerrainIsland(BoundingBox(pos,pos+size)));
	//add_island(new TerrainIsland(BoundingBox(pos2,pos2+size)));

	field_size=sf::Vector2f(75000,75000);
	sf::Vector2f rock_count(50,50);

	sf::Vector2f size_min=sf::Vector2f(
			field_size.x/rock_count.x,
			field_size.y/rock_count.y)*0.5f;
	sf::Vector2f size_max=size_min*2.0f;


	for(int x=0;x<rock_count.x;x++) {
		for(int y=0;y<rock_count.y;y++) {
			if((x+y)%2==0) continue;

			float tx=Utils::rand_range(0,1);
			float scale=Utils::rand_range(0.5,2.5);
			float width=Utils::lerp(size_min.x,size_max.x,tx);
			float height=Utils::lerp(size_min.y,size_max.y,1-tx);

			//sf::Vector2f size(10000/50,10000/50);
			sf::Vector2f size=sf::Vector2f(width,height)*scale;
			sf::Vector2f pos(field_size.x/rock_count.x*(float)x,field_size.y/rock_count.y*(float)y);
			pos-=size*0.5f;
			pos.x=std::floor(pos.x);
			pos.y=std::floor(pos.y);

			TerrainIsland* island=new TerrainIsland(BoundingBox(pos,pos+size));
			add_island(island);
		}
	}

	/*
	sf::Vector2f pos(500,0);
	sf::Vector2f size(500,500);
	add_island(new TerrainIsland(BoundingBox(pos,pos+size)));
	*/


	/*
	for(float x=0;x<1000;x+=100) {
		for(float y=0;y<field_size.y;y+=100) {
			sf::Vector2f size(250,250);
			sf::Vector2f pos=sf::Vector2f(x,y);

			TerrainIsland* island=new TerrainIsland(BoundingBox(pos,pos+size));
			add_island(island);
		}
	}
	*/

}
void Terrain::update_visual(sf::FloatRect rect) {

	sf::Vector2f offset;

	while(rect.left>field_size.x) {
		rect.left-=field_size.x;
		offset.x+=field_size.x;
	}
	while(rect.left<0) {
		rect.left+=field_size.x;
		offset.x-=field_size.x;
	}
	while(rect.top>field_size.y) {
		rect.top-=field_size.y;
		offset.y+=field_size.y;
	}
	while(rect.top<0) {
		rect.top+=field_size.y;
		offset.y-=field_size.y;
	}


	for(int i=0;i<islands.size();i++) {
		sf::FloatRect r1=rect;

		islands[i]->offset=offset;

		if(islands[i]->box.end.x<rect.left) {
			islands[i]->offset.x+=field_size.x;
			r1.left-=field_size.x;
		}
		if(islands[i]->box.end.y<rect.top) {
			islands[i]->offset.y+=field_size.y;
			r1.top-=field_size.y;
		}

		r1.left-=islands[i]->box.start.x;
		r1.top-=islands[i]->box.start.y;

		islands[i]->update_visual(r1);
	}
}
void Terrain::damage_area(sf::FloatRect rect) {
	for(int i=0;i<islands.size();i++) {
		sf::FloatRect r1=rect;
		r1.left-=islands[i]->box.start.x+islands[i]->offset.x;
		r1.top-=islands[i]->box.start.y+islands[i]->offset.y;
		islands[i]->damage_area(r1);
	}
}
bool Terrain::check_collision(sf::FloatRect rect) {
	for(int i=0;i<islands.size();i++) {
		sf::FloatRect r1=rect;
		r1.left-=islands[i]->box.start.x+islands[i]->offset.x;
		r1.top-=islands[i]->box.start.y+islands[i]->offset.y;
		if(islands[i]->check_collision(r1)) {
			return true;
		}
	}
	return false;
}
bool Terrain::check_collision(sf::Vector2f pos) {
	for(int i=0;i<islands.size();i++) {
		if(islands[i]->check_collision(pos-islands[i]->box.start-islands[i]->offset)) {
			return true;
		}
	}
	return false;
}

