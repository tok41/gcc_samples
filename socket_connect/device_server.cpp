#include <iostream>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <exception>
#include <iomanip>
#include <arpa/inet.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <vector>
#include <map>
#include <ctime>
#include <time.h>
#include <sstream>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include "picojson.h"

#include "NN_parameters.hpp"
extern "C" {
#include "device_server.h"
}
#include "device_server.hpp"


void set_command_packet(command_packet *cmd, int data_size, unsigned char type, unsigned char rw){
	/*
	  パケットのヘッダを作る
	  type : コマンド(16進数表現)
	  rx : Read/Write(0x00:Write, 0x01:Read)
	*/
	cmd->header1[0] = 0x52;
	cmd->header1[1] = 0x45;
	cmd->header1[2] = 0x41;
	cmd->header1[3] = 0x49;
	memcpy(&cmd->header2[0], &data_size, 4);
	cmd->header2[4] = type;
	cmd->header2[5] = rw;
	cmd->header2[6] = 0x00;
	cmd->header2[7] = 0x00;
}

unsigned char DeviceServer::initialize_request = 0x01;
unsigned char DeviceServer::authorization = 0x02;
unsigned char DeviceServer::disconnect = 0x04;
unsigned char DeviceServer::send_sensor_data = 0x10;
unsigned char DeviceServer::req_setting_param = 0x20;
unsigned char DeviceServer::send_setting_param = 0x21;
unsigned char DeviceServer::send_ai_param = 0x30;
unsigned char DeviceServer::send_alarm = 0x40;
unsigned char DeviceServer::debug_mode_on = 0x80;
unsigned char DeviceServer::debug_mode_off = 0x81;
unsigned char DeviceServer::data_mode_on = 0x82;
unsigned char DeviceServer::data_mode_off = 0x83;
unsigned char DeviceServer::alarm_mode_on = 0x84;
unsigned char DeviceServer::alarm_mode_off = 0x85;
unsigned char DeviceServer::send_setting_alarm = 0x86;
unsigned char DeviceServer::err_auth = 0xF0;
unsigned char DeviceServer::err_packet = 0xF1;
unsigned char DeviceServer::err_param = 0xF2;
unsigned char DeviceServer::err = 0xF3;

unsigned char DeviceServer::Registering = 0x00;
unsigned char DeviceServer::Collecting = 0x01;
unsigned char DeviceServer::Monitoring = 0x02;
unsigned char DeviceServer::Stopped = 0x03;
unsigned char DeviceServer::Waiting = 0x04;
unsigned char DeviceServer::Disconnected = 0x05;
unsigned char DeviceServer::Busy = 0x06;

DeviceServer::DeviceServer()
{
	// constructor
	thd = NULL;
	socket_halt_flag = false;
	debug_mode_ip = "127.0.0.1";
	//debug_mode_ip = "192.168.1.100";
	debug_mode_tcp_port = 50004;

	//google::InitGoogleLogging(argv[0]);
	//gflags::ParseCommandLineFlags(&argc, &argv, true);
	//time_t now = time(NULL);
	//struct tm *pnow = localtime(&now);
	//char buffer[256];
	//snprintf(buffer, sizeof(buffer), "%02d%02d%02d_%02d%02d%02d"
	//		 , pnow->tm_year+1900-2000
	//		 , pnow->tm_mon + 1
	//		 , pnow->tm_mday
	//		 , pnow->tm_hour
	//		 , pnow->tm_min
	//		 , pnow->tm_sec);

	s_data = NULL;
}

DeviceServer::~DeviceServer()
{
	// destructor
}

void DeviceServer::stop()
{
	if( thd != NULL) {
		thd->join();
		delete thd;
	}
	if(s_data != NULL) delete[] s_data;
}

bool DeviceServer::run(std::function<void(void)> l_f, int port_n, int udp_sensor_port, int udp_alarm_port)
{
	PORT_DEVICE = port_n;
	PORT_SENSOR = udp_sensor_port;
	PORT_ALARM = udp_alarm_port;
	// DeviceServerメインの処理スレッドを立てる
	try {
		//thd = new std::thread(&DeviceServer::main_thread, this, l_f, port_n, udp_sensor_port, udp_alarm_port);
		thd = new std::thread(&DeviceServer::main_thread, this, l_f);
	} catch (std::exception &ex) {
		thd = NULL;
		LOG(ERROR) << "thread can't open";
		return false;
	}
	return true;
}

