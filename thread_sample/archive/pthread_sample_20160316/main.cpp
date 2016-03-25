#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>

#include "MyPThread.h"

int main(int argc, char *argv[])
{
  MyPThread thread;

  int execTimes = 0;
  while (true) {
	if (execTimes > 5) {
	  break;
	}

	//      thread.clear();             // 非同期版
	thread.clearWithMutex();    // 同期版

	execTimes++;
	sleep(3);
  }

  thread.stop();
  std::cout << "exit" << std::endl;

  //    return a.exec();
  return 0;
}
