#include "code/kcp/fake_tcp.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <code/kcp/ikcp.h>
#include <code/kcp/kcp_socket.h>
#include "code/kcp/kcp_server.h"
#include "code/kcp/kcp_client.h"
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>



#define UDP_MTU 1400

const int MAX_EVENTS = 10;

// static int ip_id = 0;
// static uint32_t seq = 0, server_ack_seq = 0;
// static int CLIENT_SUM_SEND = 0;

// struct tcp_def
// {
// 	int32_t fd;
// 	struct sockaddr_in local_addr;
// 	struct sockaddr_in remote_addr;
// };

// typedef struct __cb_params__
// {
// 	int fd;
// 	ikcpcb *m_kcp;
// } cb_params;



// bool should_exit = false;

// int tcp_server_send_cb(const char *buffer, int len, ikcpcb *kcp, void *user)
// {
// 	tcp_def *def = (tcp_def *)user;
// 	int sended = 0;

// 	// std::cout << "server send cb len: " << len << std::endl;
// 	sockaddr_in client_addr;
// 	SetAddr(c_ip, c_port, &client_addr);
// 	socklen_t addrlen = sizeof(client_addr);

// 	while (sended < len)
// 	{
// 		size_t s = (len - sended);
// 		if (s > UDP_MTU)
// 			s = UDP_MTU;
// 		ssize_t ret = sendto(def->fd, buffer + sended, s, 0, (sockaddr *)&client_addr, addrlen);
// 		// std::cout << "server send to: " << ret << std::endl;
// 		if (ret < 0)
// 		{
// 			return -1;
// 		}
// 		sended += s;
// 	}
// 	return (size_t)sended;
// }

// void *server_loop(void *args)
// {

// 	cb_params *param = (cb_params *)args;
// 	int fd = param->fd;
// 	ikcpcb *m_kcp = param->m_kcp;
// 	assert(fd && m_kcp);
// 	while (!should_exit)
// 	{
// 		ikcp_update(m_kcp, KCP::iclock());
// 		KCP::isleep(1);
// 	}
// 	if (param)
// 	{
// 		delete param;
// 		param = nullptr;
// 	}
// 	return nullptr;
// }

// void *run_tcp_server(void *args)
// {

// 	// std::cout << "start tcp_server thread: " << std::endl;

// 	cb_params *param = (cb_params *)args;

// 	int fd = param->fd;
// 	ikcpcb *m_kcp = param->m_kcp;
// 	assert(fd && m_kcp);
// 	// struct sockaddr_in from_addr;
// 	// socklen_t from_size = sizeof(sockaddr_in);
// 	// SetAddr(s_ip, s_port, &from_addr);

// 	std::string prefix_path = "/home/hzh/workspace/work/bin/";
// 	std::string file_path;
// 	ssize_t len = 0;
// 	char buffer[2048] = {0};
// 	std::ofstream file;
// 	if (!file)
// 	{
// 		exit(1);
// 	}
// 	bool read_file = false;
// 	struct sockaddr_in src;
// 	socklen_t src_len = sizeof(struct sockaddr_in);
// 	SetAddr(c_ip, c_port, &src);
// 	uint32_t file_sended = 0;
// 	uint32_t file_size = 0;
// 	while (!should_exit)
// 	{
// 		ikcp_update(m_kcp, KCP::iclock());
// 		len = recvfrom(fd, buffer, sizeof(buffer), 0, (sockaddr *)&src, &src_len);
// 		// std::cout << "server recv len: " << len << std::endl;
// 		if (len > 0)
// 		{

// 			int ret = ikcp_input(m_kcp, buffer, len);
// 			// std::cout << "server ikcp_input ret: " << ret << std::endl;
// 			if (ret < 0)
// 			{
// 				printf("ikcp_input error: %d\n", ret);
// 				continue;
// 			}

