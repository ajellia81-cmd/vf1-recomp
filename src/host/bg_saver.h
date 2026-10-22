#pragma once
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

// Keep the writer alive across suspension: flush drains it; finish is for exit.
struct bg_saver {
 using writer = std::function<bool(const std::string &,const uint8_t *,size_t)>;
 writer write;
 std::mutex mx;
 std::condition_variable cv;
 std::map<std::string,std::vector<uint8_t>> jobs;
 bool stop=false,writing=false;
 std::thread th;
 explicit bg_saver(writer w):write(std::move(w)),th([this]{run();}){}
 void put(const std::string &path,const void *src,size_t n){
  {std::lock_guard<std::mutex> l(mx);if(stop)return;
   jobs[path].assign(static_cast<const uint8_t*>(src),static_cast<const uint8_t*>(src)+n);}
  cv.notify_all();
 }
 void run(){
  std::unique_lock<std::mutex> l(mx);
  for(;;){
   cv.wait(l,[&]{return stop||!jobs.empty();});
   if(jobs.empty())return;
   auto todo=std::move(jobs);jobs.clear();writing=true;l.unlock();
   for(auto &[path,bytes]:todo)write(path,bytes.data(),bytes.size());
   l.lock();writing=false;cv.notify_all();
  }
 }
 void flush(){std::unique_lock<std::mutex> l(mx);cv.wait(l,[&]{return jobs.empty()&&!writing;});}
 void finish(){
  {std::lock_guard<std::mutex> l(mx);stop=true;}
  cv.notify_all();if(th.joinable())th.join();
 }
 ~bg_saver(){finish();}
};
