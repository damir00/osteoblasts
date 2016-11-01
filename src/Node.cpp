#include "Node.h"
#include "Utils.h"

void Node::add_child(Node* node) {
	children.push_back(node);
	node->dbg_parent=this;
}
void Node::add_child(Node* node,int pos) {
	children.insert(children.begin()+pos,node);
	node->dbg_parent=this;
}
bool Node::remove_child(Node* node) {
	if(!node) {
		return false;
	}
	node->dbg_parent=nullptr;
	return Utils::vector_remove(children,node);
}
void Node::clear_children() {
	children.clear();
}