// 			// 发送8 + 128字节确认文件大小，文件名
// 			char recv_buffer[2048] = {0};
// 			ret = ikcp_recv(m_kcp, recv_buffer, len);
// 			if (ret > 0)
// 			{
// 				if (!read_file)
// 				{
// 					assert(ret >= 128 + 8);
// 					read_file = true;

// 					memcpy(&file_size, recv_buffer, sizeof(uint32_t));
// 					file_size = ntohl(file_size);
// 					printf("File size: %u\n", file_size);

// 					char file_name[128]{};
// 					memcpy(file_name, recv_buffer + sizeof(uint32_t), sizeof(file_name));
// 					printf("File name: %s\n", file_name);
// 					file_path = prefix_path + file_name;
// 					file.open(file_path, std::ios::out | std::ios::binary);

// 					if (len > 8 + 128)
// 					{
// 						file.write(recv_buffer + 8 + 128, ret - (8 + 128));
// 						file_sended += ret - (8 + 128);
// 					}
// 					continue;
// 				}
// 				file.write(recv_buffer, ret);
// 				file_sended += ret;
// 				if (file_sended >= file_size)
// 				{
// 					printf("File %s received completely\n", file_path.c_str());
// 					break;
// 				}
// 				// printf("ikcp_recv ret = %d,buf=%s\n",ret, recv_buffer);
// 			}
// 			else if (!ret)
// 			{
// 				break;
// 			}
// 			else
// 			{
// 				perror("ikcp_recv");
// 			}
// 		}
// 		else if (!len)
// 		{
// 			break;
// 		}
// 		else
// 		{
// 			perror("ikcp_recv");
// 		}
// 		KCP::isleep(10);
// 	}

// 	// std::cout << "server: close --------------------------" << std::endl;
// 	if (param)
// 	{
// 		delete param;
// 		param = nullptr;
// 	}
// 	file.close();
// 	should_exit = true;
// 	return nullptr;
// }

// void start_kcp_server(int fd, ikcpcb *m_kcp)
// {
// 	// char buf[65535];
// 	assert(m_kcp && fd);
// 	ikcp_setoutput(m_kcp, tcp_server_send_cb);

// 	int sndwnd = 128, rcvwnd = 128;
// 	ikcp_wndsize(m_kcp, sndwnd, rcvwnd);
// 	int mod = 0;
// 	switch (mod)
// 	{
// 	case 0:
// 		ikcp_nodelay(m_kcp, 0, 40, 0, 0);
// 	case 1:
// 		break;
// 		ikcp_nodelay(m_kcp, 0, 10, 0, 1);
// 		break;
// 	case 2:
// 		// 启动快速模式
// 		// 第二个参数 nodelay-启用以后若干常规加速将启动
// 		// 第三个参数 interval为内部处理时钟，默认设置为 10ms
// 		// 第四个参数 resend为快速重传指标，设置为2
// 		// 第五个参数 为是否禁用常规流控，这里禁止
// 		ikcp_nodelay(m_kcp, 2, 10, 2, 1);
// 		m_kcp->rx_minrto = 10;
// 		m_kcp->fastresend = 1;
// 		break;

// 	default:
// 		assert(false);
// 		break;
// 	}

// 	cb_params *param = new cb_params;
// 	param->fd = fd;
// 	param->m_kcp = m_kcp;

// 	pthread_t server_tid;
// 	if (pthread_create(&server_tid, NULL, server_loop, param) != 0)
// 	{
// 		perror("pthread_create failed");
// 		return;
// 	}

// 	cb_params *param1 = new cb_params;
// 	param1->fd = fd;
// 	param1->m_kcp = m_kcp;
// 	pthread_t tid;
// 	if (pthread_create(&tid, NULL, run_tcp_server, param1) != 0)
// 	{
// 		perror("pthread_create failed");
// 		return;
// 	}
// }

// void ServerRun(int fd, ikcpcb *m_kcp)
// {
// 	start_kcp_server(fd, m_kcp);

