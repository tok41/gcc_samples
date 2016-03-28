#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <thread>
#include <string.h>
#include <unistd.h>
#include <map>

std::map<int, std::mutex*> mtx_map;

std::map<int, std::condition_variable*> cond_map;

bool retrieved1 = false;
bool retrieved2 = false;

bool f1 = true;
bool f2 = true;

void do_worker1 (int idx) {
  while( f1 ) {
	std::unique_lock<std::mutex> lock( *(mtx_map[1]) ); // mutex獲る
	cond_map[1]->wait(lock); // 待つ
	
	for(int i=0 ; i<3 ; i++){
	  std::cout << idx << " : " << i << std::endl;
	  sleep(1);
	}
	cond_map[1]->notify_all(); // 作業終わったので通知
  }
}

void do_worker2 (int idx) {
  while( f2 ) {
	std::unique_lock<std::mutex> lock( *(mtx_map[2]) );
	cond_map[2]->wait(lock); // 待つ
	
	for(int i=0 ; i<3 ; i++){
	  std::cout << idx << " : " << i << std::endl;
	  sleep(1);
	}
	cond_map[2]->notify_all(); // 作業終わったので通知
  }
}

int main(int argc, char const* argv[]){
  mtx_map[1] = new std::mutex;
  mtx_map[2] = new std::mutex;
  cond_map[1] = new std::condition_variable;
  cond_map[2] = new std::condition_variable;
  std::thread t1(do_worker1, 1);
  std::thread t2(do_worker2, 2);

  std::string s;
  std::cin >> s;
  std::cout << "入力文字 : " << s << std::endl;
  cond_map[1]->notify_all();
  while (true ) {
	std::cin >> s;
	std::cout << "入力文字 : " << s << std::endl;
	//cond2.notify_all();

	if(s=="q") {
	  f1 = false;
	  f2 = false;
	  cond_map[1]->notify_all();
	  cond_map[2]->notify_all();
	  break;
	} else if(s=="1") {
	  cond_map[1]->notify_all();
	} else {
	  cond_map[2]->notify_all();
	}
	
  }
  
  std::cout << "スレッド破棄待ち" << std::endl;
  t1.join();
  t2.join();
  
  return 0;
}