//void DeviceServer::main_thread(std::function<void(void)> debug_f, int FLAGS_tcp_port, int udp_sensor_port, int udp_alarm_port)
void DeviceServer::main_thread(std::function<void(void)> debug_f)
{
	int sock0;
	int sock;
	struct sockaddr_in addr;
	struct sockaddr_in client;
	int ret;
	int len;
	fd_set readfds; // select用
	int max_fd = 0;
	std::chrono::milliseconds dura( 1000 );

	// ##### TCPソケットの作成
	sock0 = socket(AF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT_DEVICE);
	addr.sin_addr.s_addr = INADDR_ANY;

	// TCPのバインドする(ソケットにアドレスをあてがい名前をつける)
	for(int i=0 ; i<5 ; i++){
		ret = bind(sock0, (struct sockaddr *)&addr, sizeof(addr));
		if(ret == 0) break;
		LOG(ERROR) << "can't bind " << PORT_DEVICE << "port(" << strerror( errno ) << ")";
		std::chrono::milliseconds dura( 1000 );
		std::this_thread::sleep_for( dura );
		if( i < 5-1  ) { continue; }
		LOG(FATAL) << "can't bind";
		exit(-1);
	}
	// TCPクライアントからの接続要求を待てる状態にする(ここでは、5つの接続待ちを許可している)
	ret = listen(sock0, 5);
	// set sockopt
	int flag = 0;
	//ret = setsockopt( sock0, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag) );
	ret = setsockopt( sock0, SOL_SOCKET, TCP_NODELAY, (char *)&flag, sizeof(flag) );
	if ( ret != 0 ) {
		std::cout << "error setsockopt" << std::endl;
		LOG(INFO) << "error setsockopt";
	}

	// ##### UDPでのデータ受信用スレッドの作成 (センサデータの送受信用)
	int port_s = PORT_SENSOR;
	std::thread updServer = std::thread([&port_s](std::mutex  & mtx) {
			//### 受信用パケット
			socklen_t sin_size;
			struct sockaddr_in from_addr;
			struct sockaddr_in addr_udp_r;
			int sock_r;
			int ret = sock_r = socket(AF_INET, SOCK_DGRAM, 0);
			// ソケットの設定
			addr_udp_r.sin_family = AF_INET;
			addr_udp_r.sin_port = htons(port_s);
			addr_udp_r.sin_addr.s_addr = INADDR_ANY;

			// UDPソケットのバインドする
			ret = bind(sock_r, (struct sockaddr *)&addr_udp_r, sizeof(addr_udp_r));
			if( 0 != ret ) {
				LOG(ERROR) << "can't listen UDP port (for sensor)";
				exit(-1);
			}

			int idx = 0;
			do{
				//### AIユニットからデータ受ける
				// 今回は疎通試験なのですべて読み捨て
				// 本来はUDPパケットの送信元を確認してTCPのスレッドへデータを引き渡す
				unsigned char recv_data[100];
				ret = recvfrom(sock_r, recv_data, sizeof(recv_data), 0, (struct sockaddr *)&from_addr, &sin_size);
				//std::cout<<"デバイスからデータ受信(UDP_sensor) : packet_size="<<ret<<std::endl;
				LOG(INFO)<<"デバイスからデータ受信(UDP_sensor) : packet_size="<<ret;
				if( recv_data[8]==0x04 ) {
					//std::cout<<"UDP_sensor 切断"<<std::endl;
					LOG(INFO)<<"UDP_sensor 切断";
					break;
				}
				//for(int i=0 ; i<ret ; i++) {
				//	printf("%02x ", recv_data[i]);
				//}
				//printf("\n");
			} while(true);
			close(sock_r);
		},
		std::ref(mtx)
		);
	// ##### UDPでのデータ受信用スレッドの作成 (監視データの送受信用)
	int port_a = PORT_ALARM;
	std::thread updServer_a = std::thread([&port_a](std::mutex & mtx) {
			//### 受信用パケット
			socklen_t sin_size;
			struct sockaddr_in from_addr;
			struct sockaddr_in addr_udp_r;
			int sock_r;
			int ret = sock_r = socket(AF_INET, SOCK_DGRAM, 0);
			// ソケットの設定
			addr_udp_r.sin_family = AF_INET;
			addr_udp_r.sin_port = htons(port_a);
			addr_udp_r.sin_addr.s_addr = INADDR_ANY;

			// UDPソケットのバインドする
			ret = bind(sock_r, (struct sockaddr *)&addr_udp_r, sizeof(addr_udp_r));
			if( 0 != ret ) {
				LOG(ERROR) << "can't listen UDP port (for alarm data) ";
				exit(-1);
			}

			int idx = 0;
			do{
				//### AIユニットからデータ受ける
				// 今回は疎通試験なのですべて読み捨て
				// 本来はUDPパケットの送信元を確認してTCPのスレッドへデータを引き渡す
				unsigned char recv_data[100];
				ret = recvfrom(sock_r, recv_data, sizeof(recv_data), 0, (struct sockaddr *)&from_addr, &sin_size);
				//std::cout<<"デバイスからデータ受信(UDP_alarm) : packet_size="<<ret<<std::endl;
				LOG(INFO)<<"デバイスからデータ受信(UDP_alarm) : packet_size="<<ret;
				if( recv_data[8]==0x04 ) {
					//std::cout<<"UDP_alarm 切断"<<std::endl;
					LOG(INFO)<<"UDP_alarm 切断";
					break;
				}
				//for(int i=0 ; i<ret ; i++) {
				//	printf("%02x ", recv_data[i]);
				//}
				//printf("\n");
			} while(true);
			close(sock_r);
		},
		std::ref(mtx)
		);

	// fd_setの初期化
	FD_ZERO(&readfds);

	// デバイスとの通信用スレッドの生成
	std::vector<std::thread> threadList;
	do{
		// TCPクライアントからの接続要求を受け付ける
		sock = accept(sock0, (struct sockaddr *)&client, (socklen_t *)&len);
		if(socket_halt_flag) break;
		// selectで読み込むソケットとして登録
		FD_SET(sock, &readfds);
		if(max_fd < sock) max_fd = sock;
		// TCPクライアントから接続されたらスレッドを立てる
		threadList.push_back( std::thread([&sock, &readfds, &max_fd, &debug_f, &client](DeviceServer *p) {
					// IDを割り当てる
					thread_local int device_id = 0;
					p->mtx.lock();
					{
						// mapping ip-device_id
						char ip[INET_ADDRSTRLEN];
						memcpy(ip, inet_ntoa(client.sin_addr), INET_ADDRSTRLEN);
						printf( "Client IP %s \n", ip );
						if( p->device_id_map.size() > 0 ) {
							device_id = p->device_ids[p->device_ids.size()-1] + 1;
							for (auto& x:p->device_id_map) {
								std::cout << x.first << " : " << x.second << std::endl;
								if( x.second == ip ) {
									device_id = x.first;
									break;
								}
							}
							p->device_id_map[device_id] = ip;
						} else {
							device_id = 1;
							p->device_id_map[device_id] = ip;
						}
						std::cout << "決定したデバイスID : " << device_id << std::endl;
						p->device_ids.push_back(device_id);
						// device毎のsocket,mutex,cond_variableを追加
						p->sock_map[device_id] = sock;
						p->mtx_map[device_id] = new std::mutex;
						p->cond_map[device_id] = new std::condition_variable;
						p->cmd_map[device_id] = "00";
						p->ready_map[device_id] = false;
						// buffer系
						p->buf_map[device_id] = NULL;
						p->ndata_map[device_id] = 0;
						p->body_map[device_id] = NULL;
						p->param_size[device_id] = 0;
						p->device_status[device_id] = DeviceServer::Waiting;
						p->debug_mode_flag[device_id] = false;
						// setting_parameter
						p->p_sensor_map[device_id] = new ParamsSensorSetting();
						p->p_trigger_map[device_id] = new ParamsTriggerSetting();
						p->p_ai_map[device_id] = new ParamsAISetting();
						p->p_data_map[device_id] = new ParamsDataDetinationSetting();
						p->p_alarm_map[device_id] = new ParamsAlarmDetinationSetting();
						p->p_device_map[device_id] = new ParamsDeviceSetting();
					}
					p->mtx.unlock();
					std::cout<<"device_id:"<<device_id<<", cond:"<<p->cond_map[device_id]<<", mtx:"<<p->mtx_map[device_id]<<std::endl;
					struct timeval tv; // selectのタイムアウト用
					fd_set fds;
					command_packet cmd;
					command_header head;
					LOG(INFO)<<"accepted";
					// 設定パラメータの入れ物
					thread_local setting_param *params = NULL;
					thread_local int n_setting_params = 0;

					// デバッグ用にシーケンスを通すためのドライバ関数を動かす
					auto dummy_serv = std::thread( debug_f );

					// ##### 「デバイスとの通信」と「IXサーバからの指令」のループ
					bool f = true;
					while(f) {
						// ### 「デバイスとの通信」セクション
						tv.tv_sec = 0; // 待機時間のオブジェクトを初期化
						tv.tv_usec = 100;
						memcpy(&fds, &readfds, sizeof(fd_set));
						int ret = select(max_fd+1, &fds, NULL, NULL, &tv);
						//std::cout << "########################### select ret = " << ret << std::endl;
						// 受信可能ならrecvする
						//if (FD_ISSET(p->sock_map[device_id], &fds)) {
						if (ret > 0 && FD_ISSET(p->sock_map[device_id], &fds)) {
							head = p->device_cmd_recv(p->sock_map[device_id]);
							if( head.command == DeviceServer::initialize_request )
								{
									/* AIツール接続ブロック */
									printf("初期化要求 : 0x%02x \n", head.command);
									LOG(INFO)<<"初期化要求 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									// 認証を返す
									set_command_packet(&cmd, 0, DeviceServer::authorization, 0x00);
									write(p->sock_map[device_id], &cmd, 12);
								}
							else if( head.command == DeviceServer::authorization )
								{
									printf("認証受信 : 0x%02x \n", head.command);
									LOG(INFO) << "認証受信 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Stopped;
								}
							else if( head.command == DeviceServer::req_setting_param )
								{
									// パラメータの受信
									//int n_param = (int)(head.data_size/8);
									n_setting_params = (int)(head.data_size/8);
									params = new setting_param[n_setting_params];
									p->recv_setting_param(device_id, params, p->sock_map[device_id], head);
									// 分類毎にパラメータを格納
									for( int i=0 ; i<n_setting_params ; i++ ) {
										if(params[i].tag[0]==0x01 && params[i].tag[1]==0x01 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_sensor_map[device_id]->sample_cycle, params[i].val, 4);
										else if(params[i].tag[0]==0x01 && params[i].tag[1]==0x01 && params[i].tag[2]==0x02 && params[i].tag[3]==0x00)
											memcpy(&p->p_sensor_map[device_id]->input_channel, params[i].val, 4);
										else if(params[i].tag[0]==0x01 && params[i].tag[1]==0x01 && params[i].tag[2]==0x03 && params[i].tag[3]==0x00)
											memcpy(&p->p_sensor_map[device_id]->input_mode, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_trigger_map[device_id]->trigger_channel, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x02 && params[i].tag[3]==0x00)
											memcpy(&p->p_trigger_map[device_id]->start_trigger, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x03 && params[i].tag[3]==0x01)
											memcpy(&p->p_trigger_map[device_id]->start_trigger_condition1, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x03 && params[i].tag[3]==0x02)
											memcpy(&p->p_trigger_map[device_id]->start_trigger_condition2, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x04 && params[i].tag[3]==0x00)
											memcpy(&p->p_trigger_map[device_id]->finish_trigger, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x05 && params[i].tag[3]==0x01)
											memcpy(&p->p_trigger_map[device_id]->finish_trigger_condition1, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x05 && params[i].tag[3]==0x02)
											memcpy(&p->p_trigger_map[device_id]->finish_trigger_condition2, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x01 && params[i].tag[2]==0x05 && params[i].tag[3]==0x04)
											memcpy(&p->p_trigger_map[device_id]->finish_trigger_condition3, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x02 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_trigger_map[device_id]->capture_channel, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x03 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_trigger_map[device_id]->filter_condition, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x03 && params[i].tag[2]==0x02 && params[i].tag[3]==0x01)
											memcpy(&p->p_trigger_map[device_id]->filter_cond_min, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x03 && params[i].tag[2]==0x02 && params[i].tag[3]==0x02)
											memcpy(&p->p_trigger_map[device_id]->filter_cond_max, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x03 && params[i].tag[2]==0x02 && params[i].tag[3]==0x04)
											memcpy(&p->p_trigger_map[device_id]->filter_cond_ave, params[i].val, 4);
										else if(params[i].tag[0]==0x02 && params[i].tag[1]==0x04 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_trigger_map[device_id]->ai_process_cycle, params[i].val, 4);
										else if(params[i].tag[0]==0x03 && params[i].tag[1]==0x01 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_ai_map[device_id]->ai_method, params[i].val, 4);
										else if(params[i].tag[0]==0x04 && params[i].tag[1]==0x01 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_data_map[device_id]->ip, params[i].val, 4);
										else if(params[i].tag[0]==0x04 && params[i].tag[1]==0x01 && params[i].tag[2]==0x02 && params[i].tag[3]==0x00)
											memcpy(&p->p_data_map[device_id]->port, params[i].val, 4);
										else if(params[i].tag[0]==0x04 && params[i].tag[1]==0x01 && params[i].tag[2]==0x03 && params[i].tag[3]==0x00)
											memcpy(&p->p_data_map[device_id]->mode, params[i].val, 4);
										else if(params[i].tag[0]==0x05 && params[i].tag[1]==0x01 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_alarm_map[device_id]->ip, params[i].val, 4);
										else if(params[i].tag[0]==0x05 && params[i].tag[1]==0x01 && params[i].tag[2]==0x02 && params[i].tag[3]==0x00)
											memcpy(&p->p_alarm_map[device_id]->port, params[i].val, 4);
										else if(params[i].tag[0]==0x05 && params[i].tag[1]==0x01 && params[i].tag[2]==0x03 && params[i].tag[3]==0x00)
											memcpy(&p->p_alarm_map[device_id]->mode, params[i].val, 4);
										else if(params[i].tag[0]==0x06 && params[i].tag[1]==0x01 && params[i].tag[2]==0x01 && params[i].tag[3]==0x00)
											memcpy(&p->p_device_map[device_id]->ip_device, params[i].val, 4);
										else if(params[i].tag[0]==0x06 && params[i].tag[1]==0x01 && params[i].tag[2]==0x02 && params[i].tag[3]==0x00)
											memcpy(&p->p_device_map[device_id]->subnet, params[i].val, 4);
										else if(params[i].tag[0]==0x06 && params[i].tag[1]==0x01 && params[i].tag[2]==0x03 && params[i].tag[3]==0x00)
											memcpy(&p->p_device_map[device_id]->def_gateway, params[i].val, 4);
										else if(params[i].tag[0]==0x06 && params[i].tag[1]==0x01 && params[i].tag[2]==0x04 && params[i].tag[3]==0x00)
											memcpy(&p->p_device_map[device_id]->ip_server, params[i].val, 4);
										else if(params[i].tag[0]==0x06 && params[i].tag[1]==0x01 && params[i].tag[2]==0x05 && params[i].tag[3]==0x00)
											memcpy(&p->p_device_map[device_id]->port, params[i].val, 4);
										else
											LOG(INFO) << "tag_error : setting_parameter";
									}
									// statusの変更
									p->device_status[device_id] = DeviceServer::Stopped;
									printf("設定パラメータ受信 : 0x%02x \n", head.command);
									LOG(INFO) << "設定パラメータ受信 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command
											  << ", データサイズ:"<<head.data_size<<"パラメータ数"<<(int)(head.data_size/8);

									//printf("データサイズ:%d, パラメータ数:%d,\n", head.data_size, (int)(head.data_size/8));
									//for (int i=0 ; i<(int)(head.data_size/8) ; i++){
									//	printf(" tag : ");
									//	for(int j=0 ; j<4 ; j++)
									//		printf("0x%02x ", params[i].tag[j] & 0xff);
									//	printf(", val : ");
									//	for(int j=0 ; j<4 ; j++)
									//		printf("0x%02x ", params[i].val[j] & 0xff);
									//	printf("\n");
									//	fflush(stdout);
									//}
								}
							else if( head.command == DeviceServer::send_setting_param )
								{
									printf("設定パラメータ転送確認受信 : 0x%02x \n", head.command);
									LOG(INFO) << "設定パラメータ転送確認受信 : " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Stopped;
								}
							else if( head.command == DeviceServer::send_setting_alarm )
								{
									printf("アラーム設定パラメータ転送確認受信 : 0x%02x \n", head.command);
									LOG(INFO) << "アラーム設定パラメータ転送確認受信 : " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Stopped;
								}
							else if( head.command == DeviceServer::send_alarm )
								{
									//p->device_status[device_id] = DeviceServer::Stopped;
									printf("アラーム受信 : 0x%02x \n", head.command);
									LOG(INFO) << "アラーム受信 : " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
								}
							else if( head.command == DeviceServer::send_ai_param )
								{
									printf("AI設定値転送確認受信 : 0x%02x \n", head.command);
									LOG(INFO) << "AI設定値転送確認受信 : " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Stopped;
								}
							else if( head.command == DeviceServer::debug_mode_on )
								{
									p->device_status[device_id] = DeviceServer::Stopped;
									//printf("デバッグモード設定完了通知受信 : 0x%02x \n", head.command);
									LOG(INFO) << "デバッグモード設定完了通知受信 : " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									// スレッド立ててTCPで接続しに行く
									p->th_deb = new std::thread([](DeviceServer *p, int device_id) {
											std::cout << "debug_task_thread" << std::endl;
											int sock_d;
											struct sockaddr_in addr_d;
											int tcp_port = p->debug_mode_tcp_port;
											std::string ip_addr = p->debug_mode_ip;
											// ##### TCPソケットの作成
											sock_d = socket(AF_INET, SOCK_STREAM, 0);
											addr_d.sin_family = AF_INET;
											addr_d.sin_port = htons(tcp_port);
											addr_d.sin_addr.s_addr = inet_addr(ip_addr.c_str());
											// 接続
											int ret = 0;
											for (int i=0 ; i<5 ; i++) {
												ret = connect(sock_d, (struct sockaddr *)&addr_d, sizeof(addr_d));
												if(ret == 0) {
													std::cout<<std::dec;
													//std::cout << "connect -> "<<ip_addr<<":"<<tcp_port << std::endl;
													LOG(INFO) << "connect_debug_task -> "<<ip_addr<<":"<<tcp_port;
													p->debug_mode_flag[device_id] = true;
													break;
												} else {
													//std::cout << "connect_error -> "<<ip_addr<<":"<<tcp_port << std::endl;
													LOG(ERROR) << "can't connect debug_task : " << ip_addr << ":" << tcp_port;
													std::chrono::milliseconds dura( 1000 );
													std::this_thread::sleep_for( dura );
													//if( i<5-1 ) { continue; }
													//else {
													//	p->debug_mode_flag[device_id] = false;
													//	LOG(ERROR) << "can't connect debug_task";
													//	return;
													//}
												}
											}
											p->flag_d = false;
											while( true ) {
												std::unique_lock<std::mutex> lock( p->mtx_d );
												while( p->s_data_q.empty() ) {
													p->cond_d.wait(lock);
												}
												if( p->flag_d ) { break; }
												//unsigned char *data_str = p->s_data_q.front();
												//p->s_data_q.pop();
												sensor_data *s_data = p->s_data_q.front();
												std::cout<< "data_value(debug_th) : " << s_data->val[0] << std::endl;
												std::cout<< "s_data memory : " <<s_data<<std::endl;
												std::cout<< "s_data_q memory : " <<&p->s_data_q<<std::endl;
												std::cout<< "q_size : " <<p->s_data_q.size()<<std::endl;
												p->s_data_q.pop();
												int size = 4+1+4*s_data->n_ch;
												unsigned char data_str[size];
												int pos = 0;
												memcpy(&data_str[pos], &s_data->time, 4); pos = pos + 4;
												memcpy(&data_str[pos], &s_data->input_ch, 1); pos = pos + 1;
												for(int i=0 ; i<s_data->n_ch ; i++) {
													memcpy(&data_str[pos], &s_data->val[i], 4);
													pos = pos + 4;
												}
												command_packet cmd;
												set_command_packet(&cmd, size, DeviceServer::send_sensor_data, 0x00);
												unsigned char pac[12+size];
												memcpy(&pac[0], cmd.header1, 4);
												memcpy(&pac[4], cmd.header2, 8);
												memcpy(&pac[12], data_str, size);
												
												printf("デバッグ用データ : ");
												for(int i=0 ; i<sizeof(pac) ; i++)
													printf("0x%02x ", pac[i]);
												printf("\n");
												
												write(sock_d, pac, 12+size);
												// *** 送ったら受信確認を受け取る
												command_header head = p->device_cmd_recv(sock_d);
												//printf("command : 0x%02x, size : %d\n", head.command, head.data_size);
												LOG(INFO) << "recv : デバッグ用データの受信確認 : 0x"
														  <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
												p->cond_d.notify_all();
												delete s_data;
											}
											close(sock_d);
											std::cout << "finish debug_task_thread" << std::endl;
										}, p, device_id);
								}
							else if( head.command == DeviceServer::debug_mode_off )
								{
									printf("デバッグモード解除完了通知受信 : 0x%02x \n", head.command);
									LOG(INFO) << "デバッグモード解除完了通知受信 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Stopped;
								}
							else if( head.command == DeviceServer::data_mode_on )
								{
									printf("データ収集モード設定完了通知受信 : 0x%02x \n", head.command);
									LOG(INFO) << "データ収集モード設定完了通知受信 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Collecting;
								}
							else if( head.command == DeviceServer::data_mode_off )
								{
									p->device_status[device_id] = DeviceServer::Stopped;
									printf("データ収集モード解除完了通知受信 : 0x%02x \n", head.command);
									LOG(INFO) << "データ収集モード解除完了通知受信 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
								}
							else if( head.command == DeviceServer::alarm_mode_on )
								{
									printf("監視モード設定完了通知受信 : 0x%02x \n", head.command);
									LOG(INFO) << "監視モード設定完了通知受信 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Monitoring;
								}
							else if( head.command == DeviceServer::alarm_mode_off )
								{
									printf("監視モード解除完了通知受信 : 0x%02x \n", head.command);
									LOG(INFO) << "監視モード解除完了通知受信 : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Stopped;
								}
							else if( head.command == DeviceServer::disconnect )
								{
									printf("デバイス切断受信 : 0x%02x \n", head.command);
									LOG(INFO) << "デバイス切断信号受信 : " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									p->device_status[device_id] = DeviceServer::Disconnected;
									return;
								}
							else if( head.command == DeviceServer::err_packet )
								{
									LOG(INFO) << "パケットエラー : "<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
								}
							else if( head.command == DeviceServer::err_param )
								{
									LOG(INFO) << "設定パラメータエラー : " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									//int n = (head.data_size);
									//if ( n>0 ) {
									//	unsigned char tmp[n];
									//	p->recv_body(tmp, p->sock_map[device_id], head);
									//	for(int i=0 ; i<n ; i++) printf("%02x", tmp[i]);
									//}
									p->device_status[device_id] = DeviceServer::Stopped;
								}
							else
								{
									LOG(INFO) << "unknown コマンド受信: " <<std::hex<<std::setw(2)<<std::setfill('0')<<(int)head.command;
									//int n = (head.data_size);
									//if ( n>0 ) {
									//	unsigned char tmp[n];
									//	p->recv_body(tmp, p->sock_map[device_id], head);
									//	for(int i=0 ; i<n ; i++) printf("%02x", tmp[i]);
									//}
									p->device_status[device_id] = DeviceServer::Stopped;
								}
						}
						// ### 「IXサーバからの指令」セクション
						std::unique_lock<std::mutex> lock( *(p->mtx_map[device_id]) );
						if (p->cond_map[device_id]->wait_for(lock, std::chrono::seconds(1), [&] { return p->ready_map[device_id]; }) ) {
							//std::cout<<"デバイスコマンド(deviceThread) : device_id="<<device_id<<", input_cmd="<<p->cmd_map[device_id]<<std::endl;
							int l = 0;
							if ( p->cmd_map[device_id]=="04" )
								{
									// デバイス切断
									std::cout << "デバイス切断" << std::endl;
									LOG(INFO) << "デバイス切断";
									set_command_packet(&cmd, 0, DeviceServer::disconnect, 0x00);
									write(p->sock_map[device_id], &cmd, 12);
									p->device_status[device_id] = DeviceServer::Disconnected;
									f = false;
								}
							else if ( p->cmd_map[device_id]=="20" )
								{
									// 設定パラメータ要求
									//std::cout << "設定パラメータ要求" << std::endl;
									LOG(INFO) << "設定パラメータ要求";
									p->device_status[device_id] = DeviceServer::Busy;
									set_command_packet(&cmd, 0, DeviceServer::req_setting_param, 0x00);
									//write(p->sock_map[device_id], &cmd, 12);
									l = send(p->sock_map[device_id], &cmd, 12, 0);
									std::cout << "送信結果 : " << l << std::endl;
								}
							else if ( p->cmd_map[device_id]=="21" )
								{
									p->device_status[device_id] = DeviceServer::Busy;
									// 設定パラメータ転送
									//std::cout << "設定パラメータ転送" << std::endl;
									LOG(INFO) << "設定パラメータ転送";
									// 設定パラメータ転送用のbody部を作る(受信してきた値をそのまま入れる)
									//p->make_setting_parameter(device_id, n_setting_params, params);

									// test
									printf("setting_parameter[%d] : ", p->param_size[device_id]);
									for (int i=0 ; i<p->param_size[device_id] ; i++) {
										if( i%8==0 ) printf("\n");
										printf("0x%02x ", p->buf_map[device_id][i]);
									} printf("\n");
									// test

									set_command_packet(&cmd, p->param_size[device_id], DeviceServer::send_setting_param, 0x00);
									unsigned char pac[12+p->param_size[device_id]];
									memcpy(&pac[0], cmd.header1, 4);
									memcpy(&pac[4], cmd.header2, 8);
									memcpy(&pac[12], p->buf_map[device_id], p->param_size[device_id]);
									//write(p->sock_map[device_id], pac, 12+p->param_size[device_id]);
									l = send(p->sock_map[device_id], pac, 12+p->param_size[device_id], 0);
									//std::cout << "送信結果 : " << l << std::endl;
								}
							else if ( p->cmd_map[device_id]=="86" )
								{
									// 設定パラメータ転送
									p->device_status[device_id] = DeviceServer::Busy;
									//std::cout << "アラーム設定パラメータ転送" << std::endl;
									LOG(INFO) << "アラーム設定パラメータ転送";
									//
									set_command_packet(&cmd, p->param_size[device_id], DeviceServer::send_setting_alarm, 0x00);
									//std::cout << "サイズ : " << p->params_buf_size << std::endl;
									unsigned char pac[12+p->param_size[device_id]];
									memcpy(&pac[0], cmd.header1, 4);
									memcpy(&pac[4], cmd.header2, 8);
									memcpy(&pac[12], p->body_map[device_id], p->param_size[device_id]);
									write(p->sock_map[device_id], pac, 12+p->param_size[device_id]);
									//for(int i=0 ; i<sizeof(pac) ; i++) printf("%02x ", pac[i]);
								}
							else if ( p->cmd_map[device_id]=="30" )
								{
									p->device_status[device_id] = DeviceServer::Busy;
									// ***** NN数 : ここでハードコードしてるのはいただけない!!
									int n_network = 1;
									// ***** 閾値 : JSONで読み取る必要がある!!
									float threashold = 0.2;
									// AIパラメータ転送
									std::cout << "AIパラメータ転送" << std::endl;
									LOG(INFO) << "AIパラメータ転送";
									command_packet tmp;
									// data_size = NNクラスで持ってるbyte_size + 1byte(NN数) + 4byte(閾値)
									int data_size = p->NN.byte_size + 1 + 4;
									set_command_packet(&tmp, data_size, DeviceServer::send_ai_param, 0x00);
									// ネットワーク設定をバッファに格納
									unsigned char *w_buf = new unsigned char[4+8+data_size];
									memcpy(&w_buf[0], tmp.header1, 4);
									memcpy(&w_buf[4], tmp.header2, 8);
									memcpy(&w_buf[4+8], &threashold, 4); // 閾値
									memcpy(&w_buf[4+8+4], &n_network, 1); // NN数を入れる
									memcpy(&w_buf[4+8+4+1], p->NN.params_byte, p->NN.byte_size); // NNクラスでまとめたデータ
									//// printf debug
									//printf("NN parameter[%d] : \n", p->NN.byte_size);
									//for(int i=0 ; i<12+data_size ; i++) printf("0x%02x ", w_buf[i]);
									//printf("\n");
									//// printf debug
									write(p->sock_map[device_id], w_buf, 12+data_size);
									delete[] w_buf;
								}
							else if ( p->cmd_map[device_id]=="80" )
								{
									// デバッグモード起動
									p->device_status[device_id] = DeviceServer::Busy;
									std::cout << "デバッグモード起動" << std::endl;
									LOG(INFO) << "デバッグモード起動指示";
									set_command_packet(&cmd, 0, DeviceServer::debug_mode_on, 0x00);
									write(p->sock_map[device_id], &cmd, 12);
								}
							else if ( p->cmd_map[device_id]=="81" )
								{
									// デバッグモードOFF
									p->device_status[device_id] = DeviceServer::Busy;
									std::cout << "デバッグモードOFF" << std::endl;
									LOG(INFO) << "デバッグモード解除指示";
									set_command_packet(&cmd, 0, DeviceServer::debug_mode_off, 0x00);
									write(p->sock_map[device_id], &cmd, 12);

									// デバイス制御スレッドにコマンドを通知する
									p->ready_d = true;
									p->flag_d = true;
									sensor_data tmp;
									p->s_data_q.push(&tmp);
									// 待機状態のスレッドを起こす
									p->cond_d.notify_one();
									//
									p->th_deb->join();
									delete p->th_deb;
								}
							else if ( p->cmd_map[device_id]=="82" )
								{
									// データ収集モードON
									p->device_status[device_id] = DeviceServer::Busy;
									std::cout << "データ収集モードON" << std::endl;
									LOG(INFO) << "データ収集モード設定指示";
									set_command_packet(&cmd, 0, DeviceServer::data_mode_on, 0x00);
									write(p->sock_map[device_id], &cmd, 12);
								}
							else if ( p->cmd_map[device_id]=="83" )
								{
									// データ収集モードOFF
									p->device_status[device_id] = DeviceServer::Busy;
									std::cout << "データ収集モードOFF" << std::endl;
									LOG(INFO) << "データ収集モード解除指示";
									set_command_packet(&cmd, 0, DeviceServer::data_mode_off, 0x00);
									write(p->sock_map[device_id], &cmd, 12);
								}
							else if ( p->cmd_map[device_id]=="84" )
								{
									// 監視モードON
									p->device_status[device_id] = DeviceServer::Busy;
									std::cout << "監視モードON" << std::endl;
									LOG(INFO) << "監視モード設定指示";
									set_command_packet(&cmd, 0, DeviceServer::alarm_mode_on, 0x00);
									write(p->sock_map[device_id], &cmd, 12);
								}
							else if ( p->cmd_map[device_id]=="85" )
								{
									// 監視モードOFF
									p->device_status[device_id] = DeviceServer::Busy;
									std::cout << "監視モードOFF" << std::endl;
									LOG(INFO) << "監視モード解除指示";
									set_command_packet(&cmd, 0, DeviceServer::alarm_mode_off, 0x00);
									write(p->sock_map[device_id], &cmd, 12);
								}
							else
								{
									// 登録されていないコマンド
									LOG(INFO) << "unknown コマンドが入力された";
									//// debug用
									//set_command_packet(&cmd, 0, DeviceServer::err_packet, 0x00);
									//write(p->sock_map[device_id], &cmd, 12);
								}
							p->ready_map[device_id] = false;
							p->cond_map[device_id]->notify_one();
						}
						google::FlushLogFiles(google::INFO);
					}
					if( params!=NULL ) { delete[] params; } // 設定パラメータの入れ物破棄する
					if( p->buf_map[device_id]!=NULL ) { delete p->buf_map[device_id]; }
					if( p->body_map[device_id]!=NULL ) { delete p->body_map[device_id]; }
					if( p->p_sensor_map[device_id]!=NULL ) { delete p->p_sensor_map[device_id]; }
					if( p->p_trigger_map[device_id]!=NULL ) { delete p->p_trigger_map[device_id]; }
					if( p->p_ai_map[device_id]!=NULL ) { delete p->p_ai_map[device_id]; }
					if( p->p_data_map[device_id]!=NULL ) { delete p->p_data_map[device_id]; }
					if( p->p_alarm_map[device_id]!=NULL ) { delete p->p_alarm_map[device_id]; }
					if( p->p_device_map[device_id]!=NULL ) { delete p->p_device_map[device_id]; }
					FD_CLR(p->sock_map[device_id], &readfds); // 監視対象のfdリストから該当のfdを抜く
					dummy_serv.join(); // デバッグ用のダミーサーバの停止を待つ
					//std::cout<<"デバイス処理スレッド抜けます@dev_thread  "<<p->sock_map[device_id]<<std::endl;
					LOG(INFO) << "デバイス処理スレッド抜けます@dev_thread";
					close(p->sock_map[device_id]);
				}, this) );
	} while(true);
	// threadListのスレッドの停止を待つ
	for (std::thread &th : threadList) th.join();
	updServer.join();
	updServer_a.join();
	close(sock0);
}