// 	std::string msg;
// 	// std::cout << "send: ";
// 	// while (std::cin >> msg) {
// 	//     int ret = ikcp_send(m_kcp, msg.c_str(), msg.size());
// 	//     // std::cout << "data_size: " << ret << std::endl;
// 	// 	if (ret < 0) {
// 	// 		perror("send");
// 	// 	}
// 	// }
// }

// enum conn_state
// {
// 	tcp_closed = 0,
// 	tcp_syn_sent = 1,
// 	tcp_established = 2,
// };

// static int s_state = tcp_closed;

// // IP 校验和计算函数
// uint16_t calculate_ip_checksum(const struct iphdr *iphdr_packet)
// {
// 	uint32_t sum = 0;
// 	int length = iphdr_packet->ihl * 4;
// 	const uint16_t *buf = reinterpret_cast<const uint16_t *>(iphdr_packet);

// 	for (int i = 0; i < length / 2; ++i)
// 	{
// 		sum += ntohs(buf[i]);
// 	}

// 	if (length % 2)
// 	{
// 		sum += static_cast<uint16_t>(*(reinterpret_cast<const uint8_t *>(iphdr_packet) + length - 1));
// 	}

// 	while (sum >> 16)
// 	{
// 		sum = (sum & 0xffff) + (sum >> 16);
// 	}

// 	return static_cast<uint16_t>(~sum);
// }

// struct pseudo_header
// {
// 	uint32_t source_address;
// 	uint32_t dest_address;
// 	uint8_t placeholder;
// 	uint8_t protocol;
// 	uint16_t tcp_length;
// };

// uint16_t calculate_tcp_checksum(const struct iphdr *iphdr_packet, const struct tcphdr *tcphdr_packet, const char *data, size_t data_len)
// {
// 	uint32_t sum = 0;

// 	pseudo_header pseudo_hdr;
// 	pseudo_hdr.source_address = iphdr_packet->saddr;
// 	pseudo_hdr.dest_address = iphdr_packet->daddr;
// 	pseudo_hdr.placeholder = 0;
// 	pseudo_hdr.protocol = IPPROTO_TCP;
// 	pseudo_hdr.tcp_length = htons(sizeof(struct tcphdr) + data_len);

// 	const uint16_t *pseudo_header_words = reinterpret_cast<const uint16_t *>(&pseudo_hdr);
// 	int pseudo_length = sizeof(pseudo_hdr);
// 	for (int i = 0; i < pseudo_length / 2; ++i)
// 	{
// 		sum += ntohs(pseudo_header_words[i]);
// 	}

// 	const uint16_t *tcp_packet = reinterpret_cast<const uint16_t *>(tcphdr_packet);
// 	int tcp_length = sizeof(struct tcphdr);
// 	for (int i = 0; i < tcp_length / 2; ++i)
// 	{
// 		sum += ntohs(tcp_packet[i]);
// 	}

// 	char *new_data = new char[data_len + 1];
// 	std::memcpy(new_data, data, data_len);
// 	if (data_len & 1)
// 	{
// 		new_data[data_len] = 0;
// 		data_len++;
// 	}

// 	const uint16_t *data_packet = reinterpret_cast<const uint16_t *>(new_data);
// 	int data_length = data_len;
// 	for (int i = 0; i < data_length / 2; ++i)
// 	{
// 		sum += ntohs(data_packet[i]);
// 	}

// 	while (sum >> 16)
// 	{
// 		sum = (sum & 0xffff) + (sum >> 16);
// 	}

// 	delete[] new_data;
// 	return static_cast<uint16_t>(~sum);
// }

// int tcp_client_cb(const char *buffer, int len, ikcpcb *kcp, void *user)
// {
// 	tcp_def *def = (tcp_def *)user;

// 	// std::cout << "server_ack_seq: " << server_ack_seq << std::endl;

// 	int sended = 0;
// 	while (sended < len)
// 	{
// 		size_t s = (len - sended);
// 		if (s > UDP_MTU)
// 			s = UDP_MTU;

