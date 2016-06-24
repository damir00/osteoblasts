#ifndef Audio_H_
#define Audio_H_

#include <string>

class AudioPrivate;

class Audio {
	static AudioPrivate* priv;
public:
	static void init();
	static void deinit();
	static void play_music(std::string filename);
	static void play_sound(std::string sound);

};

#endif
