#ifndef _BGA_QUAD_H_
#define _BGA_QUAD_H_

#include <cmath>

#include <SFML/System/Vector2.hpp>

class Quad {
public:
	sf::Vector2f p1;
	sf::Vector2f p2;
	Quad() {}
	Quad(sf::Vector2f _p1,sf::Vector2f _p2) {
		p1=_p1;
		p2=_p2;
	}
	bool intersects(const Quad& q) const {
		return (p1.x<=q.p2.x && p2.x>=q.p1.x && p1.y<=q.p2.y && p2.y>=q.p1.y);
	}
	bool contains(const sf::Vector2f& v) const {
		return (v.x>=p1.x && v.x<=p2.x && v.y>=p1.y && v.y<=p2.y);
	}
	void translate(sf::Vector2f p) {
		p1+=p;
		p2+=p;
	}
	sf::Vector2f size() const {
		return p2-p1;
	}

	Quad mod(const Quad& area) const {
		sf::Vector2f area_size=area.size();

		Quad q2=*this;
		q2.p1.x=std::fmod((q2.p1.x-area.p1.x),(area_size.x))+area.p1.x;
		if(q2.p1.x<area.p1.x) {
			q2.p1.x+=area_size.x;
		}
		q2.p1.y=std::fmod((q2.p1.y-area.p1.y),(area_size.y))+area.p1.y;
		if(q2.p1.y<area.p1.y) {
			q2.p1.y+=area_size.y;
		}
		q2.p2=q2.p1+size();
		return q2;
	}
};

#endif