// 		int data_len = s;
// 		/*
// 		模拟TCP
// 		*/

// 		// 构造 IP 头部
// 		struct iphdr ip_header;
// 		std::memset(&ip_header, 0, sizeof(ip_header));
// 		ip_header.ihl = 5;
// 		ip_header.version = IPVERSION;
// 		ip_header.tos = 0;
// 		ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len;
// 		ip_header.id = htons(++ip_id);
// 		ip_header.frag_off = 0;
// 		ip_header.ttl = 255;
// 		ip_header.protocol = IPPROTO_TCP;
// 		ip_header.check = 0;

// 		ip_header.saddr = inet_addr(inet_ntoa(def->local_addr.sin_addr));
// 		ip_header.daddr = inet_addr(inet_ntoa(def->remote_addr.sin_addr));

// 		struct tcphdr tcp_header;
// 		std::memset(&tcp_header, 0, sizeof(tcp_header));
// 		tcp_header.source = htons(c_port);			   // 源端口号
// 		tcp_header.dest = htons(s_port);			   // 目标端口号
// 		tcp_header.seq = htonl(seq + CLIENT_SUM_SEND); // 序列号
// 		tcp_header.ack_seq = htonl(server_ack_seq);	   // 确认号
// 		tcp_header.doff = 5;						   // TCP头部长度
// 		tcp_header.fin = 0;
// 		tcp_header.syn = 0;				 // SYN标志位
// 		tcp_header.rst = 0;				 // RST标志位
// 		tcp_header.psh = 1;				 // PSH标志位
// 		tcp_header.ack = 1;				 // ACK标志位
// 		tcp_header.urg = 0;				 // URG标志位
// 		tcp_header.window = htons(8192); // 窗口大小
// 		tcp_header.urg_ptr = 0;			 // 紧急指针
// 		tcp_header.check = 0;

// 		// ip_hdr.ip_sum = checksum(&ip_header, sizeof(ip_header));
// 		ip_header.check = calculate_ip_checksum(&ip_header);
// 		tcp_header.check = htons(calculate_tcp_checksum(&ip_header, &tcp_header, buffer, data_len));

// 		// // std::cout << "tcp_check: " << htons(tcp_header.check) << std::endl;
// 		// 计算 TCP 校验和
// 		// 构造数据包

// 		char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len];
// 		std::memcpy(packet, &ip_header, sizeof(struct iphdr));
// 		std::memcpy(packet + sizeof(struct iphdr), &tcp_header, sizeof(struct tcphdr));
// 		std::memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), buffer + sended, data_len);

// 		struct sockaddr_in dest;
// 		SetAddr(inet_ntoa(def->remote_addr.sin_addr), def->remote_addr.sin_port, &dest);

// 		ssize_t ret = sendto(def->fd, packet, sizeof(packet), 0, (struct sockaddr *)&dest, sizeof(sockaddr));
// 		// std::cout << "ret client send: " << ret << std::endl;
// 		if (ret < 0)
// 		{
// 			perror("sendto");
// 			break;
// 		}
// 		CLIENT_SUM_SEND += data_len;

// 		// ssize_t ret = send(def->fd, buffer + sended, s, 0);
// 		// if(ret < 0){
// 		//     return -1;
// 		// }
// 		sended += s;
// 	}

// 	return (size_t)sended;
// }

// void *client_loop(void *args)
// {
// 	cb_params *param = (cb_params *)args;
// 	ikcpcb *m_kcp = param->m_kcp;
// 	while (!should_exit)
// 	{
// 		ikcp_update(m_kcp, KCP::iclock());
// 		KCP::isleep(1);
// 	}

// 	delete param;
// 	return nullptr;
// }

// void run_tcp_client(int fd, ikcpcb *m_kcp)
// {

// 	while (true)
// 	{
// 		ssize_t len = 0;
// 		char buffer[2048] = {0};

