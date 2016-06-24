#include "Ship.h"
#include "Game.h"



Bullet::Bullet() {
	size=0;
	type=0;
	//shape.setFillColor(sf::Color::White);
	life=2*1000;

	sprite.setTexture(*GameRes::projectile,true);
	target=NULL;

	is_point=true;
	damage=10;

	on_top=false;
	explode_on_death=false;
}
void Bullet::autorotate() {
	sprite.setRotation(Utils::vec_angle(vel)*180.0f/M_PI-90.0f);
}
void Bullet::update(float delta) {
	pos+=vel*(float)delta;
	life-=delta;
}
void Bullet::draw(const NodeState& state) {
	sf::RenderStates s=state.render_state;
	s.transform.translate(pos);
	state.render_target->draw(sprite,s);
}
sf::FloatRect Bullet::get_bb() {
	float size2=size*0.5f;
	return sf::FloatRect(pos.x-size2,pos.y-size2,size,size);
}
void Bullet::check_ship(Ship* s,float delta) {
	s->damage(damage);
	life=0;
	game->add_explosion(pos,false);
}

class BulletRocket : public Bullet {

public:

	float rot_speed;
	float velocity;
	float max_velocity;
	SpriteAnim anim_fire;

	float angle;

	BulletRocket() {
		sprite.setTexture(*Utils::vector_rand(GameRes::rockets),true);

		angle=0;
		rot_speed=M_PI*0.001;
		velocity=0.2f;
		max_velocity=0.75f;
		life=5*1000;
		damage=20;

		anim_fire.load(GameRes::get_anim("hero/rocket_fire.png"),true);

		explode_on_death=true;
	}
	void update(float delta) {
		if(target) {
			float angle_delta=Utils::vec_angle(pos-target->pos);
			angle=Utils::angle_normalize(Utils::angle_move_towards(angle,angle_delta,rot_speed*delta));
		}

		//printf("rocket update, target %p\n",target);

		sprite.setRotation(angle*180/M_PI+90);
		velocity=Utils::num_move_towards(velocity,max_velocity,0.01*delta);
		pos+=Utils::angle_vec(angle)*velocity*delta;
		life-=delta;

		anim_fire.update(delta);
	}
	void draw(const NodeState& state) {
		Bullet::draw(state);

		NodeState s=state;
		s.render_state.transform.translate(pos);
		s.render_state.transform.rotate(sprite.getRotation());
		s.render_state.transform.translate(10,30);
		anim_fire.draw(state);
	}
};
class BulletBlast : public Bullet {

	float width;
	sf::FloatRect bb;

	float cur_len1;
	float cur_len2;

	int mode;	//0=full,1=left,2=right

	SpriteAnim anim;

public:
	float max_size;
	float cur_size;
	float duration;

	BulletBlast() {
		is_point=false;

		duration=500.0;
		life=duration;

		max_size=1000;
		width=200;
		damage=1000;

		anim.load(GameRes::get_anim("hero/blast2.png"),false);

		set_mode_full();
		update(0);
	}

	void set_mode_full() {
		mode=0;


		sf::Texture* tex=GameRes::get_texture("hero/blast.png");
		if(tex) {
			sprite.setTexture(*tex,true);
			sprite.setOrigin(Utils::vec_to_f(tex->getSize())*0.5f);
		}

	}
	void set_mode_left() {
		mode=1;
		/*
		sf::Texture* tex=GameRes::get_texture("hero/blast.png");
		if(tex) {
			sprite.setTexture(*tex);
			sprite.setTextureRect(sf::IntRect(0,0,tex->getSize().x/2,tex->getSize().y));
			sprite.setOrigin(Utils::vec_to_f(tex->getSize())*0.5f);
		}
		sprite.setRotation(0);
		*/
	}
	void set_mode_right() {
		mode=2;
		/*
		sf::Texture* tex=GameRes::get_texture("hero/blast.png");
		if(tex) {
			sprite.setTexture(*tex);
			sprite.setTextureRect(sf::IntRect(0,0,tex->getSize().x/2,tex->getSize().y));
			sprite.setOrigin(Utils::vec_to_f(tex->getSize())*0.5f);
		}
		sprite.setRotation(180);
		*/
	}

	void update(float delta) {
		if(life<0) return;
		life-=delta;
		cur_size=Utils::lerp(max_size,0,life/duration);

		const sf::Texture *tex=sprite.getTexture();
		if(tex) {
			float scale=cur_size/(float)tex->getSize().x;
			sprite.setScale(scale,scale);
		}
		bb=Utils::vec_center_rect(pos,sf::Vector2f(cur_size,cur_size));

		cur_len2=cur_size*cur_size;
		cur_len1=cur_size-width;
		cur_len1*=cur_len1;

		anim.set_progress(1-life/duration);
	}
	void check_ship(Ship* s,float delta) {
		float len=Utils::vec_len_fast(s->pos-pos);
		if(len<cur_len1 || len>cur_len2) return;
		s->damage(damage*delta/1000.0f);
	}
	sf::FloatRect get_bb() {
		return bb;
	}
	void draw(const NodeState& state) {
		/*
		sf::Transform t=parentTransform;
		t.translate(pos);
		t.scale(4,4);
		anim.draw(target,t);
		*/
	}
};

