#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <exception>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <vector>

#include "server_class.hpp"


DeviceServer::DeviceServer()
{
  retrieved = false;
  retrieved_c = false;
  cmd_flag = true;
  dev_flag = true;
  socket_halt_flag = false;
  cmd = -1;
  cmd_c = -1;
  tcp_port = 50002;
}

DeviceServer::~DeviceServer()
{
}

int DeviceServer::stop()
{
  // DeviceServerを止める
  thd->join();
  delete thd;
  return 0;
}

int DeviceServer::run()
{
  // DeviceServerメインの処理スレッドを立てる
  thd = new std::thread(&DeviceServer::main_thread, this);
  return 0;
}

void DeviceServer::main_thread()
{
  int sock0;
  int sock;
  struct sockaddr_in addr;
  struct sockaddr_in client;
  int ret;
  int len;
  
  // TCPソケットの作成
  sock0 = socket(AF_INET, SOCK_STREAM, 0);
  // ソケットの設定
  addr.sin_family = AF_INET;
  addr.sin_port = htons(tcp_port);
  addr.sin_addr.s_addr = INADDR_ANY;

  std::chrono::milliseconds dura( 1000 );
  // TCPのバインドする(ソケットにアドレスをあてがい名前をつける)
  for(int i=0 ; i<5 ; i++){
	ret = bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
	if(ret == 0) break;
	std::cout << "can't bind " << tcp_port << std::endl;
	std::this_thread::sleep_for( dura );
  }
  // TCPクライアントからの接続要求を待てる状態にする(ここでは、5つの接続待ちを許可している)
  ret = listen(sock0, 5);
  
  // 上位からのコマンド受信用のスレッド生成
  std::thread cmd_thread = std::thread([&sock0](DeviceServer *p) {
	  while(p->cmd_flag) {
		std::cout << "コマンド待ち@cmd_thread" << std::endl;
		std::unique_lock<std::mutex> lock(p->mtx_c);
		p->cond_c.wait(lock, [&]{ return p->retrieved_c; });
		std::cout << "コマンド受信@cmd_thread : cmd=" << p->cmd_c << std::endl;
		if( p->cmd_c == 0 ) {
		  // サーバの停止処理
		  p->socket_halt();
		  p->setDeviceCommand(0);
		  p->cmd_flag = false;
		} else if( p->cmd_c == 9 ) {
		  // dev_threadの停止
		  p->setDeviceCommand(0);
		} else {
		  p->setDeviceCommand(1);
		}
		// コマンドのクリアと待機状態への遷移
		p->cmd_c = -1;
		p->retrieved_c = false;
	  }
	}, this );
  
  // UDPでのデータ通信用スレッド生成
  
  // デバイスとの通信用スレッドの生成
  std::vector<std::thread> threadList;
  do{
	// TCPクライアントからの接続要求を受け付ける
	sock = accept(sock0, (struct sockaddr *)&client, (socklen_t *)&len);
	if(socket_halt_flag) break;
	// TCPクライアントから接続されたらスレッドを立てる
	threadList.push_back( std::thread([&sock](DeviceServer *p){
		  std::cout << "accepted@dev_thread" << std::endl;
		  // デバイスからacceptを受けて、接続要求を受信する
		  std::cout << "接続要求受ける@dev_thread" << std::endl;
		  // 接続要求を受信したことを返す
		  std::cout << "接続要求に返答@dev_thread" << std::endl;

		  std::cout << "スレッド抜けます@dev_thread" << std::endl;
		  close(sock);
		}, this) );
  } while(true);
  
  // threadListのスレッドの停止を待つ
  for (std::thread &th : threadList) th.join();
  cmd_thread.join();
  close(sock0);
  std::cout << "main_thread : finish" << std::endl;
}


void DeviceServer::socket_halt()
{
  // acceptを抜けるために、connectだけしに行く関数
  std::cout << "socketを切ります" << std::endl;
  struct sockaddr_in server;
  int sock;
  // socket切断のフラグを立てる
  socket_halt_flag = true;
  /* ソケットの作成 */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  /* 接続先指定用構造体の準備 */
  server.sin_family = AF_INET;
  server.sin_port = htons(tcp_port);
  server.sin_addr.s_addr = INADDR_ANY;
  /* サーバに接続 */
  connect(sock, (struct sockaddr *)&server, sizeof(server));
  // ソケットの破棄
  close(sock);
}

int DeviceServer::setDeviceCommand(int c)
{
  std::cout << "コマンド設定 : setDeviceCommand : " << c << std::endl;
  cmd = c;
  // デバイス制御スレッドにコマンドを通知する
  std::lock_guard<std::mutex> lock(mtx);
  retrieved = true;
  // 待機状態のスレッドを起こす
  cond.notify_all();
  return 0;
}

int DeviceServer::setCommand(int c)
{
  std::cout << "コマンド設定 : setCommand : " << c << std::endl;
  cmd_c = c;
  // コマンド受信スレッドのロックを解除
  std::lock_guard<std::mutex> lock(mtx_c);
  retrieved_c = true;
  // 待機状態のスレッドを起こす
  cond_c.notify_all();
  return 0;
}


int DeviceServer::setTcpPort(int port)
{
  // 本当は範囲の確認が必要
  tcp_port = port;
  return tcp_port;
}

// Test用のサンプルメソッド
void DeviceServer::impl(int n)
{
  std::cout << "スレッド開始" << std::endl;
  sleep(5);
  std::cout << "スレッド終わり" << std::endl;
}