// 		while (true)
// 		{
// 			// ikcp_update(m_kcp, KCP::iclock());
// 			struct sockaddr_in src;
// 			socklen_t src_len = sizeof(struct sockaddr_in);
// 			SetAddr(s_ip, s_port, &src);
// 			len = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src, &src_len);

// 			if (len > 0)
// 			{
// 				KCP::tcp_info info;
// 				KCP::prase_tcp_packet(buffer, len, &info);
// 				if (info.port_dst != c_port)
// 				{
// 					continue;
// 				}
// 				// std::cout << info.port_src << " " << info.port_dst << " " <<  info.seq << " " << info.ack_seq << std::endl;
// 				server_ack_seq = info.seq + len - (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr));
// 				// std::cout << "client recv len: " << len << std::endl;
// 				// ikcp_send(m_kcp, "", 0);
// 				if (len > (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr)))
// 				{
// 					int ret = ikcp_input(m_kcp, buffer + sizeof(struct iphdr) + sizeof(struct tcphdr), len - (sizeof(struct iphdr) + sizeof(struct tcphdr)));
// 					if (ret < 0)
// 					{
// 						printf("ikcp_input error: %d\n", ret);
// 						continue;
// 					}

// 					char recv_buffer[2048] = {0};
// 					ret = ikcp_recv(m_kcp, recv_buffer, len);
// 					if (ret > 0)
// 					{
// 						printf("ikcp_recv ret = %d,buf=%s\n", ret, recv_buffer);
// 					}
// 				}
// 			}
// 			KCP::isleep(1);
// 		}
// 	}
// 	close(fd);
// }

// void kcp_client_start(int fd, ikcpcb *m_kcp)
// {
// 	// char buf[65535];
// 	ikcp_setoutput(m_kcp, tcp_client_cb);

// 	int sndwnd = 128, rcvwnd = 128;
// 	ikcp_wndsize(m_kcp, sndwnd, rcvwnd);
// 	int mod = 0;
// 	switch (mod)
// 	{
// 	case 0:
// 		ikcp_nodelay(m_kcp, 0, 10, 0, 0);
// 	case 1:
// 		break;
// 		ikcp_nodelay(m_kcp, 0, 10, 0, 1);
// 		break;
// 	case 2:
// 		// 启动快速模式
// 		// 第二个参数 nodelay-启用以后若干常规加速将启动
// 		// 第三个参数 interval为内部处理时钟，默认设置为 10ms
// 		// 第四个参数 resend为快速重传指标，设置为2
// 		// 第五个参数 为是否禁用常规流控，这里禁止
// 		ikcp_nodelay(m_kcp, 2, 10, 2, 1);
// 		m_kcp->rx_minrto = 10;
// 		m_kcp->fastresend = 1;
// 		break;

// 	default:
// 		assert(false);
// 		break;
// 	}

// 	cb_params *params = new cb_params;
// 	params->fd = fd;
// 	params->m_kcp = m_kcp;
// 	pthread_t loop_tid;
// 	if (pthread_create(&loop_tid, NULL, client_loop, params) != 0)
// 	{
// 		perror("pthread_create");
// 	}

// 	std::thread server_thread(run_tcp_client, fd, m_kcp);
// 	server_thread.detach();
// }

// void ClientRun(int fd, ikcpcb *m_kcp)
// {
// 	char buf[65536] = {0};
// 	struct sockaddr_in server_addr;
// 	SetAddr(s_ip, s_port, &server_addr);

// 	struct in_addr client_in_addr, server_in_addr;
// 	inet_aton(c_ip, &client_in_addr);
// 	inet_aton(c_ip, &server_in_addr);

// 	uint8_t syn = 0, ack = 0;
// 	int ack_seq = 0;
// 	struct pkt_info info;

// 	while (1)
// 	{
// 		char *data = ReserveHdrSize(buf);
// 		ssize_t _s = 0;

// 		printf("s_state: %d  ----------------------\n", s_state);

