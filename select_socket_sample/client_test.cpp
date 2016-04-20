#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
	struct sockaddr_in server;
	int sock;
	char buf[256];
	int n;
	std::string s;
	
	/* ソケットの作成 */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	/* 接続先指定用構造体の準備 */
	server.sin_family = AF_INET;
	server.sin_port = htons(11111);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");
	
	/* サーバに接続 */
	connect(sock, (struct sockaddr *)&server, sizeof(server));
	std::cout << "connect" << std::endl;

	bool f = true;
	while(f){
		std::cout << "入力してください : ";
		std::cin >> s;
		if(s=="q")
			{
				break;
			}
		else {
			std::cout << s << ", " << s.length();
			n = write(sock, s.c_str(), s.length() );
			std::cout << ", write_n=" << n << std::endl;

			n = recv(sock, buf, sizeof(buf), 0);
			std::cout << "recv msg : " << buf << std::endl;
		}
	}
	
	close(sock);

	return 0;
}
