#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/stat.h>
#include <netdb.h>
#include <unistd.h>

#include <stdio.h>

#include <arpa/inet.h>
#include <string.h>

using namespace std;

int main()
{
  struct sockaddr_in server;
  int sock;
  char buf[32];
  int n;

  // TCP用
  /* ソケットの作成 */
  sock = socket(AF_INET, SOCK_STREAM, 0);
  /* 接続先指定用構造体の準備 */
  server.sin_family = AF_INET;
  server.sin_port = htons(50002);
  server.sin_addr.s_addr = inet_addr("127.0.0.1");

  /* サーバに接続 */
  connect(sock, (struct sockaddr *)&server, sizeof(server));

  ///* サーバからデータを受信 */
  //memset(buf, 0, sizeof(buf));
  //n = read(sock, buf, sizeof(buf));
  //printf("%d, %s\n", n, buf);
  //
  //// データの送信
  //write(sock, "HELLO", 5);

  /* socketの終了 */
  close(sock);

  return 0;
}
