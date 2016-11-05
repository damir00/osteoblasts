#ifndef _BGA_NODE_H_
#define _BGA_NODE_H_

#include <signal.h>

#include <memory>
#include <vector>
#include <string>
#include <unordered_map>

#include "Quad.h"
#include "Texture.h"

class Color {
public:
	float r,g,b,a;
	Color() {
		r=g=b=a=1.0;
	}
	Color(float _r,float _g,float _b,float _a) {
		set(_r,_g,_b,_a);
	}
	Color(sf::Color c) {
		set((float)c.r/255.0,(float)c.g/255.0,(float)c.b/255.0,(float)c.a/255.0);
	}
	Color(int hex) {
		set((float)((hex & 0x00ff0000)>>16)/255.0,
			(float)((hex & 0x0000ff00)>>8)/255.0,
			(float)(hex & 0x000000ff)/255.0,
			(float)((hex & 0xff000000)>>24)/255.0);
	}

	Color operator*(const Color& c) {
		return Color(r*c.r,g*c.g,b*c.b,a*c.a);
	}
	void set(float _r,float _g,float _b,float _a) {
		r=_r;
		g=_g;
		b=_b;
		a=_a;
	}
	void operator*=(const Color& c) {
		r*=c.r;
		g*=c.g;
		b*=c.b;
		a*=c.a;
	}
	void operator+=(const Color& c) {
		r+=c.r;
		g+=c.g;
		b+=c.b;
		a+=c.a;
	}
	Color lerp(Color c,float x) {
		return Color(
				Utils::lerp(r,c.r,x),
				Utils::lerp(g,c.g,x),
				Utils::lerp(b,c.b,x),
				Utils::lerp(a,c.a,x));
	}
	sf::Color sfColor() const {
		return sf::Color(r*255.0,g*255.0,b*255.0,a*255.0);
	}

	static Color rand() {
		return Color(
				Utils::rand_range(0,1),
				Utils::rand_range(0,1),
				Utils::rand_range(0,1),
				1.0);
	}
};

class NodeShader {
	std::unordered_map<std::string,float> param_float;
	std::unordered_map<std::string,sf::Vector2f> param_vec2;
public:
	sf::Shader* shader;

	NodeShader() {
		shader=NULL;
	}
	NodeShader(sf::Shader* _shader) {
		shader=_shader;
	}
	void set_param(const std::string& name,float p) {
		param_float[name]=p;
	}
	void set_param(const std::string& name,const sf::Vector2f& p) {
		param_vec2[name]=p;
	}
	void applyParams() {
		if(!shader) {
			return;
		}
		//for(std::unordered_map<std::string,float>::iterator it=param_float.begin();it!=param_float.end();it++) {
		for(/*std::unordered_map<std::string,float>::iterator*/auto it : param_float) {
			shader->setParameter(it.first,it.second);
		}
		for(auto it : param_vec2) {
			shader->setParameter(it.first,it.second);
		}
	}
};

class Node {
public:
	bool visible;
	sf::Vector2f pos;
	sf::Vector2f scale;
	float rotation;		//degrees
	sf::Vector2f origin;	//rotation center + pos offset

	std::vector<Node*> children;

	bool clip_enabled;
	Quad clip_quad;

	Color color;
	Color color_add;
	NodeShader shader;
	std::vector<NodeShader> post_process_shaders;

	enum Type {
		TYPE_NO_RENDER,
		TYPE_TEXTURE,		//textured sprite
		TYPE_SOLID,			//solid color, scale=size
		TYPE_TEXT
	};
	Type type;

	//type-specific parameters, TODO move it out
	Texture texture;
	sf::Text text;

	Node() {
		visible=true;
		clip_enabled=false;
		type=TYPE_TEXTURE;
		scale=sf::Vector2f(1,1);
		rotation=0;
		color_add.set(0,0,0,0);

		dbg_parent=nullptr;
		dbg_is_root=false;
	}
	virtual ~Node() {
		Node* p=dbg_parent;
		while(p) {
			if(p->dbg_is_root) {
				//raise(SIGSEGV);
				printf("ERROR: unsafe node free!!\n");
			}
			p=p->dbg_parent;
		}
	}

	Node* dbg_parent;
	bool dbg_is_root;

	void set_origin_center() {
		origin=texture.get_size()*0.5f;
	}
	void set_origin_text_center_x() {
		origin=sf::Vector2f(text.getLocalBounds().width*0.5f,0.0f);
	}

	void add_child(Node* node);
	void add_child(Node* node,int pos);
	bool remove_child(Node* node);
	void clear_children();

	typedef std::unique_ptr<Node> Ptr;
};

#endif
