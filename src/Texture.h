#ifndef _BGA_TEXTURE_H_
#define _BGA_TEXTURE_H_

#include <SFML/Graphics.hpp>

#include "Utils.h"

class Texture {
public:
	sf::Texture* tex;
	sf::IntRect rect;

	Texture() {
		tex=NULL;
	}
	Texture(sf::Texture* _tex) {
		tex=_tex;
		if(tex) {
			rect.left=0;
			rect.top=0;
			rect.width=tex->getSize().x;
			rect.height=tex->getSize().y;
		}
	}

	sf::Vector2f get_size() const {
		return sf::Vector2f(rect.width,rect.height);
		/*
		if(!tex) {
			return sf::Vector2f(0,0);
		}
		return Utils::vec_to_f(tex->getSize());
		*/
	}
};

#endif