void DeviceServer::set_sensor_data(sensor_data data)
{
	std::unique_lock<std::mutex> lock( mtx_d );
	//// データ設定作業
	////if( s_data!=NULL ) delete[] s_data;
	//int size = 4+1+4*data.n_ch;
	//s_data_len = size;
	////s_data = new unsigned char[s_data_len];
	//unsigned char data_str[size];
	//int pos = 0;
	////memcpy(&s_data[pos], &data.time, 4); pos = pos + 4;
	////memcpy(&s_data[pos], &data.input_ch, 1); pos = pos + 1;
	////for(int i=0 ; i<data.n_ch ; i++) {
	////	memcpy(&s_data[pos], &data.val[i], 4);
	////	pos = pos + 4;
	////}
	//memcpy(&data_str[pos], &data.time, 4); pos = pos + 4;
	//memcpy(&data_str[pos], &data.input_ch, 1); pos = pos + 1;
	//for(int i=0 ; i<data.n_ch ; i++) {
	//	memcpy(&data_str[pos], &data.val[i], 4);
	//	pos = pos + 4;
	//}

	sensor_data *tmp_data = new sensor_data();
	memcpy(tmp_data, &data, sizeof(data));
	std::cout<< "data_value(base) : " << tmp_data->val[0] << std::endl;
	std::cout<< "s_data memory : " <<tmp_data<<std::endl;
	std::cout<< "s_data_q memory : " <<&s_data_q<<std::endl;
	std::cout<< "q_size : " <<s_data_q.size()<<std::endl;
	s_data_q.push(tmp_data);
	// 待機状態のスレッドを起こす
	cond_d.notify_one();
	//
	//printf("s_data_l : %d\n", s_data_len);
	//printf("data.n_ch : %d\n", data.n_ch);
	//printf("data.input_ch : 0x%02x\n", data.input_ch);
	//printf("data.val : ");
	//for(int i=0 ; i<data.n_ch ; i++)
	//	printf("%f, ", data.val[i]);
	//printf("\n");
	//printf("data_byte : ");
	//for(int i=0 ; i<(4+1+4*data.n_ch) ; i++)
	//	printf("0x%02x ", s_data[i]);
	//printf("\n");
	//
}

