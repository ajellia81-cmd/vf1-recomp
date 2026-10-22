// ROM-free round-trip checks for all per-player key/button/selection settings.
#include "../../src/host/controls.h"
#include <cassert>
#include <cstdio>
int main() {
	for(int player=0;player<2;++player) {
		auto expected=player_controls::defaults(player);
		expected.pad_choice=player ? "none" : "synthetic-controller-guid";
		expected.key[C_GUARD]=SDL_SCANCODE_A;
		expected.pad[C_GUARD]=SDL_GAMEPAD_BUTTON_EAST;
		expected.pad[C_MENU]=PAD_LT;
		expected.pad[C_COIN]=SDL_GAMEPAD_BUTTON_NORTH;
		FILE *file=tmpfile(); assert(file); expected.save(file,player); rewind(file);
		std::map<std::string,std::string> kv; char line[1100],key[64],value[1024];
		while(fgets(line,sizeof line,file)) if(sscanf(line," %63[a-z0-9_] = %1023[^\r\n]",key,value)==2) kv[key]=value;
		fclose(file);
		auto actual=player_controls::defaults(player); actual.load(kv,player);
		assert(actual.pad_choice==expected.pad_choice);
		for(int action=0;action<C_COUNT;++action) {
			assert(actual.key[action]==expected.key[action]);
			assert(actual.pad[action]==expected.pad[action]);
		}
	}
	std::puts("PASS: both players' controller selection and all keyboard/button bindings survive save/load");
}
