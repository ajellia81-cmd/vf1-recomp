#include "../../src/host/bg_saver.h"
#include <cassert>
#include <cstdio>
int main(){
 std::string saved;
 bg_saver saver([&](const std::string&,const uint8_t *p,size_t n){saved.assign(reinterpret_cast<const char*>(p),n);return true;});
 saver.put("state","before",6);saver.flush();assert(saved=="before");
 saver.put("state","after",5);saver.flush();assert(saved=="after");
 saver.finish();saver.finish();
 std::puts("PASS: autosave drain completes and accepts new writes after resume");
}