void DeviceServer::set_debugmode_setting(const char* ip_addr, int tcp_port)
{
	debug_mode_ip = ip_addr;
	//std::cout << ip_addr << "," << sizeof(ip_addr) << ", " << debug_mode_ip << std::endl;
	debug_mode_tcp_port = tcp_port;
}


int DeviceServer::set_AI_parameter(std::string str)
{
	if(int ret = NN.setJsonString(str) < 0 ) {
		std::cout << "JSON文字列のparseに失敗 : " << ret << std::endl;
		return 0;
	}
	// パラメータの確認
	//NN.dispNNParameterSUM();
	// バイト文字列に変換
	NN.convert();
	return 1;
}

void DeviceServer::make_setting_parameter(int d_id, int n_p, setting_param *sp) {
	unsigned char msg[(4+4)*n_p]; // タグで4byte,valueで4byte, パラメータ数が3つまで実装
	param_size[d_id] = (4+4)*n_p;

	buf_map[d_id] = new unsigned char[(4+4)*n_p];
	for(int i=0 ; i<n_p ; i++) {
		memcpy(&buf_map[d_id][i*(4+4)], sp[i].tag, 4);
		memcpy(&buf_map[d_id][i*(4+4)+4], sp[i].val, 4);
	}

	//printf("setting_param : ");
	//for (int i=0 ; i<(4+4)*n_p ; i++) {
	//	if(i%8==0) printf("\n");
	//	printf("0x%02x, ", buf_map[d_id][i]);
	//}
	//printf("\n");
}
void DeviceServer::init_params_buffer(int dev_id) {
	param_size[dev_id] = 0;
}
void DeviceServer::set_params(int d_id, ParamsSensorSetting p1) {
	int pos = 0;
	unsigned char msg[ param_size[d_id] + (4+4)*14 ]; // タグで4byte,valueで4byte, パラメータ数が3つまで実装
	if( param_size[d_id] > 0 ) {
		//printf("パラメータ追加 %d\n", param_size[d_id]);
		memcpy(&msg[0], buf_map[d_id], param_size[d_id]);
		pos = param_size[d_id];
	}
	msg[pos+0]=0x01; msg[pos+1]=0x01; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p1.sample_cycle, 4); pos=pos+8;
	msg[pos+0]=0x01; msg[pos+1]=0x01; msg[pos+2]=0x02; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p1.input_channel, 4); pos=pos+8;
	msg[pos+0]=0x01; msg[pos+1]=0x01; msg[pos+2]=0x03; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p1.input_mode, 4); pos=pos+8;

	param_size[d_id] = pos;
	buf_map[d_id] = new unsigned char[sizeof(msg)];
	memcpy(buf_map[d_id], msg, sizeof(msg));

	//// test
	//printf("setting_parameter[%d] : ", param_size[d_id]);
	//for (int i=0 ; i<param_size[d_id] ; i++) {
	//	if(i%8==0) printf("\n");
	//	printf("%02x ", buf_map[d_id][i]);
	//} printf("\n");
	//// test
}
void DeviceServer::set_params(int d_id, ParamsTriggerSetting p2) {
	int pos = 0;
	unsigned char msg[ param_size[d_id] + (4+4)*14 ]; // タグで4byte,valueで4byte, パラメータ数が3つまで実装
	if( param_size[d_id] > 0 ) {
		//printf("パラメータ追加 %d\n", param_size[d_id]);
		memcpy(&msg[0], buf_map[d_id], param_size[d_id]);
		pos = param_size[d_id];
	}
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p2.trigger_channel, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x02; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p2.start_trigger, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x03; msg[pos+3]=0x01; memcpy(&msg[pos+4], &p2.start_trigger_condition1, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x03; msg[pos+3]=0x02; memcpy(&msg[pos+4], &p2.start_trigger_condition2, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x04; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p2.finish_trigger, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x05; msg[pos+3]=0x01; memcpy(&msg[pos+4], &p2.finish_trigger_condition1, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x05; msg[pos+3]=0x02; memcpy(&msg[pos+4], &p2.finish_trigger_condition2, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x01; msg[pos+2]=0x05; msg[pos+3]=0x04; memcpy(&msg[pos+4], &p2.finish_trigger_condition3, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x02; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p2.capture_channel, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x03; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p2.filter_condition, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x03; msg[pos+2]=0x02; msg[pos+3]=0x01; memcpy(&msg[pos+4], &p2.filter_cond_min, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x03; msg[pos+2]=0x02; msg[pos+3]=0x02; memcpy(&msg[pos+4], &p2.filter_cond_max, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x03; msg[pos+2]=0x02; msg[pos+3]=0x04; memcpy(&msg[pos+4], &p2.filter_cond_ave, 4); pos=pos+8;
	msg[pos+0]=0x02; msg[pos+1]=0x04; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p2.ai_process_cycle, 4); pos=pos+8;
	param_size[d_id] = pos;
	buf_map[d_id] = new unsigned char[sizeof(msg)];
	memcpy(buf_map[d_id], msg, sizeof(msg));

	//// test
	//printf("setting_parameter[%d] : ", param_size[d_id]);
	//for (int i=0 ; i<param_size[d_id] ; i++) {
	//	if(i%8==0) printf("\n");
	//	printf("%02x ", buf_map[d_id][i]);
	//} printf("\n");
	//// test
}
void DeviceServer::set_params(int d_id, ParamsAISetting p3) {
	int pos = 0;
	unsigned char msg[ param_size[d_id] + (4+4)*14 ]; // タグで4byte,valueで4byte, パラメータ数が3つまで実装
	if( param_size[d_id] > 0 ) {
		//printf("パラメータ追加 %d\n", param_size[d_id]);
		memcpy(&msg[0], buf_map[d_id], param_size[d_id]);
		pos = param_size[d_id];
	}
	msg[pos+0]=0x03; msg[pos+1]=0x01; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p3.ai_method, 4); pos=pos+8;
	param_size[d_id] = pos;
	buf_map[d_id] = new unsigned char[sizeof(msg)];
	memcpy(buf_map[d_id], msg, sizeof(msg));
}
void DeviceServer::set_params(int d_id, ParamsDataDetinationSetting p4) {
	int pos = 0;
	unsigned char msg[ param_size[d_id] + (4+4)*14 ]; // タグで4byte,valueで4byte, パラメータ数が3つまで実装
	if( param_size[d_id] > 0 ) {
		//printf("パラメータ追加 %d\n", param_size[d_id]);
		memcpy(&msg[0], buf_map[d_id], param_size[d_id]);
		pos = param_size[d_id];
	}
	msg[pos+0]=0x04; msg[pos+1]=0x01; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p4.ip, 4); pos=pos+8;
	msg[pos+0]=0x04; msg[pos+1]=0x01; msg[pos+2]=0x02; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p4.port, 4); pos=pos+8;
	msg[pos+0]=0x04; msg[pos+1]=0x01; msg[pos+2]=0x03; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p4.mode, 4); pos=pos+8;
	param_size[d_id] = pos;
	buf_map[d_id] = new unsigned char[sizeof(msg)];
	memcpy(buf_map[d_id], msg, sizeof(msg));
}
void DeviceServer::set_params(int d_id, ParamsAlarmDetinationSetting p5) {
	int pos = 0;
	unsigned char msg[ param_size[d_id] + (4+4)*14 ]; // タグで4byte,valueで4byte, パラメータ数が3つまで実装
	if( param_size[d_id] > 0 ) {
		//printf("パラメータ追加 %d\n", param_size[d_id]);
		memcpy(&msg[0], buf_map[d_id], param_size[d_id]);
		pos = param_size[d_id];
	}
	msg[pos+0]=0x05; msg[pos+1]=0x01; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p5.ip, 4); pos=pos+8;
	msg[pos+0]=0x05; msg[pos+1]=0x01; msg[pos+2]=0x02; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p5.port, 4); pos=pos+8;
	msg[pos+0]=0x05; msg[pos+1]=0x01; msg[pos+2]=0x03; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p5.mode, 4); pos=pos+8;
	param_size[d_id] = pos;
	buf_map[d_id] = new unsigned char[sizeof(msg)];
	memcpy(buf_map[d_id], msg, sizeof(msg));
}
void DeviceServer::set_params(int d_id, ParamsDeviceSetting p6) {
	int pos = 0;
	unsigned char msg[ param_size[d_id] + (4+4)*14 ]; // タグで4byte,valueで4byte, パラメータ数が3つまで実装
	if( param_size[d_id] > 0 ) {
		//printf("パラメータ追加 %d\n", param_size[d_id]);
		memcpy(&msg[0], buf_map[d_id], param_size[d_id]);
		pos = param_size[d_id];
	}
	msg[pos+0]=0x06; msg[pos+1]=0x01; msg[pos+2]=0x01; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p6.ip_device, 4); pos=pos+8;
	msg[pos+0]=0x06; msg[pos+1]=0x01; msg[pos+2]=0x02; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p6.subnet, 4); pos=pos+8;
	msg[pos+0]=0x06; msg[pos+1]=0x01; msg[pos+2]=0x03; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p6.def_gateway, 4); pos=pos+8;
	msg[pos+0]=0x06; msg[pos+1]=0x01; msg[pos+2]=0x04; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p6.ip_server, 4); pos=pos+8;
	msg[pos+0]=0x06; msg[pos+1]=0x01; msg[pos+2]=0x05; msg[pos+3]=0x00; memcpy(&msg[pos+4], &p6.port, 4); pos=pos+8;
	param_size[d_id] = pos;
	buf_map[d_id] = new unsigned char[sizeof(msg)];
	memcpy(buf_map[d_id], msg, sizeof(msg));
}
unsigned char * DeviceServer::set_params(int d_id, std::string param_json) {
	picojson::value v;
	std::string err = picojson::parse(v, param_json);
	if(!err.empty()) {
		std::cerr << err << std::endl;
		return NULL;
	}
	std::map<std::string, picojson::value> &data = v.get<picojson::object>();
	for (auto& x:data) {
		std::cout << x.first << " : " << x.second << std::endl;
	}

	return NULL;
}


