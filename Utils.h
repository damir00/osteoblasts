#ifndef _Utils_H_
#define _Utils_H_

#include <SFML/Graphics.hpp>

#include <string>
#include <vector>
#include <stdio.h>
#include <math.h>

#define DEBUG(...) //fprintf(stderr,"%s:%d ",__FILE__,__LINE__); fprintf(stderr,__VA_ARGS__); fprintf(stderr,"\n"); fflush(stderr);
#define DEBUG_P() DEBUG("");


class GameFramework;

//counts time from reset
//and becomes ready when timeouts
class Timeout {
public:
	float start;
	float current;

	Timeout() {
		set(1);
	}
	bool ready() {
		return (current<=0.0f);
	}
	void reset() {
		current=start;
	}
	void frame(float delta) {
		if(current>0.0f) {
			current-=delta;
		}
	}
	void set(float max) {
		start=max;
		current=0.0f;
	}
	float get_progress() {
		return 1-current/start;
	}
};

//on-off with min rest time between switching
class StickySwitch {
	float switch_time;
	bool on_target;
public:
	float rest_on;
	float rest_off;
	bool on;

	StickySwitch() {
		init(true,0,0);
	}
	void init(bool _on,float _rest_on,float _rest_off) {
		on=_on;
		on_target=_on;
		rest_on=_rest_on;
		rest_off=_rest_off;
		switch_time=std::max(_rest_on,_rest_off);
	}

	void set(bool _on) {
		on_target=_on;
	}
	void set_force(bool _on) {
		on=_on;
		switch_time=0.0f;
	}
	void frame(float delta) {
		//if(switch_time<min_rest) {
		if( (on && switch_time<rest_off) || (!on && switch_time<rest_on)) {
			switch_time+=delta;
		}
		else if(on!=on_target) {
			on=on_target;
			switch_time=0.0f;
		}
	}
};

//timeout for bullet firing. supports bursts
class FireTimeout {
	int fire_count;
	int current_fire_count;
public:
	Timeout fire_cooldown;
	Timeout fire_delay;

	FireTimeout() {
		trigger=false;
	}

	bool trigger;

	void set(float burst_pause,float bullet_pause,int _fire_count) {
		fire_cooldown.set(burst_pause);
		fire_delay.set(bullet_pause);
		fire_count=_fire_count;
		current_fire_count=0;
	}

	void consume() {
		trigger=false;
	}

	//returns true if trigger
	bool frame(float delta) {
		if(trigger) {
			return true;
		}

		trigger=false;

		if(fire_cooldown.ready()) {
			if(fire_delay.ready()) {
				trigger=true;
				current_fire_count++;
				if(current_fire_count>=fire_count) {
					current_fire_count=0;
					fire_cooldown.reset();
				}
				else {
					fire_delay.reset();
				}
			}
			else {
				fire_delay.frame(delta);
			}
		}
		else {
			fire_cooldown.frame(delta);
		}

		return trigger;
	}
};


class Utils {
public:

	static float probability(float x) {
		return (rand_float()<=x);
	}

	static sf::Color hex_to_color(int hex);	//ARGB
	static float rand_float();
	static float rand_range(float low,float high);
	static int rand_range_i(int low,int high);
	static float minf(float x1,float x2);
	static float maxf(float x1,float x2);
	static int clampi(int low,int high,int val);
	static float clamp(float low,float high,float val);
	static float lerp(float p1,float p2,float x);
	static float dist(float dx,float dy);
	static float dist_fast(float dx,float dy);

	static float num_move_towards(float start,float end,float limit);
	static float angle_move_towards(float start,float end,float limit);
	static float angle_normalize(float angle);	//0-2*M_PI
	static float num_wrap(float x,float mod);

	static float vec_len(sf::Vector2f vec);
	static float vec_len_fast(sf::Vector2f vec);

	static sf::Vector2f vecf_sum(sf::Vector2f v1,sf::Vector2f v2);
	static sf::Vector2f angle_vec(float angle);
	static float vec_angle(sf::Vector2f vec);
	static sf::Vector2f rand_angle_vec();
	static sf::Vector2f rand_dist_vec(float max_range);

