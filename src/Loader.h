#ifndef _BGA_LOADER_H_
#define _BGA_LOADER_H_

#include <string>
#include <vector>

#include <SFML/Graphics.hpp>

#include "Utils.h"
#include "Texture.h"

class Loader {
public:
	class Impl;
private:
	static Impl* impl;
public:

	static void init();
	static Texture get_texture(const std::string &path);
	static std::vector<Texture> get_texture_sequence(const std::string &path,int count);

	static sf::Image* get_image(const std::string &path);

	static sf::Font* get_font(const std::string &path);
	static sf::Font* get_menu_font() { return get_font("font/BMjapan.TTF"); }
};

#endif