int DeviceServer::recv_body(unsigned char *buf, int sock, command_header header){
	// 単純にパケットのbody部を読み込んで返す
	int n = recv(sock, buf, header.data_size, 0);
	if( n==header.data_size ) return 1;
	else return 0;
}

void DeviceServer::recv_setting_param(int dev_id, setting_param *params, int sock, command_header header)
{
	unsigned char buf[1024]; // 予め固定幅を確保するのをやめたい
	int n_param = (int)(header.data_size/8);
	int n = recv(sock, buf, header.data_size, 0);

	// テスト用に受信データをバッファに保存
	ndata_map[dev_id] = header.data_size;
	buf_map[dev_id] = new unsigned char[ndata_map[dev_id]];
	memcpy(buf_map[dev_id], buf, ndata_map[dev_id]);

	//printf("recv_data[%d] : ", n);
	//for (int i=0 ; i<n ; i++) printf("0x%02x ", buf[i]);
	//printf("\n");

	if(n==header.data_size){
		for (int i=0 ; i<n_param ; i++) {
			memcpy(&params[i].tag, &buf[i*8+0], 4);
			memcpy(&params[i].val, &buf[i*8+4], 4);
		}
	} else
		std::cout << "データ長エラー req:" << header.data_size << ", actually:" << n << std::endl;
}

