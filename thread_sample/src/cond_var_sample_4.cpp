// http://yohhoy.hatenablog.jp/entry/2014/09/23/193617

#include <queue>
#include <thread>
#include <mutex>
#include <iostream>
#include <unistd.h>
#include <condition_variable>

class mt_queue {
  static const int capacity = 3;
  std::queue<int> q_;
  std::mutex mtx_;
  std::condition_variable cv_;
public:
  void push(int data)
  {
	std::unique_lock<std::mutex> lk(mtx_);
	while (q_.size() == capacity) {
	  std::cout << "q_size1 : " << q_.size() << std::endl;
	  cv_.wait(lk);
	}
	q_.push(data);
	cv_.notify_all();
  }
  int pop()
  {
	std::unique_lock<std::mutex> lk(mtx_);
	while (q_.empty()) {
	  cv_.wait(lk);
	}
	int data = q_.front();
	q_.pop();
	cv_.notify_all();
	
	return data;
  }
};


//----------------------------------------------------------
const int N = 10;

int main()
{
  mt_queue q;
  
  std::thread th1([&]{
	  for (int i = 1; i <= N; ++i) {
		q.push(i);
		usleep(100);
	  }
	  q.push(-1);  // end of data
	  std::cout << "push終わり" << std::endl;
	});
  
  std::thread th2([&]{
	  int v;
	  while ((v = q.pop()) > 0) {
		std::cout << v << std::endl;
		usleep(100000);
	  }
	  std::cout << "(EOD)" << std::endl;
	});
  
  th1.join();
  th2.join();
}