class BulletHammer : public Bullet {
public:
	float duration;
	float prev_angle;
	float cur_angle;
	float range;
	float range2;

	Ship* parent;

	BulletHammer() {
		is_point=false;
		duration=800;
		range=700;
		range2=range*range;
		damage=1000;

		life=duration;

		update_angle();
		prev_angle=cur_angle;

		/*
		sf::Texture* tex=GameRes::get_texture("hero/hammer_trace.png");
		if(tex) {
			sprite.setTexture(*tex,true);
		}
		*/
	}
	void update(float delta) {
		if(life<0) return;

		if(parent) {
			pos=parent->pos;
		}

		life-=delta;
		update_angle();
	}
	void update_angle() {
		prev_angle=cur_angle;
		cur_angle=Utils::angle_normalize(life/duration*M_PI*2.0f+M_PI*0.5f);

		//sprite.setRotation(Utils::rad_to_deg(cur_angle)-90);
		//sprite.setPosition(-50,50);
	}
	void check_ship(Ship* s,float delta) {

		if(Utils::vec_len_fast(pos-s->pos)>range2) return;
		float ship_angle=Utils::vec_angle(pos-s->pos);

		float a1=prev_angle;
		float a2=cur_angle;	//lesser

		if(a2>a1) a2-=M_PI*2.0f;
		if(ship_angle>a1) ship_angle-=M_PI*2.0f;

		if(a1>ship_angle && ship_angle>a2) {
			s->damage(damage*delta/1000.0f);
		}
	}
	sf::FloatRect get_bb() {

		sf::Vector2f p=pos-sf::Vector2f(range,range);
		return sf::FloatRect(p.x,p.y,range*2,range*2);
	}

	void draw(const NodeState& state) {
		/*
		sf::Transform t=parentTransform;

		t.rotate(Utils::rad_to_deg(cur_angle)-90);
		t.translate(sf::Vector2f(-80,61));

		target.draw(sprite,t);
		*/
	}
};

class BulletLaser : public Bullet {
public:

	Ship* bullet_parent;
	sf::Vector2f v_size;
	sf::Texture* tex;

	BulletLaser() {
		life=1;
		is_point=false;

		damage=100;

		tex=GameRes::get_texture("luna/beam.png");
		if(tex) {
			tex->setRepeated(true);
			sprite.setTexture(*tex);
		}
		set_vsize(sf::Vector2f(10,300));

		bullet_parent=NULL;

		shader.shader=&GameRes::shader_damage;
		shader.set_param("amount",0.13f);
		shader.set_param("freq",4);
	}
	void set_vsize(sf::Vector2f _v_size) {
		v_size=_v_size;

//		sprite.setScale(5,1);
		sprite.setTextureRect(sf::IntRect(0,0,tex->getSize().x,v_size.y));
		//sprite.setPosition(-v_size.x*0.5f,0);
		sprite.setPosition(-50*0.5f,0);
		//sprite.setPosition(-tex->getSize().x*0.5f,0);

	}
	sf::FloatRect get_bb() {
		sf::Vector2f p=pos-sf::Vector2f(0.0f,v_size.x*0.5f);
		return sf::FloatRect(p.x,p.y,v_size.x,v_size.y);
	}
	void check_ship(Ship* s,float delta) {
		sf::FloatRect rect;
		if(get_bb().intersects(s->bounding_box)) {
			s->damage(damage*delta/1000.0f);
		}
	}
	void update(float delta) {
		if(bullet_parent) {
			pos=bullet_parent->pos;
		}
		shader.set_param("time",game->game_clock.getElapsedTime().asSeconds()*5);
	}
};


ShipGun ShipGun::fixed(sf::Vector2f pos,float angle,float bullet_speed,sf::Texture* texture) {
	ShipGun g;
	g.pos=pos;
	g.lat_spread=0;
	g.angle=angle;
	g.angle_spread=0;
	g.aim_type=GUN_AIM_TYPE_FIXED;
	g.spiral_speed=0;
	g.use_master_timer=true;
	g.bullet_speed=bullet_speed;
	g.bullet_texture=texture;
	return g;
}
ShipGun ShipGun::spiral(sf::Vector2f pos,float spiral_speed,float bullet_speed,sf::Texture* texture) {
	ShipGun g;
	g.pos=pos;
	g.lat_spread=0;
	g.angle=0;
	g.angle_spread=0;
	g.aim_type=GUN_AIM_TYPE_SPIRAL;
	g.spiral_speed=spiral_speed;
	g.use_master_timer=true;
	g.bullet_speed=bullet_speed;
	g.bullet_texture=texture;
	return g;
}




void Ship::update_bounding_box() {
	bounding_box=Utils::vec_center_rect(pos,getScaledSize());
}

