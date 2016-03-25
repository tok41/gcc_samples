#include <iostream>
#include <thread>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include "server_class.hpp"


int main()
{
  DeviceServer c;
  std::cout << "start : test_main" << std::endl;
  c.run();

  int ret = -1;
  std::string s;

  bool f = true;
  while(f){
	std::cout << "コマンドを入力してください : ";
	std::cin >> s;
	if(s=="q") {
	  ret = c.setCommand(0);
	  break;
	}
	try
	  {
		int num = std::stoi(s);
		std::cout << "input num > " << num << std::endl;
		ret = c.setCommand(num);
	  }
	catch(...)
	  {
		std::cout << "コマンドを数値で入力してください" << std::endl;
	  }
  }
  
  // サーバ終了
  ret = c.stop();
  
  std::cout << "finish : test_main " << ret << std::endl;
  return 0;
}
