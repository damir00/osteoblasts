#ifndef _Terrain_H_
#define _Terrain_H_

#include <string.h>
#include <stdio.h>

#include <SFML/Graphics.hpp>

#include "Node.h"
#include "Utils.h"
#include "Quad.h"
#include "SimpleList.h"
#include "TerrainLoader.h"

class TerrainIslandPoint {
public:
	//sf::Vector2f pos;
	//float size;
	bool active;
	float health;

	TerrainIslandPoint() {
		active=false;
		health=0;
	}
};

/*
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
*/
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
		delete[] chunks;
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
		if(x<0 || y<0 || x>=(int)size.x || y>=(int)size.y) {
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
		delete[] data;
	}
	void unload() {
		data_size=0;
		delete[] data;
		data=NULL;
		size=sf::Vector2u(0,0);
	}
	void from_sfimage(sf::Image* image) {
		delete[] data;

		size=image->getSize();
		data_size=size.x*size.y*4;
		data=new sf::Uint8[data_size];
		memcpy(data,image->getPixelsPtr(),data_size);
	}
	void create(int width,int height) {
		delete[] data;
		size=sf::Vector2u(width,height);
		data_size=size.x*size.y*4;
		data=new sf::Uint8[data_size];
		memset(data,0,data_size);
	}
};

//simple static quadtree
template<class T>
class Octree {
public:
	std::vector<T> items;
	SimpleList<T> ret_list;
	Octree<T>* children[4];
	Quad quad;
	int max_depth;

	Quad quad_for_child(int i) {
		/*
		0 1
		2 3
		*/
		sf::Vector2f center=(quad.p1+quad.p2)*0.5f;
		if(i==0) {
			return Quad(quad.p1,center);
		}
		else if(i==1) {
			return Quad(sf::Vector2f(center.x,quad.p1.y),sf::Vector2f(quad.p2.x,center.y));
		}
		else if(i==2) {
			return Quad(sf::Vector2f(quad.p1.x,center.y),sf::Vector2f(center.x,quad.p2.y));
		}
		else {
			return Quad(center,quad.p2);
		}
	}

	static void list_put_unique(SimpleList<T>& list,T item) {
		for(int i=0;i<list.size();i++) {
			if(list[i]==item) {
				return;
			}
		}
		list.push_back(item);
	}

public:
	Octree() {
		for(int i=0;i<4;i++) {
			children[i]=NULL;
		}
		reset(Quad(sf::Vector2f(0,0),sf::Vector2f(0,0)),0);
	}
	~Octree() {
		for(int i=0;i<4;i++) {
			delete(children[i]);
		}
	}
	void reset(const Quad& _quad,int _max_depth) {
		for(int i=0;i<4;i++) {
			delete(children[i]);
			children[i]=NULL;
		}
		quad=_quad;
		max_depth=_max_depth;
	}
	void put(const Quad& item_quad,const T& item) {
		if(!item_quad.intersects(quad)) {
			printf("WARN: this trunk does not contain item quad\n");
			return;
		}

		if(max_depth<0) {
			printf("BUG: max_depth %d\n",max_depth);
			return;
		}

		if(max_depth==0) {
			items.push_back(item);
			return;
		}
		for(int i=0;i<4;i++) {
			if(!children[i]) {
				Quad child_quad=quad_for_child(i);
				if(child_quad.intersects(item_quad)) {
					children[i]=new Octree<T>();
					children[i]->reset(child_quad,max_depth-1);
					children[i]->put(item_quad,item);
				}
			}
			else {
				if(children[i]->quad.intersects(item_quad)) {
					children[i]->put(item_quad,item);
				}
			}
		}
	}

	void query_list(const Quad& q,SimpleList<T>& _ret) {
		if(!q.intersects(quad)) {
			return;
		}
		for(const T& item : items) {
			list_put_unique(_ret,item);
		}
		for(int i=0;i<4;i++) {
			if(children[i]) {
				children[i]->query_list(q,_ret);
			}
		}
	}
	SimpleList<T>& query(const Quad& q) {
		ret_list.clear();
		query_list(q,ret_list);
		return ret_list;
	}

	void query_list(const sf::Vector2f& point,SimpleList<T>& _ret) {
		if(!quad.contains(point)) {
			return;
		}
		for(const T& item : items) {
			list_put_unique(_ret,item);
		}
		for(int i=0;i<4;i++) {
			if(children[i]) {
				children[i]->query_list(point,_ret);
			}
		}
	}
	SimpleList<T>& query(const sf::Vector2f& point) {
		ret_list.clear();
		query_list(point,ret_list);
		return ret_list;
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
	TerrainIslandPoint *map;

	int load_chunk_size;
	ChunkGrid<bool> load_chunks;
	bool loaded;

	sf::Mutex load_mutex;

	sf::Mutex texture_mutex;
	std::vector<sf::IntRect> texture_updates;

	sf::Vector2i noise_offset;

	float terrain_health;

	float image_get_light(const ImageData& data,int px,int py,int radius);

	//rect is in cell coordinate system
	void update_area_cell(sf::IntRect rect);

	void update_texture(const sf::IntRect& rect);

public:

	class ChunkAddress {
	public:
		TerrainIsland* island;
		int chunk_index;
		ChunkAddress() {
			island=nullptr;
			chunk_index=-1;
		}
		bool valid() const {
			return (island && chunk_index>=0);
		}
	};

	int cell_size;
	uint32_t version_id;	//will get incremented every time the island is changed (by damage_area)
	static TerrainLoader* loader;

	//rect is in texture coordinate system
	void update_area(sf::IntRect rect);

	Quad box;
	sf::Vector2f box_size;
	sf::Vector2f offset;

	TerrainIsland(Quad _box);
	void update_visual(const sf::FloatRect& rect);

	void init();
	void load();
	void unload();
	void damage_area(const sf::FloatRect& rect,float damage);
	void damage_chunk(int index,float damage);
	bool check_collision(const sf::FloatRect& rect);
	bool check_collision(const sf::FloatRect& rect,sf::Vector2f& normal);
	bool check_collision(const sf::Vector2f& pos,ChunkAddress& chunk);

	void generate_icon_texture(sf::Texture* texture);	//texture needs to be proper size!
	Texture generate_icon_texture();
};

class Terrain : public Node {
	std::vector<TerrainIsland*> islands;

	Octree<TerrainIsland*> octree;
	SimpleList<TerrainIsland*> loaded_islands;

	void add_island(TerrainIsland* island);

	TerrainIsland* get_island_at_point(const sf::Vector2f& pos);

public:

	class RayQuery {
	public:

		bool hit;
		float hit_position;
		TerrainIsland::ChunkAddress chunk_address;

		RayQuery() {
			hit=false;
			hit_position=1.0f;
		}
	};

	sf::Vector2f field_size;

	Terrain();
	void update_visual(const sf::FloatRect& rect);
	void damage_area(const sf::FloatRect& rect,float damage);
	bool check_collision(const sf::FloatRect& rect);
	bool check_collision(const sf::FloatRect& rect,sf::Vector2f& normal);
	bool check_collision(sf::Vector2f pos,TerrainIsland::ChunkAddress& chunk);
	RayQuery query_ray(const sf::Vector2f& start,const sf::Vector2f& end);
	bool island_intersects(TerrainIsland* island,const Quad& quad);

	void damage_ray(const RayQuery& ray,float damage);

	SimpleList<TerrainIsland*>& list_islands(const Quad& _quad);
};

#endif