Ship::Ship() {
	anim_is_fire=false;
	team=TEAM_ENEMY;
	game=NULL;
	damage_mult=1.0;
	max_acceleration=0.002;
	max_speed=5;
	ctrl_fire=false;
	circle_anim=Utils::rand_range(0,M_PI*2.0f);

	projectile_texture=GameRes::projectile;
	entered=false;
	health=10;
	hurt_by_terrain=true;
	hurt_by_ship=true;
	hurt_by_bullet=true;
	hit=false;

	switch_anim_fire.init(false,0,100);
	fire_timeout.set(1000,100,5);

	shield_on=false;
	shield_size=100.0f;
	can_remove=true;
	blink_enabled=true;

	controller=NULL;

	death_atlas=NULL;
	death_active=false;

	setScale(1.0f);

}
Ship::~Ship() {
	if(controller) {
		delete(controller);
	}
}
void Ship::setScale(float _scale) {
	scale=_scale;
}
void Ship::damage(float dmg) {
	health-=dmg;
	hit=true;

	if(blink_enabled) {
		blink_damage.reset();
	}

	on_damage(dmg);
}

void Ship::draw(const NodeState& state) {
	if(anim.loaded()) {
		anim.draw(state);
	}
	else {
		state.render_target->draw(sprite,state.render_state);
	}
}
void Ship::set_texture(const sf::Texture* tex) {
	if(!tex) {
		return;
	}
	sprite.setTexture(*tex,true);
	sprite.setOrigin(Utils::vec_to_f(tex->getSize())*0.5f);
}
void Ship::set_anim(SpriteAnimData* _anim,bool _is_fire) {
	anim.load(_anim);
	anim.loop=!_is_fire;
	anim_is_fire=_is_fire;
}
void Ship::move(sf::Vector2f x) {
	ctrl_move=x;
}

void Ship::frame(float delta) {

	if(!blink_damage.done()) {
		blink_damage.update(delta);
		//visible=!blink_damage.get();
		render_state.shader=(blink_damage.get() ? &GameRes::shader_blink : NULL);
	}

	switch_anim_fire.set(false);

	if(fire_timeout.frame(delta) && ctrl_fire) {
		fire();
		fire_timeout.consume();
	}

	if(anim.loaded()) {
		if(anim_is_fire) {
			switch_anim_fire.frame(delta);
			anim.set_frame(switch_anim_fire.on ? 1 : 0);
		}
		else {
			anim.update(delta);
		}
	}


	for(int i=0;i<guns.size();i++) {
		if(guns[i].aim_type==GUN_AIM_TYPE_SPIRAL) {
			guns[i].angle+=guns[i].spiral_speed*delta;
		}
	}

	if(controller && health>0) {
		controller->frame(delta);
	}

	//apply move ctrl
	acc+=Utils::vec_cap(ctrl_move,max_acceleration);
	ctrl_move=sf::Vector2f(0,0);

	vel+=acc*delta;
	pos+=vel*delta;
	acc=sf::Vector2f(0,0);

	vel=Utils::vec_cap(vel,max_speed);

	render_state.transform=sf::Transform::Identity;
	render_state.transform.translate(pos);
	render_state.transform.scale(scale,scale);

	//death atlas
	if(death_atlas && health<=0) {
		if(!death_active) {
			ctrl_fire=false;
			blink_enabled=false;

			death_explosion_timeout.set(100);
			death_explosion_timeout.reset();
			death_total_explosion_count=15;
			death_explosion_count=0;
			death_active=true;
		}
		death_explosion_timeout.frame(delta);
		if(death_explosion_timeout.ready()) {
			death_explosion_timeout.reset();

			death_explosion_count++;
			if(!can_remove && death_explosion_count>=death_total_explosion_count) {
				can_remove=true;

				for(int i=0;i<death_atlas->items.size();i++) {
					Decal* d=new Decal();
					d->sprite=death_atlas->get_sprite(i);
					d->life=Utils::rand_range(2000,3000);
					d->pos=pos-size*0.5f+sf::Vector2f(death_atlas->items[i].pos.x,death_atlas->items[i].pos.y);
					d->vel=vel+sf::Vector2f(
							death_atlas->items[i].pos.x+death_atlas->items[i].size.x/2-size.x/2,
							death_atlas->items[i].pos.y+death_atlas->items[i].size.y/2-size.y/2
							)*0.001f;
					d->vel+=sf::Vector2f(Utils::rand_range(-0.5,0.5),Utils::rand_range(-0.5,0.5))*0.08f;
					game->add_decal(d);
				}
			}

			game->add_explosion(pos-size*0.5f+sf::Vector2f(size.x*Utils::rand_float(),size.y*Utils::rand_float()));

		}
	}
}
sf::Vector2f Ship::find_attack_vector(float dist) {

	int gun_start;
	for(gun_start=0;gun_start<guns.size();gun_start++) {
		if(guns[gun_start].aim_type==GUN_AIM_TYPE_FIXED) break;
	}
	if(gun_start>=guns.size()) {
		return game->player->pos+Utils::angle_vec(circle_anim)*dist;
	}

	sf::Vector2f delta=pos-game->player->pos;

	float min_dot=Utils::vec_dot(delta,Utils::angle_vec(guns[gun_start].angle));
	int min_i=gun_start;

	for(int i=gun_start+1;i<guns.size();i++) {
		if(guns[i].aim_type!=GUN_AIM_TYPE_FIXED) continue;
		float d=Utils::vec_dot(delta,Utils::angle_vec(guns[i].angle));
		if(d<min_dot) {
			min_dot=d;
			min_i=i;
		}
	}
	return game->player->pos+Utils::angle_vec(guns[min_i].angle+M_PI)*dist;
}
void Ship::fire() {

	switch_anim_fire.set(true);

	for(int i=0;i<guns.size();i++) {

		if(!guns[i].use_master_timer) continue;

		float vel=guns[i].bullet_speed;

		Bullet *b=new Bullet();

		if(guns[i].aim_type==GUN_AIM_TYPE_AIM) {
			//dir=Utils::vec_normalize(game->player->pos-pos);
			b->vel=Utils::target_lead(game->player->pos-pos,game->player->vel,vel);
		}
		else {
			b->vel=vel*Utils::angle_vec(guns[i].angle+Utils::rand_range(-guns[i].angle_spread,guns[i].angle_spread));
		}
		sf::Vector2f b_pos=pos+guns[i].pos;	//TODO: add lateral spread

		b->pos=b_pos;
		b->autorotate();
		b->sprite.setTexture(*guns[i].bullet_texture,true);
		b->type=1;
		b->damage*=damage_mult;
		game->add_bullet(b);
	}
}
void Ship::on_spawn() {
	max_health=health;
}
void Ship::set_death_atlas(Atlas* atlas) {
	death_atlas=atlas;
	if(death_atlas) {
		can_remove=false;
	}
}