// 		if (s_state == tcp_closed)
// 		{
// 			seq = 12345;
// 			syn = 1;
// 			s_state = tcp_syn_sent;
// 		}
// 		else if (s_state == tcp_syn_sent)
// 		{
// 			if (info.ack_seq != seq + 1)
// 			{
// 				printf("invalid ack_seq: %d\n", info.ack_seq);
// 				break;
// 			}

// 			seq++;
// 			syn = 0;
// 			ack_seq = info.seq + 1;
// 			ack = 1;
// 			s_state = tcp_established;
// 		}
// 		else
// 		{
// 			ack_seq = info.seq + info.data_len;
// 			seq += _s;
// 		}

// 		int total = BuildPkt(data, _s, &client_in_addr, c_port, &server_in_addr, s_port, seq, ack_seq, syn, ack);

// 		_s = sendto(fd, buf, total, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
// 		if (_s < 0)
// 		{
// 			perror("sendto");
// 			break;
// 		}

// 		if (s_state == tcp_established)
// 		{
// 			server_ack_seq = info.seq + 1;
// 			kcp_client_start(fd, m_kcp);

// 			std::string filepath = "/home/hzh/workspace/work/logs/data.txt";
// 			// 打开文件并发送内容
// 			std::string filename = filepath.substr(filepath.find_last_of("/") + 1);

// 			std::ifstream file(filepath, std::ios::in | std::ios::binary);
// 			if (!file)
// 			{
// 				exit(1);
// 			}

// 			char temp[8 + 128]{};
// 			// 获取文件大小
// 			file.seekg(0, std::ios::end);
// 			uint32_t file_size = file.tellg();
// 			file.seekg(0, std::ios::beg);

// 			file_size = htonl(file_size);
// 			memcpy(temp, &file_size, sizeof(uint32_t));
// 			memcpy(temp + sizeof(uint32_t), &filename[0], filename.size());
// 			ikcp_send(m_kcp, temp, sizeof(temp));

// 			char buf[1024];
// 			size_t BUFFER_SIZE = 1024;
// 			while (!file.eof())
// 			{
// 				file.read(buf, BUFFER_SIZE);
// 				ikcp_send(m_kcp, buf, BUFFER_SIZE);
// 			}
// 			file.close();

// 			std::cout << "send file finished" << std::endl;
// 			std::string msg = "send file finished";
// 			// ikcp_send(m_kcp, msg.c_str(), msg.size());

// 			return;

// 			// std::cout << "sned:";
// 			while (std::cin >> msg)
// 			{
// 				ssize_t ret = ikcp_send(m_kcp, msg.c_str(), msg.size());
// 				std::cout << "ret send: " << ret << std::endl;
// 			}
// 			return;
// 			int sum = 0;
// 			while (std::cin >> msg)
// 			{
// 				ssize_t data_len = msg.size();

// 				// 构造 IP 头部
// 				struct iphdr ip_header;
// 				std::memset(&ip_header, 0, sizeof(ip_header));
// 				ip_header.ihl = 5;
// 				ip_header.version = IPVERSION;
// 				ip_header.tos = 0;
// 				ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len;
// 				ip_header.id = htons(++ip_id);
// 				ip_header.frag_off = 0;
// 				ip_header.ttl = 255;
// 				ip_header.protocol = IPPROTO_TCP;
// 				ip_header.check = 0;
// 				ip_header.saddr = inet_addr(c_ip);
// 				ip_header.daddr = inet_addr(s_ip);

