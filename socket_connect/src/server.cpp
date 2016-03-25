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


int main()
{
  std::cout << "This is test program.\n";

  bool f = true;

  auto th1 = std::thread([&f]{
	  // スレッドの中の処理
	  
	  int sock0;
	  int sock;
	  struct sockaddr_in addr;
	  struct sockaddr_in client;
	  int len;
	  int ret;
	  
	  // TCPソケットの作成
	  sock0 = socket(AF_INET, SOCK_STREAM, 0);
	  // ソケットの設定
	  addr.sin_family = AF_INET;
	  addr.sin_port = htons(23456);
	  addr.sin_addr.s_addr = INADDR_ANY;
	  
	  // TCPのバインドする(ソケットにアドレスをあてがい名前をつける)
	  ret = bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
	  // TCPクライアントからの接続要求を待てる状態にする(ここでは、5つの接続待ちを許可している)
	  ret = listen(sock0, 5);

	  // データ受信用のUDPスレッドを立てる
	  std::thread updServer = std::thread([&f] {
		  //### 受信用パケット
		  struct sockaddr_in from_addr;
		  struct sockaddr_in addr_udp_r;
		  socklen_t sin_size;
		  int sock_r;
		  int ret = sock_r = socket(AF_INET, SOCK_DGRAM, 0);
		  // ソケットの設定
		  addr_udp_r.sin_family = AF_INET;
		  addr_udp_r.sin_port = htons(34567);
		  addr_udp_r.sin_addr.s_addr = INADDR_ANY;

		  // UDPソケットのバインドする
		  ret = bind(sock_r, (struct sockaddr *)&addr_udp_r, sizeof(addr_udp_r));

		  do{
			//### AIユニットからデータ受ける
			char recv_data[100];
			ret = recvfrom(sock_r, recv_data, sizeof(recv_data), 0, (struct sockaddr *)&from_addr, &sin_size);
			std::cout << " ret:" << ret << ", recv : " << recv_data << std::endl;
		  } while(f);

		  close(sock_r);
		});

	  // スレッドをコンテナに入れるために、コンテナを作成
	  std::vector<std::thread> threadList;
	  do {
		// TCPクライアントからの接続要求を受け付ける
		sock = accept(sock0, (struct sockaddr *)&client, (socklen_t *)&len);

		threadList.push_back( std::thread([&sock]{
			  int ret;
			  /* 5文字送信 */
			  write(sock, "HELLO", 5);
			  /* クライアントから受け取る */
			  char buf[128];
			  memset(buf, 0, sizeof(buf));
			  ret = recv(sock, buf, 5, 0);
			  std::cout << "from client : " <<  buf << std::endl;
			  /* TCPセッションの終了 */
			  close(sock);
			  std::cout << "finish server processing" << std::endl;
			  // 終了する
			  return;
			} ) );
		std::cout << "end inner thread : " << f<< std::endl;
	  } while(f);
	  // threadListのスレッドの停止を待つ
	  for (std::thread &th : threadList)
	  	th.join();
	  std::cout << "end main thread" << std::endl;
	  // listen するsocketの終了
	  close(sock0);
	  //return;
	} );

  // メインのサーバスレッドの外でそのスレッドの管理をするための処理
  std::cout << "============= Test SERVER ==============" << std::endl;
  do {
	std::cout << "enter 'q' to quit." << std::endl;
	auto c = getchar();  // Wait until user hits "enter"
	if( c == 'q' ) {
	  f = false;
	  break;
	}
  } while(true);

  // メインのスレッドの停止を待つ
  th1.join();
  
  std::cout << "end" << std::endl;
  return 0;
}

