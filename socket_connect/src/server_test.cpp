#include <iostream>
#include <thread>
#include <exception>

#include <mutex>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string.h>

#include <netdb.h>
#include <unistd.h>

#include <vector>

#include "server_class.hpp"

int main()
{
  std::cout << "mainスレッド開始" << std::endl;
  
  // デバイスサーバクラスのインスタンスを生成
  device_server ds;
  
  ds.disp();

  ds.Activate();

  std::cout << "mainスレッド終了" << std::endl;
  
  return 0;
}
