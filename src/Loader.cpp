#include <unordered_map>
#include <stdio.h>
#include <sstream>

#include "Loader.h"

namespace {

Texture load_texture(const std::string& path) {
	sf::Texture* t=new sf::Texture();
	if(!t->loadFromFile(path)) {
		printf("can't load texture %s\n",path.c_str());
		delete(t);
		t=NULL;
	}
	return Texture(t);
}
void unload_texture(Texture texture) {
	if(texture.tex==NULL) {
		return;
	}
	delete(texture.tex);
}

sf::Image* load_image(const std::string& path) {
	sf::Image *img=new sf::Image();
	if(!img->loadFromFile(path)) {
		printf("can't load image %s\n",path.c_str());
		delete(img);
		img=NULL;
	}
	return img;
}
void unload_image(sf::Image* image) {
	delete(image);
}

sf::Font* load_font(const std::string& path) {
	sf::Font* f=new sf::Font();
	if(!f->loadFromFile(path)) {
		printf("can't load font %s\n",path.c_str());
		delete(f);
		return NULL;
	}
	return f;
}
void unload_font(sf::Font* font) {
	delete(font);
}
sf::Shader* load_shader(const std::string& path) {
	sf::Shader* s=new sf::Shader();
	if(!s->loadFromFile(path,sf::Shader::Fragment)) {
		printf("can't load font %s\n",path.c_str());
		delete(s);
		return NULL;
	}
	s->setParameter("texture", sf::Shader::CurrentTexture);
	return s;
}
void unload_shader(sf::Shader* shader) {
	delete(shader);
}

template <typename K,typename V>
class Cache {
public:
	typedef V (*Fetch)(const K&);
	typedef void (*Delete)(V);

private:
	std::unordered_map<K,V> items;
	Fetch f_fetch;
	Delete f_delete;

public:
	Cache(Fetch _fetch,Delete _delete) {
		f_fetch=_fetch;
		f_delete=_delete;
	}
	~Cache() {
		clear();
	}
	V get(K key) {
		if(items.count(key)>0) {
			return items[key];
		}
		V val=f_fetch(key);
		items[key]=val;
		return val;
	}
	void remove(K key) {
		if(items.count(key)==0) {
			return;
		}
		f_delete(key);
		items.erase(key);
	}
	void clear() {
		for(auto it : items) {
			f_delete(it.second);
		}
		items.clear();
	}
};
}

Loader::Impl* Loader::impl=NULL;

class Loader::Impl {
public:
	Cache<std::string,Texture> cache_textures;
	Cache<std::string,sf::Image*> cache_images;
	Cache<std::string,sf::Font*> cache_fonts;
	Cache<std::string,sf::Shader*> cache_shaders;

	Impl() :
		cache_textures(load_texture,unload_texture),
		cache_images(load_image,unload_image),
		cache_fonts(load_font,unload_font),
		cache_shaders(load_shader,unload_shader)
	{
	}
	~Impl() {}

	void clear_all() {
		cache_textures.clear();
		cache_images.clear();
		cache_fonts.clear();
		cache_shaders.clear();
	}

	Texture get_texture(const std::string& path) {
		return cache_textures.get("assets/"+path);
	}
	std::vector<Texture> get_texture_sequence(const std::string &path,int count) {
		std::vector<Texture> seq;
		for(int i=1;i<=count;i++) {
			std::ostringstream s;
			s<<path<<"_"<<i<<".png";
			seq.push_back(get_texture(s.str()));
		}
		return seq;
	}

	sf::Image* get_image(const std::string& path) {
		return cache_images.get("assets/"+path);
	}
	sf::Font* get_font(const std::string& path) {
		return cache_fonts.get("assets/"+path);
	}
	sf::Shader* get_shader(const std::string& path) {
		return cache_shaders.get("assets/"+path);
	}

};


void Loader::init() {
	if(impl) {
		return;
	}
	impl=new Impl();
}
Texture Loader::get_texture(const std::string& path) {
	return impl->get_texture(path);
}
std::vector<Texture> Loader::get_texture_sequence(const std::string &path,int count) {
	return impl->get_texture_sequence(path,count);
}
sf::Image* Loader::get_image(const std::string &path) {
	return impl->get_image(path);
}
sf::Font* Loader::get_font(const std::string &path) {
	return impl->get_font(path);
}
sf::Shader* Loader::get_shader(const std::string& path) {
	return impl->get_shader(path);
}
void Loader::clear_all() {
	impl->clear_all();
}