command_header DeviceServer::device_cmd_recv(int sock)
{
	command_header cmd;
	unsigned char buf[12];
	// ヘッダ受信
	int n = recv(sock, buf, 12, 0);
	if(n == 0) {
		// デバイスとの接続切れた
		cmd.data_size = 0;
		//cmd.command = 0x00;
		cmd.command = DeviceServer::disconnect;
		cmd.io = 0x00;
	} else {
		// ヘッダ情報の格納
		// header1の確認
		if(buf[0]==0x52 && buf[1]==0x45 && buf[2]==0x41 && buf[3]==0x49)
			{
				// バッファのサイズを完全に指定しているのを何かしたい
				memcpy(&cmd.data_size, &buf[4], 4);
				memcpy(&cmd.command, &buf[4+4], 1);
				memcpy(&cmd.io, &buf[4+4+1], 1);
				//
				LOG(INFO)<<"received packet : data_size="<<(int)cmd.data_size <<", command="<<std::hex<<std::setw(2)<<std::setfill('0')<<(int)cmd.command;
			}
		else
			{
				cmd.data_size = 0;
				cmd.command = DeviceServer::err_packet;
				LOG(INFO)<<"ERROR packet : invarid header";
				cmd.io = 0x00;
			}
	}
	return cmd;
}