	static sf::Vector2i vec_to_i(sf::Vector2f v);
	static sf::Vector2f vec_to_f(sf::Vector2i v);
	static sf::Vector2f vec_to_f(sf::Vector2u v);
	static sf::FloatRect vec_center_rect(sf::Vector2f center,sf::Vector2f size);
	static sf::Vector2f vec_normalize(sf::Vector2f v);
	static sf::Vector2f vec_cap(sf::Vector2f v,float x);
	static float vec_dot(const sf::Vector2f& v1,const sf::Vector2f& v2);

	static sf::Vector2f sprite_size(const sf::Sprite& sprite);
	static sf::Vector2f target_lead(sf::Vector2f target_pos,sf::Vector2f target_vel,float bullet_vel);

	static float deg_to_rad(float deg) { return deg/180.0f*M_PI; }
	static float rad_to_deg(float rad) { return rad*180.0f/M_PI; }

	static std::string to_string(float x);
	static std::string file_swap_extension(std::string filename,std::string extension);

	static GameFramework* init_framework(int window_w,int window_h/*,std::string level*/);

	template<class T>
	static T vector_rand(const std::vector<T>& vec) {
		return vec[rand()%vec.size()];
	}

	template<class T>
	static bool vector_remove(std::vector<T>& vec,T val) {
		for(int i=0;i<vec.size();i++) {
			if(vec[i]==val) {
				vec.erase(vec.begin()+i);
				return true;
			}
		}
		return false;
	}
};

template<class T>
class Cache {
	T (*allocator)();

	std::vector<T> cache;
public:
	Cache( T(*_allocator)() ) {
		allocator=_allocator;
	}
	T get() {
		if(cache.size()>0) {
			T item=cache.back();
			cache.pop_back();
			return item;
		}
		return allocator();
	}
	void put(T item) {
		cache.push_back(item);
	}
};

//traces current to target
class Tracer {
public:
	float current;
	float target;
	bool idle;		//idle
	bool changed;	//changed since last frame

	Tracer(float start=0) {
		current=target=start;
		idle=true;
		changed=true;
	}
	void move_to(float x) {
		target=x;
		idle=(current==target);
	}
	void set(float x) {
		current=target=x;
		idle=true;
		changed=true;
	}
	void frame(float delta) {
		float prev=current;
		current=Utils::num_move_towards(current,target,delta);
		idle=(current==target);
		changed=(prev!=current);
	}
};


class Range {
public:
	float low;
	float high;

	Range() {
		set_fixed(0);
	}
	Range(float x) {
		set_fixed(x);
	}
	Range(float _low,float _high) {
		set(_low,_high);
	}
	void set(float _low,float _high) {
		low=_low;
		high=_high;
	}
	void set_fixed(float value) {
		low=value;
		high=value;
	}
	float get_value() {
		return Utils::rand_range(low,high);
	}
	int get_value_int() {
		return Utils::rand_range_i(low,high);
	}
	void lock() {
		set_fixed(get_value());
	}
};
class RangeLocked : public Range {
public:
	bool locked;
	RangeLocked() {
		locked=false;
	}
	RangeLocked(float _low,float _high,bool _locked=false) : Range(_low,_high) {
		locked=_locked;
	}
	void check_lock() {
		if(locked) {
			lock();
		}
	}
};

class SpawnWave {
public:

	enum Pattern { RANDOM };

	Range start;
	Range ship_count;
	RangeLocked mult_enemy_speed;
	RangeLocked mult_enemy_hp;
	RangeLocked mult_enemy_fire_speed;
	RangeLocked mult_enemy_damage;
	RangeLocked spawn_delay;
	std::vector<std::string> ships;
	std::vector<std::string> groups;
	Pattern pattern;
};

class SpawnSegment {
public:
	long start_ts;
	long end_ts;
	std::vector<SpawnWave> waves;
};

class SpawnData {
public:
	std::vector<SpawnSegment> segments;
};


class GameConf {
public:
	SpawnData segments;
};



#endif
