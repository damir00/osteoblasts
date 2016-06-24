#ifndef _Node_H_
#define _Node_H_

#include <SFML/Graphics.hpp>
#include <tr1/unordered_map>
#include <string>
#include <stdio.h>

class NodeShader {
	std::tr1::unordered_map<std::string,float> param_float;
public:
	sf::Shader* shader;

	NodeShader();
	NodeShader(sf::Shader* _shader);
	void set_param(const std::string& name,float p);
	void applyParams();
};

class NodeColor {
public:
	float r,g,b,a;
	NodeColor() {
		r=g=b=a=1.0;
	}
	NodeColor(float _r,float _g,float _b,float _a) {
		r=_r;
		g=_g;
		b=_b;
		a=_a;
	}
	NodeColor operator*(const NodeColor& c) {
		return NodeColor(r*c.r,g*c.g,b*c.b,a*c.a);
	}
	void operator*=(const NodeColor& c) {
		r*=c.r;
		g*=c.g;
		b*=c.b;
		a*=c.a;
	}
	sf::Color sfColor() const {
		return sf::Color(r*255.0,g*255.0,b*255.0,a*255.0);
	}
};
class NodeState {
public:
	sf::RenderTarget* render_target;
	sf::RenderStates render_state;
	NodeColor color;
	NodeState() {
		render_target=NULL;
	}
	NodeState(sf::RenderTarget& target) {
		render_target=&target;
	}
};

class Node {

private:
	NodeState combine_render_state(const NodeState& state);

public:
	Node* parent;
	std::vector<Node*> childs;
	//sf::Transform transform;
	sf::RenderStates render_state;
	NodeColor color;
	bool visible;

	//sf::Shader* shader;
	NodeShader shader;

	Node();
	virtual ~Node() {}

	virtual sf::Transform get_transform() { return render_state.transform; }

	void render(const NodeState& state);
	virtual void draw(const NodeState& state) {}

	void event_mouse_moved(sf::Event event);
	void event_mouse_clicked(sf::Event event,bool pressed);

	virtual void mouse_moved(sf::Event event) {}
	virtual void mouse_clicked(sf::Event event,bool pressed) {}

	void add_child(Node* node);
	void remove_child(Node* node);
	void remove_child(int i);
	void remove();
};

class NodeContainer : public Node {
public:
	sf::Drawable* item;

	NodeContainer() {
		item=NULL;
	}
	NodeContainer(sf::Drawable* _item) {
		item=_item;
	}
	void draw(const NodeState& state) {
		if(item) {
			state.render_target->draw(*item,state.render_state);
		}
	}
};
class NodeSprite : public Node {
public:
	sf::Sprite sprite;
	sf::Vector2f pos;
	sf::Vector2f scale;
	float angle;	//rad
	NodeSprite() {
		angle=0;
		scale=sf::Vector2f(1,1);
	}

	sf::Transform get_transform() {
		sf::Transform tr(sf::Transform::Identity);
		tr.translate(pos);
		tr.scale(scale);
		tr.rotate(angle);
		return tr;
	}

	void draw(const NodeState& state) {
		sprite.setColor(state.color.sfColor());
		state.render_target->draw(sprite,state.render_state);
	}
};

#endif

