#include "Audio.h"

#include <SFML/Audio.hpp>
#include <tr1/unordered_map>

#include "Utils.h"

class AudioPrivate {
public:
	sf::Music current_music;
	std::tr1::unordered_map<std::string,sf::SoundBuffer*> sound_buffers;
	std::vector<sf::Sound*> sounds;
};


AudioPrivate* Audio::priv=NULL;

void Audio::init() {
	if(!priv) {
		priv=new AudioPrivate();
	}
}
void Audio::deinit() {
	priv->current_music.stop();
}
void Audio::play_music(std::string filename) {
	DEBUG("playing music %s\n",filename.c_str());
	if(priv->current_music.openFromFile("assets/sound/"+filename)) {
		priv->current_music.setLoop(true);
		priv->current_music.play();
	}
	DEBUG("OK\n");
}
void Audio::play_sound(std::string sound) {
	sf::SoundBuffer* b=NULL;

	DEBUG("playing sound %s\n",sound.c_str());

	if(priv->sound_buffers.count(sound)>0) {
		b=priv->sound_buffers[sound];
	}
	else {
		b=new sf::SoundBuffer();
		if(!b->loadFromFile("assets/sound/"+sound)) {
			delete(b);
			b=NULL;
		}
		else {
			priv->sound_buffers[sound]=b;
		}
	}

	if(!b) {
		DEBUG("error\n");
		return;
	}

	sf::Sound* s=NULL;
	for(int i=0;i<priv->sounds.size();i++) {
		if(priv->sounds[i]->getStatus()!=sf::Sound::Playing) {
			s=priv->sounds[i];
			break;
		}
	}
	if(!s && priv->sounds.size()<150) {
		s=new sf::Sound();
		priv->sounds.push_back(s);
	}
	if(!s) {
		DEBUG("error\n");
		return;
	}

	s->setBuffer(*b);
	s->play();

	DEBUG("ok\n");
}