std::string DeviceServer::setDeviceCommand(int d_id, std::string c)
{
	device_status[d_id] = DeviceServer::Busy;
	std::unique_lock<std::mutex> lock( *(mtx_map[d_id]) );
	// デバイス制御スレッドにコマンドを通知する
	ready_map[d_id] = true;
	// コマンド設定作業
	cmd_map[d_id] = c;
	// 待機状態のスレッドを起こす
	cond_map[d_id]->notify_one();
	return cmd_map[d_id];
}

//void DeviceServer::socket_halt(int FLAGS_tcp_port, int port_s, int port_a)
void DeviceServer::socket_halt()
{
	// UDPのスレッドを落とす(for sensor)
	struct sockaddr_in addr_udp_s;
	int sock_s;
	sock_s = socket(AF_INET, SOCK_DGRAM, 0);
	// ソケットの設定
	addr_udp_s.sin_family = AF_INET;
	addr_udp_s.sin_port = htons(PORT_SENSOR);
	addr_udp_s.sin_addr.s_addr = INADDR_ANY;
	// 切断用のソケット作成
	command_packet cmd;
	set_command_packet(&cmd, 0, DeviceServer::disconnect, 0x00);
	unsigned char halt_pac[12];
	memcpy(&halt_pac[0], cmd.header1, 4);
	memcpy(&halt_pac[4], cmd.header2, 8);
	sendto(sock_s, halt_pac, sizeof(halt_pac), 0, (struct sockaddr *)&addr_udp_s, sizeof(addr_udp_s));
	// UDPのスレッドを落とす (for alarm)
	sock_s = socket(AF_INET, SOCK_DGRAM, 0);
	// ソケットの設定
	addr_udp_s.sin_family = AF_INET;
	addr_udp_s.sin_port = htons(PORT_ALARM);
	addr_udp_s.sin_addr.s_addr = INADDR_ANY;
	sendto(sock_s, halt_pac, sizeof(halt_pac), 0, (struct sockaddr *)&addr_udp_s, sizeof(addr_udp_s));

	// acceptを抜けるために、connectだけしに行く関数
	struct sockaddr_in server;
	int sock;
	// socket切断のフラグを立てる
	socket_halt_flag = true;
	/* ソケットの作成 */
	sock = socket(AF_INET, SOCK_STREAM, 0);
	/* 接続先指定用構造体の準備 */
	server.sin_family = AF_INET;
	server.sin_port = htons(PORT_DEVICE);
	server.sin_addr.s_addr = INADDR_ANY;
	/* サーバに接続 */
	connect(sock, (struct sockaddr *)&server, sizeof(server));
	// ソケットの破棄
	close(sock);
}

unsigned char DeviceServer::getDeviceStatus(int dev_id)
{
	return device_status[dev_id];
}


DeviceServer * pDeviceServer = NULL;

bool cmd_function(int dev_id, std::string cmd, DeviceServer *p, unsigned char wait_state)
{
	std::chrono::milliseconds dura( 100 );
	int cnt = 0;
	while(cnt < 10) {
		if( p->getDeviceStatus(dev_id) == wait_state ) {
			p->setDeviceCommand(dev_id, cmd);
			return true;
		}
		std::this_thread::sleep_for( dura );
		cnt++;
	}
	return false;
}

bool check_recv(int dev_id, DeviceServer *p, unsigned char wait_state)
{
	std::chrono::milliseconds dura( 100 );
	int cnt = 0;
	while(cnt < 10) {
		if( p->getDeviceStatus(dev_id) == wait_state ) {
			// ***** ここに、受信結果に応じた処理を書く(エラーコマンドなど)
			return true;
		}
		std::this_thread::sleep_for( dura );
		cnt++;
	}
	LOG(INFO) << "time over";
	return false;
}