Drone::Drone() {
	ctrl_fire=true;
	fire_timeout.set(500,100,3);
}
void Drone::frame(float delta) {
	Ship::frame(delta);

	circle_anim+=0.002*delta;
	sf::Vector2f target=game->player->pos+Utils::angle_vec(circle_anim)*150.0f;
	sf::Vector2f vec=Utils::vec_normalize(target-pos)*0.5f;
	move( (vec-vel)*0.5f );
}
void Drone::fire() {
	Ship* target=game->get_target(pos);
	if(!target) return;

	float vel=1.0f;
	sf::Vector2f dir=Utils::vec_normalize(target->pos-pos);

	Bullet* b=new Bullet();
	b->pos=pos;
	b->vel=dir*vel;
	b->autorotate();
	b->sprite.setTexture(*GameRes::projectile_2,true);
	b->type=0;
	b->damage*=damage_mult;

	//game->add_bullet(pos,dir*vel,0);
	game->add_bullet(b);
}

Player::Player() {
	set_texture(GameRes::player_ship);
	max_acceleration=0.008f; //0.0015;
	max_speed=9.0f; //5;

	health=100.0f;
	weapon_charge=0.0f;

	//timeout_fire.set(600);
	fire_timeout.set(500,0,1);

	weapon_i=0;
	weapon_slot=0;

	blink_enabled=false;

	anim_shinning.load(GameRes::get_anim("hero/shinning.png"));
	anim_engine_fire.load(GameRes::get_anim("hero/engine_fire.png"),true);
	//anim_hammer.load(GameRes::get_anim("hero/hammer.png"));
	anim_shield.load(GameRes::get_anim("hero/shield.png"));

	sf::Texture* hammer_texture=GameRes::get_texture("hero/hammer.png");
	if(hammer_texture) {
		sprite_hammer.setTexture(*hammer_texture);
		//sprite_hammer.setPosition(-hammer_texture->getSize().x/2,30);
		sprite_hammer.setPosition(-(float)hammer_texture->getSize().x/2.0,60);
	}
	sprite_hammer_swing.setTexture(*GameRes::get_texture("hero/hammer_trace.png"));

	std::string shield_ring_files[]={
			"hero/shield_1.png",
			"hero/shield_2.png",
			"hero/shield_3.png"
	};

	for(int i=0;i<3;i++) {
		sf::Texture* t=GameRes::get_texture(shield_ring_files[i]);
		if(t) {
			sprite_shield_ring[i].setTexture(*t);
			sprite_shield_ring[i].setOrigin(Utils::vec_to_f(t->getSize())*0.5f);
		}
	}
	/*
	sprite_shield_ring[0].setTexture(*GameRes::get_texture("hero/shield_1.png"));
	sprite_shield_ring[1].setTexture(*GameRes::get_texture("hero/shield_2.png"));
	sprite_shield_ring[2].setTexture(*GameRes::get_texture("hero/shield_3.png"));
	*/

	hammer_timeout.set(800);

	timeout_shinning.set(10000);
	timeout_shinning.reset();
	engine_fire_visible.init(false,0,750);

	hammer_on=false;

	team=TEAM_PLAYER;

	weapon_drain[0]=0.1f;
	weapon_drain[1]=0.4f;
	weapon_drain[2]=0.6f;
	weapon_drain[3]=0.6f;
	weapon_drain[4]=0.8f;
	weapon_drain[5]=0.3f;

	prev_shield_enabled=false;
}

void Player::fire_blaster(int bullet_count,float angle_spread,float angle_offset) {
	float vel=1.5f;

	const sf::Texture* blast_texture=GameRes::get_texture("hero/blast.png");

	for(int i=0;i<bullet_count;i++) {
		float angle=(float)i/(float)(bullet_count-1)*angle_spread+angle_offset;
		sf::Vector2f dir(cos(angle),sin(angle));

		Bullet* b=new Bullet();
		b->pos=pos+dir*50.0f;
		b->vel=dir*vel*Utils::rand_range(0.8,1.2);
		b->type=0;
		b->size=50;
		b->autorotate();
		b->sprite.setTexture(*blast_texture,true);

		game->add_bullet(b);

	}
}

