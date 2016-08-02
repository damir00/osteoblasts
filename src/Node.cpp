#include "Node.h"
#include "Utils.h"

void Node::add_child(Node* node) {
	children.push_back(node);
}
void Node::add_child(Node* node,int pos) {
	children.insert(children.begin()+pos,node);
}
bool Node::remove_child(Node* node) {
	return Utils::vector_remove(children,node);
}
void Node::clear_children() {
	children.clear();
}

