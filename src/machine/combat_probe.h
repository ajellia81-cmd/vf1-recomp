#pragma once
#include <array>
#include <cstdint>
#include <cstdio>
// Bounded scalar observations, without allocation or I/O in emulation.
class combat_probe {
 struct Row { uint64_t frame; uint32_t pc; int player,before,after,mode,round,live; };
 std::array<Row,2048> rows{}; unsigned next=0,count=0;
public:
 void record(uint64_t f,uint32_t pc,int player,int before,int after,int mode,int round,int live) {
  if (before==after && before>0) return;
  rows[next]={f,pc,player,before,after,mode,round,live};
  next=(next+1)%rows.size(); if(count<rows.size())++count;
 }
 bool save(const char *path) const {
  FILE *f=fopen(path,"w"); if(!f)return false;
  fprintf(f,"# Scalar energy-store observations; not confirmed hit events. No ROM or memory dump.\nframe,pc_hex,player,before,after,mode,round,live\n");
  for(unsigned i=0;i<count;++i) { const auto &r=rows[(next+rows.size()-count+i)%rows.size()];
   fprintf(f,"%llu,%08x,%d,%d,%d,%d,%d,%d\n",(unsigned long long)r.frame,r.pc,r.player,r.before,r.after,r.mode,r.round,r.live);
  }
  bool ok=!ferror(f); return fclose(f)==0 && ok;
 }
};
