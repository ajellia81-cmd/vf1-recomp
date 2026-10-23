#include "combat_probe.h"
#include <cassert>
#include <string>
#include <fstream>
#include <unistd.h>
int main() {
 combat_probe p; char path[]="/tmp/vf-combat-probe-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);
 for(int i=0;i<2100;++i)p.record(i,0x123456,1,0,0,9,9,2);
 p.record(9999,0x123456,1,10,10,9,9,2); // unchanged positive stores are omitted
 assert(p.save(path));std::ifstream f(path);std::string line;unsigned count=0;std::string first,last;
 while(std::getline(f,line)){if(count==2)first=line;last=line;++count;}
 assert(count==2050 && first.starts_with("52,") && last.starts_with("2099,"));unlink(path);
 assert(!p.save("/nonexistent-vf-diagnostics/path"));
}
