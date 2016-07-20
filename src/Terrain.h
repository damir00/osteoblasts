#ifndef _Terrain_H_
#define _Terrain_H_

#include <string.h>
#include <stdio.h>

#include <SFML/Graphics.hpp>

#include "Node.h"
#include "Utils.h"
#include "TerrainLoader.h"

class TerrainIslandPoint {
public:
	sf::Vector2f pos;
	float size;
	bool active;
};


class BoundingBox {
public:
	sf::Vector2f start;
	sf::Vector2f end;

	BoundingBox() {

	}
	BoundingBox(sf::Vector2f _start,sf::Vector2f _end) {
		start=_start;
		end=_end;
	}
	BoundingBox(sf::FloatRect rect) {
		start=sf::Vector2f(rect.left,rect.top);
		end=sf::Vector2f(rect.left+rect.width,rect.top+rect.height);
	}
	sf::Vector2f getSize() {
		return end-start;
	}
	bool intersects(BoundingBox b) {
		if(start.x>b.end.x || end.x<b.start.x ||
		   start.y>b.end.y || end.y<b.start.y) return false;
		return true;
	}
};
class IntBoundingBox {
public:
	sf::Vector2i start;
	sf::Vector2i end;

	IntBoundingBox() {}
	IntBoundingBox(sf::IntRect rect) {
		start=sf::Vector2i(rect.left,rect.top);
		end=sf::Vector2i(rect.left+rect.width,rect.top+rect.height);
	}
	IntBoundingBox(sf::Vector2i _start,sf::Vector2i _end) {
		start=_start;
		end=_end;
	}
};

template<class T>
class ChunkGrid {
	float mult;
public:
	sf::Vector2u size;
	T* chunks;
	int chunk_size;

	ChunkGrid() {
		chunks=NULL;
	}
	void for_size(sf::Vector2f _size,int _chunk_size) {
		chunk_size=_chunk_size;
		mult=1.0/(float)chunk_size;
		size.x=std::ceil(_size.x*mult);
		size.y=std::ceil(_size.y*mult);
		chunks=new T[size.x*size.y];
	}
	void unload() {
		size=sf::Vector2u(0,0);
		delete(chunks);
		chunks=NULL;
		mult=0.0;
	}
	int get_index(sf::Vector2f pos) {
		sf::Vector2i p(std::floor(pos.x*mult),std::floor(pos.y*mult));
		if(p.x<0 || p.y<0 || p.x>=size.x || p.y>=size.y) return 0;
		return p.y*size.x+p.x;

	}
	IntBoundingBox intersect(sf::FloatRect r) {
		sf::Vector2i p1(
				Utils::clampi(0,size.x-1,std::floor(r.left*mult)),
				Utils::clampi(0,size.y-1,std::floor(r.top*mult)));
		sf::Vector2i p2(
				Utils::clampi(0,size.x-1,std::ceil((r.left+r.width)*mult)),
				Utils::clampi(0,size.y-1,std::ceil((r.top+r.height)*mult)));

		return IntBoundingBox(p1,p2);
	}
	T get(sf::Vector2f pos) {
		return chunks[get_index(pos)];
	}
	void set_all(T value) {
		if(!chunks) return;
		for(int i=0;i<size.x*size.y;i++) {
			chunks[i]=value;
		}
	}

};


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
	void from_sfimage(sf::Image* image) {
		delete(data);

		size=image->getSize();
		data_size=size.x*size.y*4;
		data=new sf::Uint8[data_size];
		memcpy(data,image->getPixelsPtr(),data_size);
	}
	void create(int width,int height) {
		delete(data);

		size=sf::Vector2u(width,height);
		data_size=size.x*size.y*4;
		data=new sf::Uint8[data_size];
		memset(data,0,data_size);
	}


};

class TerrainIsland : public Node {

	ImageData terrain_texture;
	ImageData result;
	ImageData result_temp;

	static ImageDataFloat* dist_map;
	static ImageDataFloat* noise_map;

	sf::Texture* gpu_texture;
	//sf::Sprite sprite;

	int w;
	int h;
	int cell_size;
	TerrainIslandPoint *map;

	int load_chunk_size;
	ChunkGrid<bool> load_chunks;
	bool loaded;

	sf::Mutex load_mutex;

	sf::Mutex texture_mutex;
	std::vector<sf::IntRect> texture_updates;

	sf::Vector2i noise_offset;


	float image_get_light(const ImageData& data,int px,int py,int radius);

	//rect is in cell coordinate system
	void update_area_cell(sf::IntRect rect);

	void update_texture(sf::IntRect rect);

public:
	static TerrainLoader* loader;

	//rect is in texture coordinate system
	void update_area(sf::IntRect rect);

	BoundingBox box;
	sf::Vector2f offset;

	TerrainIsland(BoundingBox _box);
	void update_visual(sf::FloatRect rect);

	void init();
	void load();
	void unload();
	void damage_area(sf::FloatRect rect);
	bool check_collision(sf::FloatRect rect);
	bool check_collision(sf::Vector2f pos);
};


class Terrain : public Node {
	std::vector<TerrainIsland*> islands;
	sf::Vector2f field_size;

	void add_island(TerrainIsland* island);

public:
	Terrain();
	void update_visual(sf::FloatRect rect);
	void damage_area(sf::FloatRect rect);
	bool check_collision(sf::FloatRect rect);
	bool check_collision(sf::Vector2f vect);

};

#endif
