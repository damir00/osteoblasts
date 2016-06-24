#include "Utils.h"

#include <math.h>
#include <cmath>

#include "Game.h"
#include "Audio.h"

sf::Color Utils::hex_to_color(int hex) {
	return sf::Color(
		(hex & 0x00ff0000)>>16,
		(hex & 0x0000ff00)>>8,
		hex & 0x000000ff,
		(hex & 0xff000000)>>24
	);
}
float Utils::rand_float() {
	return (float)rand()/RAND_MAX;
}
float Utils::rand_range(float low,float high) {
	return low+rand_float()*(high-low);
}
int Utils::rand_range_i(int low,int high) {
	return low+rand()%(high-low+1);
}
float Utils::minf(float x1,float x2) {
	if(x1<x2) return x1;
	return x2;
}
float Utils::maxf(float x1,float x2) {
	if(x1>x2) return x1;
	return x2;
}
int Utils::clampi(int low,int high,int val) {
	if(val<low) return low;
	if(val>high) return high;
	return val;
}
float Utils::clamp(float low,float high,float val) {
	if(val<low) return low;
	if(val>high) return high;
	return val;
}
float Utils::lerp(float p1,float p2,float x) {
	return p1+(p2-p1)*x;
}
float Utils::dist(float dx,float dy) {
	return sqrt(dx*dx+dy*dy);
}
float Utils::dist_fast(float dx,float dy) {
	return dx*dx+dy*dy;
}

float Utils::vec_len(sf::Vector2f vec) {
	return dist(vec.x,vec.y);
}
float Utils::vec_len_fast(sf::Vector2f vec) {
	return dist_fast(vec.x,vec.y);
}
sf::Vector2f Utils::rand_angle_vec() {
	return angle_vec(rand_range(0,3.14*2));
}
sf::Vector2f Utils::rand_dist_vec(float max_range) {
	float r=rand_range(0,max_range);
	float a=rand_range(0,M_PI_2);
	return sf::Vector2f(cos(a)*r,sin(a)*r);
}
sf::Vector2f Utils::angle_vec(float angle) {
	return sf::Vector2f(cos(angle),sin(angle));
}
float Utils::num_move_towards(float start,float end,float limit) {
	if(end>start) {
		start+=limit;
		if(start>end) return end;
		return start;
	}
	start-=limit;
	if(start<end) return end;
	return start;
}
float Utils::angle_move_towards(float start,float end,float limit) {
	float delta=end-start;
	if(delta>0) {
		if(delta<M_PI) return start+limit;
		return start-limit;
	}
	if(delta<-M_PI) return start+limit;
	return start-limit;
}
float Utils::num_wrap(float x,float mod) {
	return x-mod*std::floor(x/mod);
}
float Utils::angle_normalize(float angle) {
	return num_wrap(angle,2.0f*M_PI);
}

float Utils::vec_angle(sf::Vector2f vec) {
	return std::atan2(vec.y,vec.x)+M_PI;
}

sf::Vector2f Utils::vecf_sum(sf::Vector2f v1,sf::Vector2f v2) {
	return sf::Vector2f(v1.x+v2.x,v1.y+v2.y);
}
sf::Vector2i Utils::vec_to_i(sf::Vector2f v) {
	return sf::Vector2i(v.x,v.y);
}
sf::Vector2f Utils::vec_to_f(sf::Vector2i v) {
	return sf::Vector2f(v.x,v.y);
}
sf::Vector2f Utils::vec_to_f(sf::Vector2u v) {
	return sf::Vector2f(v.x,v.y);
}
sf::FloatRect Utils::vec_center_rect(sf::Vector2f center,sf::Vector2f size) {
	return sf::FloatRect(center.x-size.x*0.5f,center.y-size.y*0.5f,size.x,size.y);
}
sf::Vector2f Utils::vec_normalize(sf::Vector2f v) {
	return v/vec_len(v);
}
sf::Vector2f Utils::vec_cap(sf::Vector2f v,float x) {
	float len=vec_len(v);
	if(len>x) {
		return v/len*x;
	}
	return v;
}
float Utils::vec_dot(const sf::Vector2f& v1,const sf::Vector2f& v2) {
    return (v1.x*v2.x + v1.y*v2.y);
}
sf::Vector2f Utils::sprite_size(const sf::Sprite& sprite) {
	if(sprite.getTexture()==NULL) {
		return sf::Vector2f(0,0);
	}
	return vec_to_f(sprite.getTexture()->getSize());
}


float largest_root_of_quadratic_equation(float A,float B,float C){
  return (B+std::sqrt(B*B-4*A*C))/(2*A);
}
sf::Vector2f Utils::target_lead(sf::Vector2f target_pos,sf::Vector2f target_vel,float bullet_vel) {
	  float a = bullet_vel*bullet_vel - Utils::vec_dot(target_vel,target_vel);
	  float b = -2.0f*Utils::vec_dot(target_vel,target_pos);
	  float c = -Utils::vec_dot(target_pos,target_pos);
	  sf::Vector2f p=target_pos+largest_root_of_quadratic_equation(a,b,c)*target_vel;
	  return Utils::vec_normalize(p)*bullet_vel;
}

std::string Utils::to_string(float x) {
	char buff[100];
	sprintf(buff,"%f",x);
	return buff;
}
std::string Utils::file_swap_extension(std::string filename,std::string extension) {
	size_t p=filename.rfind(".");
	if(p==std::string::npos) return filename;
	filename.erase(p+1);
	return filename+extension;
}

GameFramework* Utils::init_framework(int window_w,int window_h/*,std::string level*/) {
	GameFramework *game=new GameFramework();
	game->create_window(window_w,window_h);
	GameRes::init();
	Audio::init();
	game->init_resources();
	game->game->init();
	return game;
}

