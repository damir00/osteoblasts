#ifndef _BGA_UTILS_H_
#define _BGA_UTILS_H_

#include <stdlib.h>
#include <math.h>
#include <vector>
#include <cmath>

#include <SFML/System.hpp>
#include <SFML/Graphics.hpp>

#ifndef M_PI
	#define M_PI 3.14159265358979323846
#endif

class Utils {
public:
	static sf::Vector2i vec_to_i(const sf::Vector2f& v) { return sf::Vector2i(v.x,v.y); }
	static sf::Vector2i vec_to_i(const sf::Vector2u& v) { return sf::Vector2i(v.x,v.y); }
	static sf::Vector2f vec_to_f(const sf::Vector2i& v) { return sf::Vector2f(v.x,v.y); }
	static sf::Vector2f vec_to_f(const sf::Vector2u& v) { return sf::Vector2f(v.x,v.y); }
	static sf::Vector2f area_center(const sf::FloatRect& rect) {
		return sf::Vector2f(rect.left+rect.width*0.5,rect.top+rect.height*0.5);
	}
	static sf::Vector2f vec_normalize(const sf::Vector2f& vec) {
		float len=sqrtf(vec.x*vec.x+vec.y*vec.y);
		return sf::Vector2f(vec.x/len,vec.y/len);
	}
	static sf::Vector2f vec_for_angle_deg(float angle,float len=1.0f) {
		float r=deg_to_rad(angle);
		return sf::Vector2f(cos(r)*len,sin(r)*len);
	}
	static sf::Vector2f vec_for_angle(float angle,float len=1.0f) {
		return sf::Vector2f(cos(angle)*len,sin(angle)*len);
	}
	static float vec_length(const sf::Vector2f& v) { return sqrtf(v.x*v.x+v.y*v.y); }
	static float vec_length_fast(const sf::Vector2f& v) { return (v.x*v.x+v.y*v.y); }
	static sf::Vector2f vec_cap_length(const sf::Vector2f& v,float l_min,float l_max) {
		float len=vec_length_fast(v);
		if(len<l_min*l_min) {
			return v*(l_min/sqrtf(len));
		}
		if(len>l_max*l_max) {
			return v*(l_max/sqrtf(len));
		}
		return v;
	}
	static sf::Vector2f vec_lerp(const sf::Vector2f& v1,const sf::Vector2f& v2,float x) {
		return sf::Vector2f(v1.x+(v2.x-v1.x)*x,v1.y+(v2.y-v1.y)*x);
	}
	static sf::Vector2f vec_reflect(const sf::Vector2f& v,const sf::Vector2f& normal) {
		float dot=v.x*normal.x + v.y*normal.y;
		return v-normal*2.0f*dot;
	}

	static float rad_to_deg(float rad) { return rad*(180.0f/M_PI); }
	static float deg_to_rad(float deg) { return deg*(M_PI/180.0f); }

	static float angle_move_towards(float start,float end,float limit) {
		float delta=end-start;
		if(delta>0) {
			if(delta<M_PI) return start+limit;
			return start-limit;
		}
		if(delta<-M_PI) return start+limit;
		return start-limit;
	}
	static float num_wrap(float x,float mod) {
		return x-mod*std::floor(x/mod);
	}
	static float angle_normalize(float angle) {
		return num_wrap(angle,2.0f*M_PI);
	}
	static float vec_angle(sf::Vector2f vec) {
		return std::atan2(vec.y,vec.x)+M_PI;
	}


	static float dist(float dx,float dy) { return sqrtf(dx*dx+dy*dy); }
	static float dist_fast(float dx,float dy) { return dx*dx+dy*dy; }
	static float dist(const sf::Vector2f& v) { return sqrtf(v.x*v.x+v.y*v.y); }

	static float rand_float() { return (float)rand()/RAND_MAX; }
	static float rand_range(float low,float high) { return low+rand_float()*(high-low); }
	static int rand_range_i(int low,int high) { return low+rand()%(high-low+1); }
	static float rand_sign() { return (probability(0.5) ? 1 : -1); }
	static float probability(float x) { return (rand_float()<=x); }

	static sf::Vector2f rand_vec(float min,float max) {
		return sf::Vector2f(rand_range(min,max),rand_range(min,max));
	}


	static float lerp(float p1,float p2,float x) { return p1+(p2-p1)*x; }
	static int clampi(int low,int high,int val) {
		if(val<low) return low;
		if(val>high) return high;
		return val;
	}
	static float clamp(float low,float high,float val) {
		if(val<low) return low;
		if(val>high) return high;
		return val;
	}
	static float num_move_towards(float start,float end,float limit) {
		if(end>start) {
			start+=limit;
			if(start>end) return end;
			return start;
		}
		start-=limit;
		if(start<end) return end;
		return start;
	}

	template<class T>
	static T vector_rand(const std::vector<T>& vec) { return vec[rand()%vec.size()]; }
	template<class T>
	static std::vector<T> to_vec(T item) {
		std::vector<T> v;
		v.push_back(item);
		return v;
	}
	template<class T>
	static bool vector_remove(std::vector<T> &vec,T item) {
		for(int i=0;i<vec.size();i++) {
			if(vec[i]==item) {
				vec.erase(vec.begin()+i);
				return true;
			}
		}
		return false;
	}
	template<class T>
	static int vector_index_of(const std::vector<T> &vec,T item) {
		for(int i=0;i<vec.size();i++) {
			if(vec[i]==item) {
				return i;
			}
		}
		return -1;
	}
	template<class T>
	static bool vector_contains(const std::vector<T>& vec,T item) {
		return (vector_index_of(vec,item)!=-1);
	}
	//if size is smaller, resize to size, otherwise do nothing
	template<class T>
	static void vector_fit_size(std::vector<T>& vec,int size) {
		if(vec.size()<size) {
			vec.resize(size);
		}
	}


};

#endif
