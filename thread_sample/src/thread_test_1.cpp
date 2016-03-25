#include <iostream>
#include <thread>
#include <exception>
#include <unistd.h>
#include <mutex>
#include <map>
#include <memory>

std::map<int, std::mutex* > mtx_map;

void do_worker1 (int idx) {
  std::lock_guard<std::mutex> lock( *(mtx_map[1]) );
  for(int i=0 ; i<3 ; i++){
	std::cout << idx << " : " << i << std::endl;
	sleep(1);
  }
}

int main (int argc, char *argv[]) {
  try {
	mtx_map[1] = new std::mutex;
	
	std::thread t1(do_worker1, 1);
	std::thread t2(do_worker1, 2);
	
	t1.join();
	t2.join();

	for(auto& x:mtx_map) {
	  std::cout << x.first << " delete" << std::endl;
	  delete x.second;
	}
  } catch (std::exception &ex) {
	std::cerr << ex.what() << std::endl;
  }
  
  return 0;
}


