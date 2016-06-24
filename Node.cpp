#include "Node.h"

#include <stdio.h>
#include "Utils.h"

NodeShader::NodeShader() {
	shader=NULL;
}
NodeShader::NodeShader(sf::Shader* _shader) {
	shader=_shader;
}
void NodeShader::set_param(const std::string& name,float p) {
	param_float[name]=p;
}
void NodeShader::applyParams() {
	if(!shader) {
		return;
	}
	for(std::tr1::unordered_map<std::string,float>::iterator it=param_float.begin();it!=param_float.end();it++) {
		shader->setParameter(it->first,it->second);
	}
}


Node::Node() {
	visible=true;
	parent=NULL;
//	shader=NULL;
}
NodeState Node::combine_render_state(const NodeState& state) {
	NodeState new_state=state;
	new_state.render_state.transform=state.render_state.transform*get_transform();

	//s.shader=state.shader ? state.shader : render_state.shader;

	if(shader.shader) {
		new_state.render_state.shader=shader.shader;
		shader.applyParams();
	}
	else {
		new_state.render_state.shader=render_state.shader;
	}

	new_state.color*=color;

	return new_state;
}
void Node::render(const NodeState& state) {
	if(!visible) return;

	//sf::Transform combinedTransform=parentTransform*transform;
	NodeState mystate=combine_render_state(state);
	draw(mystate);

	for (std::size_t i = 0; i < childs.size(); ++i) {
		childs[i]->render(mystate);
	}
}
void Node::event_mouse_moved(sf::Event event) {
	mouse_moved(event);
	for(std::size_t i=0;i<childs.size();i++) childs[i]->event_mouse_moved(event);
}
void Node::event_mouse_clicked(sf::Event event,bool pressed) {
	mouse_clicked(event,pressed);
	for(std::size_t i=0;i<childs.size();i++) childs[i]->event_mouse_clicked(event,pressed);
}

void Node::add_child(Node* node) {
	if(node->parent) {
		printf("WARN: node already has parent!\n");
	}
	node->parent=this;
	childs.push_back(node);
}
void Node::remove_child(Node* node) {
	if(Utils::vector_remove(childs,node)) {
		node->parent=NULL;
	}
	/*
	for(int i=0;i<childs.size();i++) {
		if(childs[i]==node) {
			node->parent=NULL;
			childs.erase(childs.begin()+i);
			return;
		}
	}
	*/
	else {
		printf("WARN: node not a child!\n");
	}
}
void Node::remove_child(int i) {
	childs[i]->parent=NULL;
	childs.erase(childs.begin()+i);
}
void Node::remove() {
	if(parent) {
		parent->remove_child(this);
	}
}