extern "C" {

    bool ai_device_init() {
        pDeviceServer = new DeviceServer();
        return true;
    }

    bool ai_device_exit() {
        delete pDeviceServer;
        pDeviceServer = NULL;
    }

    bool ai_device_start(int port, int port_s, int port_a) {
		/*
		  int port : TCPのポート番号
		  int port_s : UDPでセンサデータを送受信するためのポート番号
		  int port_a : UDPでアラームを送受信するためのポート番号
		 */
        bool ret = pDeviceServer->run([](){}, port, port_s, port_a );
        return ret;
    }

    bool ai_device_stop() {
		int* pDeviceIDList;
        int  pNum;
		ai_device_get_id_list(&pDeviceIDList, &pNum);
		for (int i=0 ; i<pNum ; i++) {
			//pDeviceServer->setDeviceCommand(pDeviceIDList[i], "04");
			bool ret = cmd_function(pDeviceIDList[i], "04", pDeviceServer, DeviceServer::Stopped);
			if(ret)
				LOG(INFO) << "disconnect AI device : " << pDeviceIDList[i];
			else
				LOG(INFO) << "Cannot disconnect AI device : " << pDeviceIDList[i];
		}
		pDeviceServer->socket_halt();
        pDeviceServer->stop();
		LOG(INFO) << "all AI_device stopped";
        return true;
    }

    bool ai_device_get_id_list(int** ppDeviceIDList, int * pNum) {
		/*
		  int** ppDeviceIDList : device_idの配列
		  int * pNum : deviceの数
		 */
        *pNum = pDeviceServer->device_ids.size();
		*ppDeviceIDList = pDeviceServer->device_ids.data();
        return true;
    }

	int ai_device_get_status(int deviceID) {
		return pDeviceServer->getDeviceStatus(deviceID);
    }

    bool ai_device_install_neural_network(int deviceID, const char * json) {
		pDeviceServer->set_AI_parameter(json);
		bool ret = cmd_function(deviceID, "30", pDeviceServer, DeviceServer::Stopped);
		// 受信確認してからreturnするべき
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
    }

    bool ai_device_set_debug_mode(int deviceID, bool flag) {
		if(flag) { 
			LOG(INFO) << "デバッグモードのIPとポートはハードコードしている";
			const char deb_ip[] = "192.168.1.100";
			pDeviceServer->set_debugmode_setting(deb_ip, 50004); // デバッグモード設定
			cmd_function(deviceID, "80", pDeviceServer, DeviceServer::Stopped);
			// ***** デバッグスレッドが立ったことを確認する
			//std::chrono::milliseconds dura( 500 );
			//std::this_thread::sleep_for( dura );
			int cnt=0;
			while( !pDeviceServer->debug_mode_flag[deviceID] ) {
				//std::cout << "debug_mode_flag : " << pDeviceServer->debug_mode_flag[deviceID];
				std::chrono::milliseconds dura( 500 );
				std::this_thread::sleep_for( dura );
				if(cnt > 20) {
					LOG(INFO) << "time over [debug_mode]";
					return false;
				}
				cnt++;
			}
			return true;
		} else {
			cmd_function(deviceID, "81", pDeviceServer, DeviceServer::Stopped);
			return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
		}
	}


    bool ai_device_send_debug_data(int deviceID, sensor_data * data) {
		pDeviceServer->set_sensor_data(*data);
		
        return true;
    }

    bool ai_device_set_mode(int deviceID, int mode) {
		std::chrono::milliseconds dura( 1000 );
		if( mode==1 ) {
			// Collecting状態に移行
			cmd_function(deviceID, "82", pDeviceServer, DeviceServer::Stopped); //データ収集モードon
			return check_recv(deviceID, pDeviceServer, DeviceServer::Collecting);
		} else if( mode==2 ) {
			// Monitoring状態に移行
			cmd_function(deviceID, "84", pDeviceServer, DeviceServer::Stopped); //データ収集モードon
			return check_recv(deviceID, pDeviceServer, DeviceServer::Monitoring);
		} else if( mode==0 ) {
			// Waiting状態に移行
			unsigned char state = pDeviceServer->getDeviceStatus(deviceID);
			int cnt=0;
			while (cnt<10) {
				if ( state != DeviceServer::Busy ) {
					if( state == DeviceServer::Collecting ) {
						cmd_function(deviceID, "83", pDeviceServer, DeviceServer::Collecting); // データ収集モードoff
						return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
					} else if( state == DeviceServer::Monitoring ) {
						cmd_function(deviceID, "85", pDeviceServer, DeviceServer::Monitoring); // 監視モードoff
						return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
					} else {
						LOG(INFO) << "モード切り替えの必要なし";
						return false;
					}
				}
				cnt++;
				std::this_thread::sleep_for( dura );
			}
			LOG(INFO) << "コマンドの終了待ち(time over)";
			return false;
		}
		LOG(INFO) << "不正なmodeが入力された : " << mode;
		return false;
    }

	// AIデバイス設定情報の入力
	bool ai_device_set_setting(int deviceID, ParamsSensorSetting * p1,
							   ParamsTriggerSetting * p2,
							   ParamsAISetting * p3,
							   ParamsDataDetinationSetting * p4,
							   ParamsAlarmDetinationSetting * p5, 
							   ParamsDeviceSetting * p6) {
		pDeviceServer->init_params_buffer(deviceID);
		pDeviceServer->set_params(deviceID, *p1);
		pDeviceServer->set_params(deviceID, *p2);
		pDeviceServer->set_params(deviceID, *p3);
		pDeviceServer->set_params(deviceID, *p4);
		pDeviceServer->set_params(deviceID, *p5);
		pDeviceServer->set_params(deviceID, *p6);
		bool ret = cmd_function(deviceID, "21", pDeviceServer, DeviceServer::Stopped);
		// 受信確認してからreturnするべき
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
	}
	bool ai_device_set_sensor_setting(int deviceID, ParamsSensorSetting * params) {
		pDeviceServer->init_params_buffer(deviceID);
		pDeviceServer->set_params(deviceID, *params);
		//pDeviceServer->setDeviceCommand(deviceID, "21");
		bool ret = cmd_function(deviceID, "21", pDeviceServer, DeviceServer::Stopped);
		// 受信確認してからreturnするべき
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
	}
	bool ai_device_set_trigger_setting(int deviceID, ParamsTriggerSetting * params) {
		pDeviceServer->init_params_buffer(deviceID);
		pDeviceServer->set_params(deviceID, *params);
		//pDeviceServer->setDeviceCommand(deviceID, "21");
		bool ret = cmd_function(deviceID, "21", pDeviceServer, DeviceServer::Stopped);
		// 受信確認してからreturnするべき
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
	}
	bool ai_device_set_ai_setting(int deviceID, ParamsAISetting * params) {
		pDeviceServer->init_params_buffer(deviceID);
		pDeviceServer->set_params(deviceID, *params);
		//pDeviceServer->setDeviceCommand(deviceID, "21");
		bool ret = cmd_function(deviceID, "21", pDeviceServer, DeviceServer::Stopped);
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
	}
	bool ai_device_set_data_detination_setting(int deviceID, ParamsDataDetinationSetting * params) {
		pDeviceServer->init_params_buffer(deviceID);
		pDeviceServer->set_params(deviceID, *params);
		//pDeviceServer->setDeviceCommand(deviceID, "21");
		bool ret = cmd_function(deviceID, "21", pDeviceServer, DeviceServer::Stopped);
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
	}
	bool ai_device_set_alarm_detination_setting(int deviceID, ParamsAlarmDetinationSetting * params) {
		pDeviceServer->init_params_buffer(deviceID);
		pDeviceServer->set_params(deviceID, *params);
		//pDeviceServer->setDeviceCommand(deviceID, "21");
		bool ret = cmd_function(deviceID, "21", pDeviceServer, DeviceServer::Stopped);
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
	}
	bool ai_device_set_device_setting(int deviceID, ParamsDeviceSetting * params) {
		pDeviceServer->init_params_buffer(deviceID);
		pDeviceServer->set_params(deviceID, *params);
		//pDeviceServer->setDeviceCommand(deviceID, "21");
		bool ret = cmd_function(deviceID, "21", pDeviceServer, DeviceServer::Stopped);
		return check_recv(deviceID, pDeviceServer, DeviceServer::Stopped);
	}

	// AIデバイス設定値を取得
	bool ai_device_get_sensor_setting(int deviceID, ParamsSensorSetting * params) {
		bool ret = cmd_function(deviceID, "20", pDeviceServer, DeviceServer::Stopped);
		// 受信完了まで待つ
		std::chrono::milliseconds dura( 1000 );
		int cnt = 0;
		while(cnt < 10) {
			if( pDeviceServer->getDeviceStatus(deviceID) == DeviceServer::Stopped ) {
				memcpy(params, pDeviceServer->p_sensor_map[deviceID], sizeof(ParamsSensorSetting));
				return true;
			}
			std::this_thread::sleep_for( dura );
			cnt++;
		}
		LOG(INFO) << "time over : receive setting_parameter";
		return false;
	}
	bool ai_device_get_trigger_setting(int deviceID, ParamsTriggerSetting * params) {
		bool ret = cmd_function(deviceID, "20", pDeviceServer, DeviceServer::Stopped);
		// 受信完了まで待つ
		std::chrono::milliseconds dura( 1000 );
		int cnt = 0;
		while(cnt < 10) {
			if( pDeviceServer->getDeviceStatus(deviceID) == DeviceServer::Stopped ) {
				memcpy(params, pDeviceServer->p_trigger_map[deviceID], sizeof(ParamsTriggerSetting));
				return true;
			}
			std::this_thread::sleep_for( dura );
			cnt++;
		}
		LOG(INFO) << "time over : receive setting_parameter";
		return false;
	}
	bool ai_device_get_ai_setting(int deviceID, ParamsAISetting * params) {
		bool ret = cmd_function(deviceID, "20", pDeviceServer, DeviceServer::Stopped);
		// 受信完了まで待つ
		std::chrono::milliseconds dura( 1000 );
		int cnt = 0;
		while(cnt < 10) {
			if( pDeviceServer->getDeviceStatus(deviceID) == DeviceServer::Stopped ) {
				memcpy(params, pDeviceServer->p_ai_map[deviceID], sizeof(ParamsAISetting));
				return true;
			}
			std::this_thread::sleep_for( dura );
			cnt++;
		}
		LOG(INFO) << "time over : receive setting_parameter";
		return false;
	}
	bool ai_device_get_data_detination_setting(int deviceID, ParamsDataDetinationSetting * params) {
		bool ret = cmd_function(deviceID, "20", pDeviceServer, DeviceServer::Stopped);
		// 受信完了まで待つ
		std::chrono::milliseconds dura( 1000 );
		int cnt = 0;
		while(cnt < 10) {
			if( pDeviceServer->getDeviceStatus(deviceID) == DeviceServer::Stopped ) {
				memcpy(params, pDeviceServer->p_data_map[deviceID], sizeof(ParamsDataDetinationSetting));
				return true;
			}
			std::this_thread::sleep_for( dura );
			cnt++;
		}
		LOG(INFO) << "time over : receive setting_parameter";
		return false;
	}
	bool ai_device_get_alarm_detination_setting(int deviceID, ParamsAlarmDetinationSetting * params) {
		bool ret = cmd_function(deviceID, "20", pDeviceServer, DeviceServer::Stopped);
		// 受信完了まで待つ
		std::chrono::milliseconds dura( 1000 );
		int cnt = 0;
		while(cnt < 10) {
			if( pDeviceServer->getDeviceStatus(deviceID) == DeviceServer::Stopped ) {
				memcpy(params, pDeviceServer->p_alarm_map[deviceID], sizeof(ParamsAlarmDetinationSetting));
				return true;
			}
			std::this_thread::sleep_for( dura );
			cnt++;
		}
		LOG(INFO) << "time over : receive setting_parameter";
		return false;
	}
	bool ai_device_get_device_setting(int deviceID, ParamsDeviceSetting * params) {
		bool ret = cmd_function(deviceID, "20", pDeviceServer, DeviceServer::Stopped);
		// 受信完了まで待つ
		std::chrono::milliseconds dura( 1000 );
		int cnt = 0;
		while(cnt < 10) {
			if( pDeviceServer->getDeviceStatus(deviceID) == DeviceServer::Stopped ) {
				memcpy(params, pDeviceServer->p_device_map[deviceID], sizeof(ParamsDeviceSetting));
				return true;
			}
			std::this_thread::sleep_for( dura );
			cnt++;
		}
		LOG(INFO) << "time over : receive setting_parameter";
		return false;
	}



}
