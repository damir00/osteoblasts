#ifndef _BGA_FRAMEWORK_H_
#define _BGA_FRAMEWORK_H_

class Menu;

class Framework {
	class Impl;
	Impl* impl;
public:

	static int MESSAGE_QUIT;

	Framework();
	~Framework();
	void run();
	void quit();
	Menu* get_root_menu();
};

#endif
