#include "Framework.h"
#include "Loader.h"
#include "Menu.h"
#include "Node.h"
#include "Quad.h"


class Test : public Menu {
public:

	sf::Vector2f item_size;
	Quad q_area;
	Quad q_item;

	Node n_area;
	Node n_item;
	Node n_item2;

	void set_node(Node& n,const Quad& q) {
		n.pos=q.p1;
		n.scale=(q.p2-q.p1);
	}

	Test() {
		q_area=Quad(sf::Vector2f(400,200),sf::Vector2f(500,300));
		item_size=sf::Vector2f(20,10);
		q_item=Quad(sf::Vector2f(0,0),item_size);

		n_area.type=Node::TYPE_SOLID;
		n_area.color.set(1,0,0,1);
		n_item.type=Node::TYPE_SOLID;
		n_item.color.set(0,0,1,1);
		n_item2.type=Node::TYPE_SOLID;
		n_item2.color.set(0,0,0.5,1);

		node.add_child(&n_area);
		node.add_child(&n_item2);
		node.add_child(&n_item);
	}

	virtual bool event_input(const MenuEvent& event) override {

		q_item.p1=pointer_pos;
		q_item.p2=pointer_pos+item_size;

		set_node(n_area,q_area);
		set_node(n_item,q_item);

		Quad q2=q_item.mod(q_area);

		set_node(n_item2,q2);

		return false;
	}
};


int main() {
	Loader::init();
	Framework f;
	Test test;
	f.get_root_menu()->add_child(&test);
	f.run();

	Loader::clear_all();

	return 0;
}
