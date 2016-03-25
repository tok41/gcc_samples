#ifndef _INC_SERV
#define _INC_SERV

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <exception>
#include <string.h>
#include <unistd.h>

class DeviceServer {
public:
  DeviceServer();
  ~DeviceServer();
  int stop();
  int run();
  int setCommand(int c);
  int setTcpPort(int port);
  void impl(int n); // 後で消す
  int tcp_port;
private:
  void main_thread();
  int setDeviceCommand(int c);
  std::mutex mtx;
  std::mutex mtx_c;
  std::condition_variable cond;
  std::condition_variable cond_c;
  std::thread *thd;
  bool retrieved;
  bool retrieved_c;
  int cmd;
  int cmd_c;
  bool cmd_flag;
  bool dev_flag;
};

#endif
