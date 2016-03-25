#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <thread>
#include <string.h>
#include <unistd.h>

std::mutex mtx1;
std::mutex mtx2;

std::condition_variable cond;
std::condition_variable cond2;

bool retrieved1 = false;
bool retrieved2 = false;

bool f1 = true;
bool f2 = true;

void do_worker1 (int idx) {
  while( f1 ) {
	std::unique_lock<std::mutex> lock( mtx1 ); // mutex獲る
	cond.wait(lock); // 待つ
	
	for(int i=0 ; i<3 ; i++){
	  std::cout << idx << " : " << i << std::endl;
	  sleep(1);
	}
	cond.notify_all(); // 作業終わったので通知
  }
}

void do_worker2 (int idx) {
  while( f2 ) {
	std::unique_lock<std::mutex> lock( mtx2 );
	cond2.wait(lock); // 待つ
	
	for(int i=0 ; i<3 ; i++){
	  std::cout << idx << " : " << i << std::endl;
	  sleep(1);
	}
	cond2.notify_all(); // 作業終わったので通知
  }
}

int main(int argc, char const* argv[]){
  std::thread t1(do_worker1, 1);
  std::thread t2(do_worker2, 2);

  std::string s;
  std::cin >> s;
  std::cout << "入力文字 : " << s << std::endl;
  cond.notify_all();
  while (true ) {
	std::cin >> s;
	std::cout << "入力文字 : " << s << std::endl;
	//cond2.notify_all();

	if(s=="q") {
	  f1 = false;
	  f2 = false;
	  cond.notify_all();
	  cond2.notify_all();
	  break;
	} else if(s=="1") {
	  cond.notify_all();
	} else {
	  cond2.notify_all();
	}
	
  }
  
  std::cout << "スレッド破棄待ち" << std::endl;
  t1.join();
  t2.join();
  
  return 0;
}