// 				struct tcphdr tcp_header;
// 				std::memset(&tcp_header, 0, sizeof(tcp_header));
// 				tcp_header.source = htons(c_port);		  // 源端口号
// 				tcp_header.dest = htons(s_port);		  // 目标端口号
// 				tcp_header.seq = htonl(seq + sum);		  // 序列号
// 				tcp_header.ack_seq = htonl(info.seq + 1); // 确认号
// 				tcp_header.doff = 5;					  // TCP头部长度
// 				tcp_header.fin = 0;
// 				tcp_header.syn = 0;				 // SYN标志位
// 				tcp_header.rst = 0;				 // RST标志位
// 				tcp_header.psh = 1;				 // PSH标志位
// 				tcp_header.ack = 1;				 // ACK标志位
// 				tcp_header.urg = 0;				 // URG标志位
// 				tcp_header.window = htons(8192); // 窗口大小
// 				tcp_header.urg_ptr = 0;			 // 紧急指针
// 				tcp_header.check = 0;

// 				// ip_hdr.ip_sum = checksum(&ip_header, sizeof(ip_header));
// 				ip_header.check = calculate_ip_checksum(&ip_header);
// 				tcp_header.check = htons(calculate_tcp_checksum(&ip_header, &tcp_header, msg.c_str(), data_len));
// 				// std::cout << "tcp_check: " << htons(tcp_header.check) << std::endl;
// 				// 计算 TCP 校验和
// 				// 构造数据包

// 				char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len];
// 				std::memcpy(packet, &ip_header, sizeof(struct iphdr));
// 				std::memcpy(packet + sizeof(struct iphdr), &tcp_header, sizeof(struct tcphdr));
// 				std::memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), msg.c_str(), data_len);

// 				struct sockaddr_in dest;
// 				SetAddr(s_ip, s_port, &dest);

// 				int ret = sendto(fd, packet, sizeof(packet), 0, (struct sockaddr *)&dest, sizeof(struct sockaddr));
// 				// std::cout << "ret client send: " << ret << std::endl;
// 				if (ret < 0)
// 				{
// 					perror("sendto");
// 					break;
// 				}
// 				sum += data_len;
// 			}
// 			// 关闭套接字
// 			close(fd);
// 		}

// 		memset(buf, 0, sizeof(buf));
// 		struct sockaddr_in from_addr;
// 		SetAddr(s_ip, s_port, &from_addr);
// 		socklen_t from_size;
// 		from_size = sizeof(struct sockaddr_in);

// 		if (s_state != tcp_established)
// 		{
// 			ssize_t sz = recv_from_addr(fd, buf, sizeof(buf), 0, &from_addr, &from_size, &info);
// 			if (sz <= 0)
// 			{
// 				break;
// 			}
// 		}
// 	}
// }

int setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags" << std::endl;
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking mode" << std::endl;
        return -1;
    }

    return 0;
}

