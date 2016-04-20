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
	struct sockaddr_in server;
	int sock;
	char buf[256];
	int n;

	// TCP用
	int port = 90000;
	//ソケットの作成
	sock = socket(AF_INET, SOCK_STREAM, 0);
	// 接続先指定用構造体の準備 
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr.s_addr = inet_addr("127.0.0.1");

	// サーバに接続
	connect(sock, (struct sockaddr *)&server, sizeof(server));

	// サーバからデータを受信
	n = read(sock, buf, sizeof(buf));
	printf("%d, %s\n", n, buf);

	//// データの送信
	//char msg[] = "HELLO_frm_client";
	//n = write(sock, msg, sizeof(msg));
	//std::cout<<"send(cl) : "<<msg<<", "<<n<<std::endl;

	close(sock);
  
	return 0;
}
