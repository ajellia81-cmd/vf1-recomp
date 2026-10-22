#include "../../src/host/ios_input.h"
#include <cassert>
#include <cstdio>
int main(){
 ios_input_snapshot input;bool menu=false;
 input.down[0][C_PUNCH]=true;input.down[1][C_START]=true;input.menu_pressed=true;
 ios_input_menu_transition(input,menu,true);
 assert(menu && input.menu_pressed && !input.down[0][C_PUNCH] && !input.down[1][C_START]);
 input.menu_pressed=false; // host consumes the opening tap
 input.menu_pressed=true; // the next tap while the menu is already visible
 ios_input_menu_transition(input,menu,true);
 assert(input.menu_pressed);
 ios_input_menu_transition(input,menu,true);
 assert(input.menu_pressed);
 ios_input_menu_transition(input,menu,false);
 assert(!menu && input.menu_pressed);
 puts("PASS: menu transitions release game buttons and preserve Menu taps while open");
}
