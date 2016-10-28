#include "Framework.h"
#include "Loader.h"
#include "Menu.h"
#include "Node.h"
#include "Quad.h"
#include "Terrain.h"

/*
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
*/

class Test : public Menu {
public:

	Node laser;
	float time;

	Terrain terrain;

	std::vector<Node::Ptr> quads;

	void add_quad(const Quad& quad) {
		Node::Ptr n(new Node());
		n->type=Node::TYPE_SOLID;
		n->scale=quad.size();
		n->pos=quad.p1;
		n->color.set(0.5,0.5,1.0,1.0);
		node.add_child(n.get());
		quads.push_back(std::move(n));
	}

	Test() {
		time=0;

		laser.texture=Loader::get_texture("player ships/Assaulter/railgun.png");
		laser.texture.tex->setRepeated(true);
		laser.shader.shader=Loader::get_shader("shader/laser.frag");

		laser.pos=sf::Vector2f(100,100);
		laser.origin.x=laser.texture.get_size().x/2.0;
		laser.scale.y=1000;

		node.add_child(&terrain);

		for(int i=0;i<20;i++) {
			sf::Vector2f size=Utils::rand_vec(20,50);
			sf::Vector2f pos=Utils::rand_vec(10,1000);
			add_quad(Quad(pos,pos+size));
		}

		node.add_child(&laser);

	}
	virtual bool event_input(const MenuEvent& event) override {
		/*
		if(event.event.type==sf::Event::MouseMoved) {
			laser.pos=event.pos;
		}
		*/

		return false;
	}
	virtual void event_frame(float dt) {
		time+=dt;

		float move_speed=0.4;
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
			node.pos.x+=dt*move_speed;
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
			node.pos.x-=dt*move_speed;
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
			node.pos.y+=dt*move_speed;
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
			node.pos.y-=dt*move_speed;
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::Q)) {
			laser.rotation+=dt*0.1;
		}
		if(sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			laser.rotation-=dt*0.1;
		}

		terrain.update_visual(sf::FloatRect(-node.pos,size));

		laser.pos=pointer_pos;

		/*
		float laser_len=100;
		float query_len=laser_len*laser.texture.get_size().y;

		Terrain::RayQuery ray=terrain.query_ray(laser.pos,laser.pos+Utils::vec_for_angle_deg(laser.rotation+90,query_len));
		laser.scale.y=laser_len*ray.hit_position;

		if(ray.hit) {
			terrain.damage_ray(ray,0.1*dt);
		}
		*/


		float laser_len=1500;
		sf::Vector2f laser_p1=laser.pos;
		sf::Vector2f laser_p2=laser.pos+Utils::vec_for_angle_deg(laser.rotation+90,laser_len);

		float laser_hit_pos=1.0;

		for(const Node::Ptr& n : quads) {
			Quad quad(n->pos,n->pos+n->scale);
			float hit_pos=0;
			bool inside=false;
			if(Utils::line_quad_intersection(laser_p1,laser_p2,quad,hit_pos,inside)) {
				laser_hit_pos=std::min(laser_hit_pos,hit_pos);
				if(inside) {
					break;
				}
			}
		}


		laser.scale.y=laser_len*laser_hit_pos/laser.texture.get_size().y;


		laser.shader.set_param("time",time);
		laser.shader.set_param("y_scale",laser.scale.y);

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