void Player::fire() {

	//0: blaster
	//1: full blaster
	//2: swinger
	//3: missiles
	//4: follower
	//5: shield

	if(weapon_i==5) {
		return;
	}

	if(getWeaponDrain(weapon_i)>weapon_charge) {
		return;
	}

	weapon_charge=std::max(0.0f,weapon_charge-getWeaponDrain(weapon_i));

	switch(weapon_i) {
		case 0: {	//blaster
			fire_blaster(10,M_PI,M_PI/2.0f+weapon_slot*M_PI);
			/*
			BulletLaser* laser=new BulletLaser();
			//laser->set_vsize(sf::Vector2f(10,200));
			laser->pos=pos;
			laser->on_top=true;
			laser->parent=this;

			game->add_bullet(laser);
			*/
			break;
		}
		case 1: {	//full blaster
			//fire_blaster(20,M_PI*2.0f,0);
			BulletBlast* blast=new BulletBlast();
			blast->set_mode_full();
			blast->pos=pos;
			blast->type=0;
			blast->damage*=damage_mult;
			game->add_bullet(blast);

			game->set_effect_blast(true,0.0f);

			break;
		}
		case 2: {	//swinger/hammer
			if(hammer_on) {
				break;
			}
			hammer_on=true;
			hammer_timeout.reset();

			BulletHammer* hammer=new BulletHammer();
			hammer->parent=this;
			hammer->pos=pos;
			hammer->type=0;
			hammer->damage*=damage_mult;
			game->add_bullet(hammer);
			break;
		}
		case 3: {	//rockets
			int count=Utils::rand_range_i(2,4);

			std::vector<Ship*> targets=game->get_targets(pos,count);

			float spread=M_PI*0.15f;

			for(int i=0;i<count;i++) {
				BulletRocket* rocket=new BulletRocket();
				rocket->pos=pos+sf::Vector2f(0,50);
				rocket->type=0;
				rocket->angle=M_PI*0.25f*2 + Utils::lerp(-spread,spread,((float)i/(float)(count-1)));
				rocket->target=(targets.size()>0 ? targets[i%targets.size()] : NULL);

				game->add_bullet(rocket);
			}
			break;
		}
		case 4: {	//drone
			Ship* drone=GameRes::ship_group_get_rand("drones");
			if(drone) {
				drone->pos=pos;
				drone->team=TEAM_PLAYER;
				game->spawn_ship(drone);
			}
			break;
		}
		case 5:		//shield
			break;
	}
}
void Player::frame(float delta) {

	engine_fire_visible.set(Utils::vec_len_fast(ctrl_move)>max_acceleration*max_acceleration*0.1f);

	Ship::frame(delta);

	anim_engine_fire.update(delta);
	engine_fire_visible.frame(delta);

	float shield_drain=delta*0.001f;
	bool shield_can_activate=(weapon_charge>getWeaponDrain(5));
	bool shield_commanded=(weapon_i==5 && ctrl_fire && weapon_charge>shield_drain);

	if(!shield_can_activate) {
		if(!prev_shield_enabled) {
			shield_commanded=false;
		}
	}

	float shield_duration=200.0f;

	tracer_shield.move_to(shield_commanded ? shield_duration : 0.0);

	tracer_shield.frame(delta);
	if(tracer_shield.changed) {
		anim_shield.set_progress(tracer_shield.current/shield_duration);
	}
	shield_on=(shield_commanded && tracer_shield.idle);
	prev_shield_enabled=shield_on;

	if(shield_on) {
		weapon_charge-=shield_drain;
	}

	if(hammer_on) {
		/*
		anim_hammer.update(delta);
		if(anim_hammer.done) {
			anim_hammer.set_frame(0);
			hammer_on=false;
		}
		*/
		hammer_timeout.frame(delta);
		if(hammer_timeout.ready()) {
			hammer_on=false;
		}
	}
	else {

	}

	if(anim_shinning.done) {
		timeout_shinning.frame(delta);
		if(timeout_shinning.ready()) {
			timeout_shinning.set(Utils::rand_range(5000,10000));
			timeout_shinning.reset();
			anim_shinning.set_frame(0);
		}
	}
	else {
		anim_shinning.update(delta);
	}

	weapon_charge=std::min(1.0f,weapon_charge+delta*0.0002f);
}

