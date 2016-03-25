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
  server.sin_port = htons(23456);
  server.sin_addr.s_addr = inet_addr("127.0.0.1");

  // UDP用
  /* ソケットの作成 */
  struct sockaddr_in addr_udp;
  int sockU;
  int ret = sockU = socket(AF_INET, SOCK_DGRAM, 0);
  /* ソケットの設定 */
  addr_udp.sin_family = AF_INET;
  addr_udp.sin_port = htons(34567);
  addr_udp.sin_addr.s_addr = INADDR_ANY;
  /* UDPで何か送る */
  char test_data[] = "UDP_TEST";
  sendto(sockU, test_data, sizeof(test_data), 0, (struct sockaddr *)&addr_udp, sizeof(addr_udp));

  /* サーバに接続 */
  connect(sock, (struct sockaddr *)&server, sizeof(server));

  /* サーバからデータを受信 */
  memset(buf, 0, sizeof(buf));
  n = read(sock, buf, sizeof(buf));
  printf("%d, %s\n", n, buf);

  // データの送信
  write(sock, "HELLO", 5);

  /* UDPで何か送る */
  char test_data2[] = "UDP_TEST2";
  sendto(sockU, test_data2, sizeof(test_data2), 0, (struct sockaddr *)&addr_udp, sizeof(addr_udp));

  /* socketの終了 */
  close(sock);

  return 0;
}
