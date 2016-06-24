#include "Anim.h"

#include <stdio.h>
#include <cmath>

#include "Utils.h"

SpriteAnim::SpriteAnim() {
	done=true;
	loop=false;
	reverse=false;
	anim=NULL;
	duration=1.0;
}
SpriteAnim::SpriteAnim(SpriteAnimData* _anim,bool _loop) {
	done=true;
	load(_anim,loop);
	anim=NULL;
}
void SpriteAnim::load(SpriteAnimData* _anim,bool _loop) {
	if(!_anim) return;
	anim=_anim;
	loop=_loop;
	sprite.setTexture(*anim->texture,true);
	sprite.setScale(anim->scale,anim->scale);

	if(anim->frames.size()>0) {
		sprite.setOrigin(anim->frames[0].width/2,anim->frames[0].height/2);
	}

	duration=anim->delay*(anim->frame_end-anim->frame_start);

	pos=0;
	if(duration==0.0f) {
		duration=1.0f;
	}
	prev_frame=-1;
	set_frame(0);
}
void SpriteAnim::update(float delta) {
	if(!anim) return;
	/*
	if(loop && done) {
		set_frame(0);
		return;
	}
	*/

	done=false;

	if(loop) {
		if(reverse) {
			delta=-delta;
		}
		pos=Utils::num_wrap(pos+delta,duration);
	}
	else {
		if(reverse) {
			pos-=delta;
			if(pos<0) {
				pos=0;
				done=true;
			}
		}
		else {
			pos+=delta;
			if(pos>duration) {
				pos=duration;
				done=true;
			}
		}
	}


	int t_frame=std::floor(pos/anim->delay);
	update_to_frame(t_frame);
}
void SpriteAnim::update_to_frame(int frame) {
	frame+=anim->frame_start;
	if(frame<anim->frame_start || frame>=anim->frame_end) {
		return;
	}
	if(frame!=prev_frame) {
		sprite.setTextureRect(anim->frames[frame]);
		prev_frame=frame;
	}
}
void SpriteAnim::set_frame(int frame) {
	if(!anim) return;
	pos=frame*anim->delay;
	update_to_frame(frame);
	done=false;
}
void SpriteAnim::set_progress(float progress) {
	set_frame(progress*duration/anim->delay);
}
void SpriteAnim::set_offset(sf::Vector2f offset) {
	sprite.setOrigin(offset);
}
int SpriteAnim::get_frame() {
	return prev_frame;
}

void SpriteAnim::draw(const NodeState& state) {
	state.render_target->draw(sprite,state.render_state);
}

bool SpriteAnim::loaded() {
	return (anim!=NULL);
}
sf::Vector2f SpriteAnim::get_size() {
	return sf::Vector2f(
			anim->frames[prev_frame].width,
			anim->frames[prev_frame].height);
}

Atlas::Atlas() {
	texture=NULL;
}
sf::Sprite Atlas::get_sprite(int i) {
	sf::Sprite sprite;
	if(texture) {
		sprite.setTexture(*texture);
	}
	sf::IntRect rect(items[i].map_pos.x,items[i].map_pos.y,items[i].size.x,items[i].size.y);
	sprite.setTextureRect(rect);
	return sprite;
}




