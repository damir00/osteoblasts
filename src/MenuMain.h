#ifndef _BGA_MENU_MAIN_H_
#define _BGA_MENU_MAIN_H_

#include "Menu.h"

class MenuMain : public Menu {
	class MenuMainImpl;
	MenuMainImpl* impl;
public:
	MenuMain();
	~MenuMain();
	virtual void event_resize() override;
	void set_quit_action(int action);

	void go_game();
};

#endif
