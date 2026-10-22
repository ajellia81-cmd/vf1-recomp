#pragma once

#include "controls.h"
#if defined(VF1_IOS) && !defined(VF1_DISABLE_JIT)
#error "iOS must be compiled with VF1_DISABLE_JIT"
#endif

// The iOS adapter owns UIKit/GameController callbacks; the SDL host samples this
// small, UI-neutral state once per emulation frame. It is compiled only in the iOS target.
struct ios_input_snapshot {
	bool down[2][C_COUNT]{};
	bool menu_pressed = false;
};

struct SDL_Window;
void ios_input_install(SDL_Window *window);
void ios_input_reset();
void ios_input_set_menu(bool open);
void ios_input_uninstall();
ios_input_snapshot ios_input_read();

// Entering the menu releases gameplay buttons; a Menu tap must survive both
// the transition and subsequent UI updates until the host consumes it.
inline void ios_input_menu_transition(ios_input_snapshot &input, bool &menu_open, bool open)
{
 if(open && !menu_open)
  for(auto &player:input.down)for(bool &down:player)down=false;
 menu_open=open;
}
