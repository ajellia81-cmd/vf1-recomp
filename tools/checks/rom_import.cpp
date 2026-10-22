#include "../../src/host/romset.h"
#include <cassert>
#include <cstdio>
int main(){
 auto r=romset::load({"/nonexistent/vf.zip"});
 assert(!r.ok&&r.error.find("as a zip file")!=std::string::npos);
 std::puts("PASS: missing archive is rejected without game data");
}
