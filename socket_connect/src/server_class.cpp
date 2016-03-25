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


// コンストラクタ
device_server::device_server(){
  std::cout << "コンストラクタ" << std::endl;
}
// デストラクタ
device_server::~device_server(){
  std::cout << "デストラクタ" << std::endl;
}

//void sl(){
//  std::cout << "スレッド開始" << std::endl;
//  sleep(5);
//  std::cout << "スレッド終わり" << std::endl;
//}

void device_server::Activate()
{
  //
  auto th1 = std::thread([]{
	  std::cout << "スレッド開始" << std::endl;
	  sleep(5);
	  std::cout << "スレッド終わり" << std::endl;
	});
  th1.join();
}

// テスト描画
void device_server::disp() {
  std::cout << "str : aaaa" << std::endl;
}
  

