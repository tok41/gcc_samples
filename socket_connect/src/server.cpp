#include <iostream>
#include <sstream>
#include <thread>
#include <string.h>

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>

int main()
{
	
	std::thread serverThread = std::thread([](){
			int sock0;
			int sock;
			struct sockaddr_in addr;
			struct sockaddr_in client;
			int ret;
			int len;
			int flag = 0;
			
			// ***** TCPソケットの作成
			int port = 90000;
			sock0 = socket(AF_INET, SOCK_STREAM, 0);
			addr.sin_family = AF_INET;
			addr.sin_port = htons(port);
			addr.sin_addr.s_addr = INADDR_ANY;
			// TCPのバインドする(ソケットにアドレスをあてがい名前をつける)
			ret = bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
			if(ret |= 0) {
				std::cout << "can't bind : " << port << std::endl;
				exit(-1);
			}
			// TCPクライアントからの接続要求を待てる状態にする(ここでは、5つの接続待ちを許可している)
			ret = listen(sock0, 5);
			ret = setsockopt( sock0, SOL_SOCKET, TCP_NODELAY, (char *)&flag, sizeof(flag) );
			if ( ret != 0 ) { std::cout << "error setsockopt" << std::endl; }

			// ***** 通信の開始
			// TCPクライアントからの接続要求を受け付ける
			sock = accept(sock0, (struct sockaddr *)&client, (socklen_t *)&len);

			// クライアントにmsgを投げる
			char c_str[] = "Hello 1";
			len = write(sock, c_str, sizeof(c_str));
			std::cout << "send : "<<c_str<<", "<<len<<std::endl;

			// クライアントからmsgを受ける
			char msg[256];
			len = recv(sock, msg, sizeof(msg), 0);
			std::cout << "recv : "<<msg<<", "<<len<<std::endl;

			sleep(1);
			len = send(sock, c_str, sizeof(c_str), 0);
			std::cout << "send : "<<c_str<<", "<<len<<std::endl;

			// ***** ソケットの破棄
			close(sock);
			close(sock0);
		});
	serverThread.join();
	
	return 0;
}

