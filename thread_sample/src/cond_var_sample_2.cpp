#include <mutex>
#include <condition_variable>
#include <atomic>
#include <iostream>
#include <thread>

std::mutex mtx;
std::condition_variable cond;
bool retrieved = false;

// スレッド関数の定義
void command_thread(){
  std::cout << "start : cmd_thread" << std::endl;
  std::cout << "wait : cmd_thread" << std::endl;
  // 待つ
  std::unique_lock<std::mutex> lock(mtx);
  cond.wait(lock, [&]{ return retrieved; });
  // コマンド受信後の処理
  std::cout << "something_process : cmd_thread" << std::endl;
  std::this_thread::sleep_for(std::chrono::seconds(3));
  std::cout << "finish : cmd_thread" << std::endl;
}

void main_thread(){
  std::cout << "start : main_thread" << std::endl;
  //
  std::cout << "something_process : main_thread" << std::endl;
  //std::this_thread::sleep_for(std::chrono::seconds(10));
  getchar();
  std::cout << "something_process(finish) : main_thread" << std::endl;
  //std::unique_lock<std::mutex> lock(mtx);
  std::lock_guard<std::mutex> lock(mtx);
  // コマンドを通知する
  std::cout << "notify : main_thread" << std::endl;
  retrieved = true;
  // 待機状態のスレッドを起こす
  cond.notify_one();
  std::cout << "finish : main_thread" << std::endl;
}


int main(int argc, char const* argv[]){
  std::thread th_main([]{
	  main_thread();
	});
  std::thread th_cmd([]{
	  command_thread();
	});

  th_cmd.join();
  th_main.join();
  
  return 0;
}
