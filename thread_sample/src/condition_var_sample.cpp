#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <thread>

std::mutex mtx;
std::condition_variable cv;
bool is_ready = false; // for spurious wakeup

void do_preparing_process(){
  std::cout << "Start Preparing" << std::endl;
  // preparing
  // ... σ(^_^;)ｱｾｱｾ...
  std::this_thread::sleep_for(std::chrono::seconds(3));
  // finish preparing
  std::cout << "Finish Preparing" << std::endl;;
  {
	std::lock_guard<std::mutex> lock(mtx);
	is_ready = true;
  }
  cv.notify_one();
}

void do_main_process(){
  std::this_thread::sleep_for(std::chrono::seconds(1));
  std::cout << "Start Main Thread" << std::endl;
  {
	std::unique_lock<std::mutex> uniq_lk(mtx); // ここでロックされる
	cv.wait(uniq_lk, []{ return is_ready;});
	// 1. uniq_lkをアンロックする
	// 2. 通知を受けるまでこのスレッドをブロックする
	// 3. 通知を受けたらuniq_lkをロックする

	/* ここではuniq_lkはロックされたまま */

  } // デストラクタでアンロックする
  std::cout << "Finish Main Thread" << std::endl;
}



int main(int argc, char const* argv[])
{
  std::thread th_prepare([&]{ do_preparing_process(); });
  std::thread th_main([&]{ do_main_process(); });

  th_prepare.join();
  th_main.join();

  return 0;
}

