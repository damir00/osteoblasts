#ifndef _BGA_NODE_H_
#define _BGA_NODE_H_

#include <memory>

#include <vector>
#include <string>

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
};

class Node {
public:
	bool visible;
	sf::Vector2f pos;
	sf::Vector2f scale;
	float rotation;		//degrees
	sf::Vector2f origin;	//rotation center + pos offset

	std::vector<Node*> children;

	Color color;

	enum Type {
		TYPE_NO_RENDER,
		TYPE_TEXTURE,		//textured sprite,
		TYPE_SOLID,			//solid color, scale=size
		TYPE_TEXT
	};
	Type type;

	//type-specific parameters, TODO move it out
	Texture texture;
	sf::Text text;

	Node() {
		visible=true;
		type=TYPE_TEXTURE;
		scale=sf::Vector2f(1,1);
		rotation=0;
	}
	virtual ~Node() {}

	void set_origin_center() {
		origin=texture.get_size()*0.5f;
	}

	void add_child(Node* node);
	void add_child(Node* node,int pos);
	bool remove_child(Node* node);
	void clear_children();

	typedef std::unique_ptr<Node> Ptr;
};

#endif
