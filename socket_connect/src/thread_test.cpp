#include <iostream>
#include <thread>
#include <string.h>
#include <unistd.h>

#include "thread_class.hpp"

//class C {
//public:
//  // blah blah blah
//  void run()
//  {
//	std::thread thd(&C::impl, this, 10);
//	thd.join();
//  }
//  void impl(int n)
//  {
//	//do something
//	std::cout << "スレッド開始" << std::endl;
//	sleep(5);
//	std::cout << "スレッド終わり" << std::endl;
//  }
//};

int main()
{
  C c;
  c.run();
  return 0;
}
