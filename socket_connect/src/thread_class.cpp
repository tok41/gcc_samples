
#include <iostream>
#include <thread>
#include <string.h>
#include <unistd.h>

#include "thread_class.hpp"

void C::run()
{
  std::thread thd(&C::impl, this, 10);
  thd.join();
}

void C::impl(int n)
{
  std::cout << "スレッド開始" << std::endl;
  sleep(5);
  std::cout << "スレッド終わり" << std::endl;
}
