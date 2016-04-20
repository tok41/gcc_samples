#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iostream>


int main()
{
	int sock0;
	int sock;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	int ret;
	int len;
	fd_set fds, readfds; // select用
	int max_fd = 0;
	struct timeval tv;
	
	// ##### TCPソケットの作成
	sock0 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(11111);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	// TCPのバインドする(ソケットにアドレスをあてがい名前をつける)
	ret = bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
	std::cout<<"bind ret : "<<ret << std::endl;
	ret = listen(sock0, 5);
	std::cout<<"listen ret : "<<ret <<std::endl;

	// TCPクライアントからの接続要求を受け付ける
	sock = accept(sock0, (struct sockaddr *)&client, (socklen_t *)&len);

	
	// fd_setの初期化
	FD_ZERO(&readfds);
	FD_SET(sock, &readfds);
	max_fd = sock;
	std::cout << "max_fd : " << max_fd << ", sock_fd : " << sock << std::endl;

	char buf[256];
	while( true ) {
		// receive
		tv.tv_sec = 3;
		tv.tv_usec = 0;
		memcpy(&fds, &readfds, sizeof(fd_set));
		ret = select(max_fd+1, &fds, NULL, NULL, &tv);
		std::cout << "select ret : " << ret << std::endl;
		if(ret > 0 && FD_ISSET(sock, &fds) ) {
			len = recv(sock, buf, sizeof(buf), 0);
			if (len > 0) {
				std::cout << "receive_msg : "<< buf << ", size="<< len << std::endl;
			} else {
				//break;
			}
		}

		tv.tv_sec = 3;
		tv.tv_usec = 0;
		//memcpy(&fds, &readfds, sizeof(fd_set));
		ret = select(max_fd+1, NULL, &fds, NULL, &tv);
		std::cout << "select ret write : " << ret << std::endl;
		if(ret > 0 && FD_ISSET(sock, &fds) ) {
			char tmp[] = "received!";
			len = send(sock, tmp, sizeof(tmp), 0);
			std::cout << "write len : " << len << std::endl;
			//if (len < 1) break;
		}
		
	}

	std::cout << "close" << std::endl;
	close(sock);
	close(sock0);
	
	return 0;
}