void Player::draw(const NodeState& state) {

	if(hammer_on) {
		sf::RenderStates s=state.render_state;
		s.transform.rotate(-360.0*hammer_timeout.get_progress());
		s.transform.translate(-80,61);

		sf::Color c=sf::Color::White;

		sprite_hammer_swing.setColor(c);
		state.render_target->draw(sprite_hammer_swing,s);

		for(int i=0;i<5;i++) {
			c.a=(1.0f-(float)i/4)*128.0f;
			sprite_hammer_swing.setColor(c);
			s.transform.rotate(10);
			state.render_target->draw(sprite_hammer_swing,s);
		}
	}
	else {
		state.render_target->draw(sprite_hammer,state.render_state);
	}

	Ship::draw(state);

	if(!anim_shinning.done) {
		anim_shinning.draw(state);
	}

	if(engine_fire_visible.on) {
		float fire_y=90; //78;

		NodeState s=state;
		s.render_state.transform.translate(36,fire_y);
		anim_engine_fire.draw(s);

		s=state;
		s.render_state.transform.translate(-36,fire_y);
		anim_engine_fire.draw(s);
	}

	if(tracer_shield.current>0.0) {
		/*
		sf::Transform t=parentTransform;
		t.translate(0,10);
		anim_shield.draw(target,t);
		*/
		anim_shield.draw(state);

		if(shield_on) {
			static float ring_speed[]={100.0,-150.0,200.0};
			for(int i=0;i<3;i++) {
				sf::RenderStates s=state.render_state;
				s.transform.rotate(game->game_clock.getElapsedTime().asSeconds()*ring_speed[i]);
				state.render_target->draw(sprite_shield_ring[i],s);
			}
		}
	}
}
float Player::getWeaponCharge(int i) {
	return weapon_charge/weapon_drain[i];
}
float Player::getWeaponDrain(int i) {
	return weapon_drain[i];
}
/*
bool Player::getWeaponDrainContinious(int i) {
	bool cont[]={1,0,0,};
}
*/

EnemyKamikaze::EnemyKamikaze() {
	health=1;
	ctrl_fire=false;
	max_speed=1;
	inited=false;
}
void EnemyKamikaze::frame(float delta) {
	Ship::frame(delta);

	if(!inited) {
		inited=true;
		vec=Utils::vec_normalize(game->player->pos-pos)*100.0f;
	}

	move(vec);

	if(Utils::vec_len_fast(game->player->pos-pos)>2000*2000) {
		health=0;
	}
}


EnemyMelee::EnemyMelee() {
	retracting=false;
	hurt_by_ship=false;
}
void EnemyMelee::frame(float delta) {
	EnemyKamikaze::frame(delta);

	if(retracting) {
		sf::Vector2f target_vec=game->player->pos-pos;

		float dot=Utils::vec_dot(vel,target_vec);
		if(dot>0) {
			retracting=false;
			vec=Utils::vec_normalize(target_vec)*100.0f;
		}
	}
}
bool EnemyMelee::on_collide_ship(Ship* s) {
	if(retracting) return true;
	vel=-vel;
	vec=Utils::vec_normalize(-vel)*100.0f;
	retracting=true;
	return true;
}


EnemyParent::EnemyParent() {
	health=50;
	fire_timeout.set(5000,0,1);
	ctrl_fire=false;
	max_speed=0.2;
	child_type=0;
}
void EnemyParent::frame(float delta) {
	Ship::frame(delta);

	sf::Vector2f target;

	/*
	if(fire_count<=0) {
		ctrl_fire=false;

		target=pos+Utils::vec_normalize(pos-game->player->pos)*10000.0f;
		if(Utils::vec_len_fast(pos-game->player->pos)>2000*2000) {
			health=0;
			return;
		}
	}
	else {
	*/
		target=game->player->pos+Utils::angle_vec(circle_anim)*250.0f;

		sf::Vector2f vec=target-pos;
		ctrl_fire=(entered && Utils::vec_len_fast(pos-game->player->pos)<400*400);
	//}

	move(target-pos);
}

void EnemyParent::fire() {
	Ship* e=new Ship();
	e->pos=pos;
	e->team=TEAM_ENEMY;
	e->set_texture(sprite.getTexture());
	game->spawn_ship(e);
}

//bosses ----