int main(int argc, char *argv[])
{
	static const char *s_ip = "127.0.0.1";
	static const char *c_ip = "127.0.0.1";
	static short s_port = 6666;
	static short c_port = 6668;

	std::string file_path;
	int server = 0;
	if (argc >= 2 && strncmp(argv[1], "-s", 2) == 0)
	{
		server = 1;
	}
	
	if (argc >= 4) {
		// if (argc >= 4 && strncmp(argv[2], "-p", 2) == 0) {
		// 	c_port = atoi(argv[3]);
		// }
		file_path = argv[2];
		c_port = atoi(argv[3]);
	}

	printf("run in %s mode...\n", server ? "server" : "client");

	int sock = server ? StartServer(s_ip, s_port) : StartFakeTcp(c_ip, c_port);

	// ikcpcb *ikcpcb;
	// tcp_def *def;
	// std::cout << "fd: " << fd << std::endl;


	//std::vector<std::unique_ptr<KCP::KcpHandleClient>> clients;
	std::map<int, std::unique_ptr<KCP::KcpHandleClient>> clients;

	if (sock < 0) {
		exit(1);
	}
	if (server) {
		int listen_sock = sock;
		int epoll_fd = epoll_create1(0);
		if (epoll_fd == -1)
		{
			perror("epoll_create1");
			close(listen_sock);
			exit(1);
		}

		struct epoll_event ev;
		ev.events = EPOLLIN;
		ev.data.fd = listen_sock;
		if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
		{
			perror("epoll_ctl: listen_sock");
			close(listen_sock);
			close(epoll_fd);
			exit(1);
		}

		struct epoll_event events[MAX_EVENTS];
		while (true) {
			int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
			if (nfds == -1) {
				perror("epoll_wait");
				break;
			}
			std::cout << "nfds: " << nfds << std::endl;
			for (int i = 0; i < nfds; ++i) {
				std::cout << "event_data_fd: " << events[i].data.fd << std::endl;
				if (events[i].data.fd == listen_sock) {
					struct sockaddr_in peer;
					socklen_t len = sizeof(peer);
					int fd = accept(listen_sock, (struct sockaddr *)&peer, &len);
					if (fd == -1) {
						perror("accept");
						continue;
					}
					std::cout << "New connection from: " << inet_ntoa(peer.sin_addr) << ":" << ntohs(peer.sin_port) <<
					"new_fd: " << fd << std::endl;

					ev.events = EPOLLRDHUP;
					ev.data.fd = fd;
					if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
						perror("epoll_ctl: conn_sock");
                        close(fd);
                        continue;
					}
					std::cout << "Epoll create handleClient" << std::endl;
					std::unique_ptr<KCP::KcpHandleClient>handleClient(new KCP::KcpHandleClient(fd, s_port, s_ip, c_port, c_ip));
					handleClient->start_kcp_server();
					// clients.push_back(std::move(handleClient));
					assert(!clients.count(fd));
					clients[fd] = std::move(handleClient);
					

					// 设置新连接为非阻塞
					// if (setNonBlocking(fd) == -1)
					// {
					// 	close(fd);
					// 	continue;
					// }

					// ev.events = EPOLLIN | EPOLLET;
					// ev.data.fd = fd;
					// if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1)
					// {
					// 	perror("epoll_ctl: conn_sock");
					// 	close(fd);
					// 	continue;
					// }
				} else {
					if (events[i].events & (EPOLLRDHUP)) {
						std::cout << "client send fin" << std::endl;
						char buffer[2048] = {};
						int n = read(events[i].data.fd, buffer, sizeof(buffer));
						std::cout << "n: " << n << std::endl;
						if (!n) {
							std::cout << "closing, close fd" << std::endl;
							assert(clients.count(events[i].data.fd));
							clients.erase(clients.find(events[i].data.fd));
							close(events[i].data.fd);
							
						} else {
							perror("clients");
						}
					}
					// ev.events = EPOLLIN | EPOLLET;
					// epoll_ctl(epoll_fd, EPOLL_CTL_DEL, events[i].data.fd, &ev);
				}
			}
		}

		// def = new tcp_def;
		// def->fd = fd;
		// SetAddr(s_ip, s_port, &def->local_addr);
		// SetAddr(c_ip, c_port, &def->remote_addr);
		// ikcpcb = ikcp_create(0x1, (void*)def);
		// ServerRun(fd, ikcpcb);
		// while (true) {
		// 	sleep(1);
		// }
	}
	else {
		if (file_path.empty()) {
			file_path = "/home/hzh/workspace/work/logs/data.txt";
		}

		std::unique_ptr<KCP::KcpClient> client(new KCP::KcpClient(sock, c_port, s_port, c_ip, s_ip, file_path));
		client->startHand();
		// // testClient(fd);
		// def = new tcp_def;
		// def->fd = sock;
		// SetAddr(c_ip, c_port, &def->local_addr);
		// SetAddr(s_ip, s_port, &def->remote_addr);
		// ikcpcb = ikcp_create(0x1, (void *)def);
		// ClientRun(sock, ikcpcb);
		// sleep(5);
		// should_exit = true;
		// if (def) {
		// 	delete def;
		// }
		// ikcp_release(ikcpcb);
	}

	// std::cout << "looping" << std::endl;

	// std::cout << "stop main: " << std::endl;

	close(sock);
	return 0;
}