BossPlatypus::BossPlatypus() {
	set_texture(GameRes::get_texture("platypus/phase1.png"));
	/*
	ControllerShooter* ct=new ControllerShooter();
	ct->dist=700.0;
	ct->circle_speed=0.000003;
	set_controller(ct);
	*/

	anim_engine.load(GameRes::get_anim("platypus/engine_fire.png"),true);
	anim_appear.load(GameRes::get_anim("platypus/platypus_appear.png"));
	anim_dissappear.load(GameRes::get_anim("platypus/platypus_dissappear.png"));
	anim_bellyshot.load(GameRes::get_anim("platypus/belly_shot.png"));

	sf::Vector2f zero;
	anim_engine.set_offset(zero);
	anim_appear.set_offset(zero);
	anim_dissappear.set_offset(zero);
	anim_bellyshot.set_offset(zero);

	platypus_pos=50;

	phase=0;

	timeout_teleport.set(500);
	timeout_launch.set(2000);
	launch_count=0;

	timeout_bellyshot.set(500);
	bellyshot_active=false;

	current_platypus_anim=NULL;

	set_death_atlas(GameRes::get_atlas("platypus/pieces.png"));
}
void BossPlatypus::frame(float delta) {
	Ship::frame(delta);

	if(death_active) {
		return;
	}

	sf::Vector2f target=game->player->pos+sf::Vector2f(0,-250);
	sf::Vector2f vec=Utils::vec_normalize(target-pos)*0.5f;
	move( (vec-vel)*0.5f );

	if(phase==0 && health<max_health/2) {
		phase=1;
		timeout_launch.reset();

		set_texture(GameRes::get_texture("platypus/phase2.png"));
	}

	if(phase==1) {

		timeout_launch.frame(delta);

		if(timeout_launch.ready()) {
			timeout_launch.reset();

			static sf::Vector2f launch_pos[]={
					sf::Vector2f(-40,50),
					sf::Vector2f( 40,50)
			};
			static std::string launch_ship[]={
					"beaver1",
					"moose1"
			};

			Ship* e=GameRes::get_ship(launch_ship[Utils::rand_range_i(0,1)]);
			e->pos=pos+launch_pos[launch_count%2];
			e->team=TEAM_ENEMY;
			game->spawn_ship(e);

			launch_count++;
		}
	}

	anim_engine.update(delta);

	//platypus anim
	if(current_platypus_anim) {
		current_platypus_anim->update(delta);
		if(current_platypus_anim->done) {

			if(current_platypus_anim==&anim_dissappear) {
				anim_dissappear.set_progress(0);
				set_anim(&anim_appear);
				anim_offset=sf::Vector2f(22-6,20-12);
				platypus_pos=Utils::rand_range(0,100);
			}
			else {
				set_anim(NULL);
				anim_offset=sf::Vector2f(0,0);
			}

		}
	}
	else {
		timeout_teleport.frame(delta);
		if(timeout_teleport.ready()) {
			timeout_teleport.reset();

			if(Utils::rand_float()<0.35f) {
				set_anim(&anim_dissappear);
				anim_offset=sf::Vector2f(0,0);
			}
			else {
				set_anim(&anim_bellyshot);
				anim_offset=sf::Vector2f(22-57,20-14);
				timeout_bellyshot.reset();
				bellyshot_active=true;
			}
		}
	}

	//bellyshot
	if(bellyshot_active) {

		timeout_bellyshot.frame(delta);

			if(timeout_bellyshot.ready()) {
			Bullet* b=new Bullet();
//			b->pos=pos+sf::Vector2f(platypus_pos-210/2+80,20-208/2+54);
			b->pos=pos+sf::Vector2f(platypus_pos-210/2+30,-208/2+72);
			b->vel=Utils::vec_normalize(game->player->pos-b->pos)*0.2f;
			b->sprite.setTexture(*GameRes::get_texture("proj4.png"),true);
			b->type=1;
			b->damage*=damage_mult;
			b->life=8*1000;
			b->on_top=true;
			game->add_bullet(b);

			bellyshot_active=false;
			//transform.Identity
		}
	}

}

void BossPlatypus::draw_anim(const NodeState& state,SpriteAnim& anim,sf::Vector2f pos) {
	sf::Vector2f offset=pos-sf::Vector2f(210,208)*0.5f;
	offset.x=(int)offset.x;
	offset.y=(int)offset.y;

	NodeState s=state;
	s.render_state.transform.translate(offset);
	anim.draw(s);
}
void BossPlatypus::set_anim(SpriteAnim* anim) {
	current_platypus_anim=anim;
	if(!current_platypus_anim) {
		return;
	}
	current_platypus_anim->set_progress(0);
}

void BossPlatypus::draw(const NodeState& state) {

	Ship::draw(state);

	if(death_active) {
		return;
	}

	sf::Vector2f pos[]={
			sf::Vector2f(4,169),
			sf::Vector2f(183,169)
	};
	for(int i=0;i<2;i++) {
		draw_anim(state,anim_engine,pos[i]);
	}

	if(current_platypus_anim) {
		draw_anim(state,*current_platypus_anim,sf::Vector2f(platypus_pos,22)+anim_offset);
	}
	else {
		draw_anim(state,anim_dissappear,sf::Vector2f(platypus_pos,22)+anim_offset);
	}
}


BossWalrus::BossWalrus() {
	hull.load(GameRes::get_anim("walrus/walrus.png"));
	//timeout_death.set(1000);
	//death_active=false;
	//can_remove=false;
	stage=0;
	set_controller(new ControllerShooter());
}
void BossWalrus::frame(float delta) {
	Ship::frame(delta);

/*
	if(health<=0) {
		if(!death_active) {
			death_active=true;
			timeout_death.reset();
			set_controller(NULL);
			ctrl_fire=false;
		}

		timeout_death.frame(delta);
		if(timeout_death.ready()) {
			can_remove=true;
			game->add_explosion(pos);
		}
		//hull.set_progress( 6.0f/9.0f + (timeout_death.get_progress())*3.0f/9.0f );

		timeout_death_explosion.frame(delta);
		if(timeout_death_explosion.ready()) {
			timeout_death_explosion.set(Utils::rand_range(250,400));
			timeout_death_explosion.reset();
			game->add_explosion(pos+Utils::rand_dist_vec(75));
		}
	}
*/
}
void BossWalrus::draw(const NodeState& state) {
	Ship::draw(state);
	hull.draw(state);
}
void BossWalrus::on_damage(float dmg) {
	hull.set_progress(1.0f-health/max_health);

	static float split_points[]={3.0f/9.0f,2.0f/9.0f,1.0f/9.0f};

	if(stage<3) {
		if(health/max_health<split_points[stage]) {

		//	printf("split points %f %f %f\n",split_points[0],split_points[1],split_points[2]);
		//	printf("walrus %p health %f, split stage %d\n",this,health/max_health,stage);

			health=max_health*split_points[stage]*0.99;

			stage++;
			BossWalrus* w=(BossWalrus*)clone();
			w->pos+=sf::Vector2f(0,50);
			w->setScale(scale);
			game->spawn_ship(w);
			w->max_health=max_health;
		}
	}

}


BossLuna::BossLuna() {
	hull.load(GameRes::get_anim("luna/luna.png"));
	set_death_atlas(GameRes::get_atlas("luna/pieces.png"));
	set_controller(new ControllerShooter());

	rocket_fired=false;

	junior_timeout.set(1000);
	junior_timeout.reset();

	laser_start_timeout.set(1000);
	laser_start_timeout.reset();
	laser_fire_timeout.set(2000);

	laser=NULL;
}
void BossLuna::draw(const NodeState& state) {
	Ship::draw(state);
	hull.draw(state);
}
void BossLuna::frame(float delta) {
	Ship::frame(delta);

	if(laser) {
		laser->pos=pos+sf::Vector2f(0,50);
	}

	if(death_active) {
		return;
	}

	if(laser_start_timeout.ready()) {
		laser_fire_timeout.frame(delta);
		if(laser_fire_timeout.ready()) {
			laser_start_timeout.reset();
			laser->life=0;
			laser=NULL;	//game deallocs laser
		}
	}
	else {
		if(fabs(pos.x-game->player->pos.x)<game->player->pos.y-pos.y) {
			laser_start_timeout.frame(delta);

			if(laser_start_timeout.ready()) {
				laser_fire_timeout.reset();

				BulletLaser* l=new BulletLaser();
				l->type=1;
				l->set_vsize(sf::Vector2f(10,1500));
				l->on_top=true;
				l->pos=pos+sf::Vector2f(0,50);
				laser=l;
				game->add_bullet(laser);
			}
		}
		else {
			junior_timeout.frame(delta);
			if(junior_timeout.ready()) {
				junior_timeout.reset();

				Ship* s=GameRes::get_ship("lunajunior");
				if(s) {
					s->pos=pos+sf::Vector2f(0,50);
					s->team=team;
					game->spawn_ship(s);
				}
			}
		}
	}

	hull.update(delta);
	if(!rocket_fired && hull.get_frame()>=4) {
		rocket_fired=true;

		BulletRocket* rocket=new BulletRocket();
		rocket->sprite.setTexture(*GameRes::get_texture("luna/rocket.png"),true);
		rocket->pos=pos+sf::Vector2f(19,12);
		rocket->type=1;
		rocket->angle=M_PI*0.5f;
		rocket->target=game->player;
		rocket->on_top=true;
		rocket->velocity=0.2f;
		rocket->max_velocity=0.3f;
		rocket->rot_speed=M_PI*0.0003f;

		game->add_bullet(rocket);
	}
	if(hull.done) {
		rocket_fired=false;
		hull.set_progress(0);
	}
}
void BossLuna::on_damage(float damage) {
	if(health<=0 && laser) {
		laser->life=0;
		laser=NULL;
	}
}


class BulletOctopuss : public Bullet {
public:

	BulletOctopuss() {
		sprite.setTexture(*GameRes::get_texture("octopuss/projectile.png"),true);
	}
	void check_ship(Ship* s,float delta) {
		s->damage(damage);
		life=0;
		//game->add_explosion(pos,false);

		char filename[200];
		sprintf(filename,"octopuss/splashscreen%d.png",Utils::rand_range_i(1,8));

		Decal* d=new Decal();
		d->sprite.setTexture(*GameRes::get_texture(filename));
		d->life=Utils::rand_range(2000,2500);
		d->pos.x=Utils::rand_range(0,game->size.x);
		d->pos.y=Utils::rand_range(0,game->size.y);
		d->vel=sf::Vector2f(0,0.01);
		d->wind_speed=0.03;
		d->alpha_fade=500;
		d->sprite.setScale(3,3);
		game->add_splash(d);
	}
};

BossOctopuss::BossOctopuss() {
	hull.load(GameRes::get_anim("octopuss/octopuss.png"),true);

	ControllerShooter* c=new ControllerShooter();
	c->dist=500;
	c->circle_speed*=0.1f;
	set_controller(c);

	fire_timeout.set(375);
}

void BossOctopuss::frame(float delta) {
	Ship::frame(delta);
	hull.update(delta);
	fire_timeout.frame(delta);
	if(fire_timeout.ready()) {
		fire_timeout.reset();

		Bullet* b=new BulletOctopuss();
		b->pos=pos+sf::Vector2f(0,80);
		//b->vel=Utils::vec_normalize(game->player->pos-b->pos)*0.5f;
		b->vel=Utils::target_lead(game->player->pos-b->pos,game->player->vel,1.0f);
		b->type=1;
		b->life=8*1000;
		game->add_bullet(b);

	}
}
void BossOctopuss::draw(const NodeState& state) {
	Ship::draw(state);
	hull.draw(state);
}











