#include "kcp_client.h"
#include <iostream>

namespace KCP {

int tcp_client_cb(const char *buffer, int len, ikcpcb *kcp, void *user)
{
	KcpClient *client = static_cast<KcpClient*>(user);
	std::cout <<"client fd: " << client->fd << std::endl;

	int sended = 0;
	while (sended < len)
	{
		size_t s = (len - sended);
		if (s > UDP_MTU)
			s = UDP_MTU;

		int data_len = s;
		/*
		模拟TCP
		*/

		// 构造 IP 头部
		struct iphdr ip_header;
		std::memset(&ip_header, 0, sizeof(ip_header));
		ip_header.ihl = 5;
		ip_header.version = IPVERSION;
		ip_header.tos = 0;
		ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len;
		ip_header.id = htons(++client->ip_id);
		ip_header.frag_off = 0;
		ip_header.ttl = 255;
		ip_header.protocol = IPPROTO_TCP;
		ip_header.check = 0;

		ip_header.saddr = inet_addr(client->c_ip);
		ip_header.daddr = inet_addr(client->s_ip);

		struct tcphdr tcp_header;
		std::memset(&tcp_header, 0, sizeof(tcp_header));
		tcp_header.source = htons(client->c_port);			   // 源端口号
		tcp_header.dest = htons(client->s_port);			   // 目标端口号
		tcp_header.seq = htonl(client->seq + client->CLIENT_SUM_SEND.load()); // 序列号
		tcp_header.ack_seq = htonl(client->server_ack_seq.load());	   // 确认号
		tcp_header.doff = 5;						   // TCP头部长度
		tcp_header.fin = 0;
		tcp_header.syn = 0;				 // SYN标志位
		tcp_header.rst = 0;				 // RST标志位
		tcp_header.psh = 1;				 // PSH标志位
		tcp_header.ack = 1;				 // ACK标志位
		tcp_header.urg = 0;				 // URG标志位
		tcp_header.window = htons(8192); // 窗口大小
		tcp_header.urg_ptr = 0;			 // 紧急指针
		tcp_header.check = 0;

		// ip_hdr.ip_sum = checksum(&ip_header, sizeof(ip_header));
		ip_header.check = calculate_ip_checksum(&ip_header);
		tcp_header.check = htons(calculate_tcp_checksum(&ip_header, &tcp_header, buffer, data_len));

		// // // std::cout <<"tcp_check: " << htons(tcp_header.check) << std::endl;
		// 计算 TCP 校验和
		// 构造数据包

		char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len];
		std::memcpy(packet, &ip_header, sizeof(struct iphdr));
		std::memcpy(packet + sizeof(struct iphdr), &tcp_header, sizeof(struct tcphdr));
		std::memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), buffer + sended, data_len);

		struct sockaddr_in dest;
		setAddr(client->s_ip, client->s_port, &dest);

		ssize_t ret = sendto(client->fd, packet, sizeof(packet), 0, (struct sockaddr *)&dest, sizeof(sockaddr));
		// std::cout <<"ret client send: " << ret << std::endl;
		if (ret < 0)
		{
			perror("sendto");
			break;
		}
		client->CLIENT_SUM_SEND.fetch_add(data_len);
		sended += s;
	}

	return (size_t)sended;
}

KcpClient::KcpClient(int fd, uint16_t c_port, uint16_t s_port, const char* c_ip, const char* s_ip, std::string file_path)
 	:fd(fd), c_port(c_port), s_port(s_port), c_ip(c_ip), s_ip(s_ip), file_path(file_path){

	s_state = TCP_CLOSED;

	stopFlag.store(false);
	finishSend.store(false);

	ip_id = 1231;

	server_ack_seq.store(0);
	CLIENT_SUM_SEND.store(0);
	
	std::srand(static_cast<unsigned>(std::time(0)));
	seq = rand();
	
	m_kcp = ikcp_create(c_port, this);
	assert(m_kcp);
}

KcpClient::~KcpClient() {
	// std::cout << "~" << fd << std::endl;
	Close();
}


void KcpClient::build_ip_tcp_header(char* data, const char* buffer, size_t data_len, int ack, int psh, int syn, int fin) {
	
	struct iphdr ip_header;
	std::memset(&ip_header, 0, sizeof(ip_header));
	ip_header.ihl = 5;
	ip_header.version = IPVERSION;
	ip_header.tos = 0;
	ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len;
	ip_header.id = htons(++ip_id);
	ip_header.frag_off = 0;
	ip_header.ttl = 255;
	ip_header.protocol = IPPROTO_TCP;
	ip_header.check = 0;
	ip_header.saddr = inet_addr(c_ip);
	ip_header.daddr = inet_addr(s_ip);

	struct tcphdr tcp_header;
	std::memset(&tcp_header, 0, sizeof(tcp_header));
	tcp_header.source = htons(c_port);			   // 源端口号
	tcp_header.dest = htons(s_port);			   // 目标端口号
	tcp_header.seq = htonl(seq + CLIENT_SUM_SEND.load()); // 序列号
	tcp_header.ack_seq = htonl(server_ack_seq.load());	   // 确认号
	tcp_header.doff = 5;						   // TCP头部长度
	tcp_header.fin = fin;
	tcp_header.syn = syn;				 // SYN标志位
	tcp_header.rst = 0;				 // RST标志位
	tcp_header.psh = psh;				 // PSH标志位
	tcp_header.ack = ack;				 // ACK标志位
	tcp_header.urg = 0;				 // URG标志位
	tcp_header.window = htons(8192); // 窗口大小
	tcp_header.urg_ptr = 0;			 // 紧急指针
	tcp_header.check = 0;

	// ip_hdr.ip_sum = checksum(&ip_header, sizeof(ip_header));
	ip_header.check = calculate_ip_checksum(&ip_header);
	tcp_header.check = htons(calculate_tcp_checksum(&ip_header, &tcp_header, buffer, data_len));

	
	memcpy(data, &ip_header, IPV4_HEADER_SIZE);
	memcpy(data + IPV4_HEADER_SIZE, &tcp_header, TCP_HEADER_SIZE);
	memcpy(data + IP_TCP_HEADER_SIZE, buffer, data_len);

	
	CLIENT_SUM_SEND.fetch_add(data_len);
}

void KcpClient::run_tcp_client() {
	// std::cout <<"start run_tcp_client" << std::endl;
	ssize_t len = 0;
	char buffer[2048] = {0};
	while (!stopFlag.load()) {
		// ikcp_update(m_kcp, KCP::iclock());
		struct sockaddr_in src;
		socklen_t src_len = sizeof(struct sockaddr_in);
		setAddr(s_ip, s_port, &src);
		len = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src, &src_len);

		if (len > 0)
		{
			KCP::tcp_info info;
			KCP::prase_tcp_packet(buffer, len, &info);
			if (info.port_dst != c_port) {
				continue;
			}
			server_ack_seq.store(info.seq + len - (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr)));
			// // std::cout <<"client recv len: " << len << std::endl;
			// ikcp_send(m_kcp, "", 0);
			

			if (len > (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr)))
			{
				
				// // 如果不是ack包，发送确认收到
				// char send_buffer[41] = {};
				// build_ip_tcp_header(send_buffer, "", 0, 1, 1, 0, 0);
				// ::sendto(fd, send_buffer, IP_TCP_HEADER_SIZE, 0, (sockaddr*)&src, src_len);


				int ret = ikcp_input(m_kcp, buffer + sizeof(struct iphdr) + sizeof(struct tcphdr), len - (sizeof(struct iphdr) + sizeof(struct tcphdr)));
				std::cout << "ret: " << ret << std::endl;
				if (ret < 0)
				{
					printf("fd: %d port: %d ikcp_input error: %d\n", fd, c_port, ret);
					KCP::tcp_info info;
					KCP::prase_tcp_packet(buffer, len, &info);
					if (info.fin) {
						std::cout << "recv fin" << std::endl;
						break;
					}
					continue;
				}

				char recv_buffer[2048] = {0};


				ret = ikcp_recv(m_kcp, recv_buffer, len);
				if (ret > 0)
				{
					printf("ikcp_recv ret = %d,buf=%s\n", ret, recv_buffer);
					// stopFlag.store(true);
					if (!strcmp(recv_buffer, "send_finish")) {
						finishSend.store(true);
					}
				}
			}
		} else if (!len) {
			// std::cout <<"start recv 0" << std::endl;
			KCP::tcp_info info;
			KCP::prase_tcp_packet(buffer, len, &info);
			if (info.port_dst != c_port) {
				continue;
			}
			if (info.fin) {
				break;
			}
		}
		KCP::isleep(1);
	}
}

void* KcpClient::client_loop()
{
	// std::cout <<"start client loop" << std::endl;
	int cnt = 0;
	while (!stopFlag.load())
	{
		if (++cnt % 1000 == 0) {
			std::cout << "fd: " << fd << "port: " << c_port << "looping" << std::endl;
		}
		ikcp_update(m_kcp, KCP::iclock());
		KCP::isleep(10);
	}

	return nullptr;
}

void KcpClient::kcp_client_start()
{
	// char buf[65535];
	ikcp_setoutput(m_kcp, tcp_client_cb);

	int sndwnd = 128, rcvwnd = 128;
	ikcp_wndsize(m_kcp, sndwnd, rcvwnd);
	int mod = 0;
	switch (mod)
	{
	case 0:
		ikcp_nodelay(m_kcp, 0, 10, 0, 0);
	case 1:
		break;
		ikcp_nodelay(m_kcp, 0, 10, 0, 1);
		break;
	case 2:
		// 启动快速模式
		// 第二个参数 nodelay-启用以后若干常规加速将启动
		// 第三个参数 interval为内部处理时钟，默认设置为 10ms
		// 第四个参数 resend为快速重传指标，设置为2
		// 第五个参数 为是否禁用常规流控，这里禁止
		ikcp_nodelay(m_kcp, 2, 10, 2, 1);
		m_kcp->rx_minrto = 10;
		m_kcp->fastresend = 1;
		break;

	default:
		assert(false);
		break;
	}

	stopFlag.store(false);
	kcp_loop_thread = std::unique_ptr<std::thread>(new std::thread(&KcpClient::client_loop, this));
	kcp_loop_thread->detach();

	kcp_client_thread = std::unique_ptr<std::thread>(new std::thread(&KcpClient::run_tcp_client, this));
	kcp_client_thread->detach();
}

void KcpClient::send_file() {
	// std::cout <<"start send file" << std::endl;
	assert(s_state == TCP_ESTABLISHED);
	kcp_client_start();

	// 打开文件并发送内容
	std::string filename = file_path.substr(file_path.find_last_of("/") + 1);

	std::ifstream file(file_path, std::ios::in | std::ios::binary);
	if (!file) {
		perror("send file");
		Close();
		return;
	}

	char temp[8 + 128]{};
	// 获取文件大小
	file.seekg(0, std::ios::end);
	uint32_t file_size = file.tellg();
	file.seekg(0, std::ios::beg);
	// std::cout << "file_size: " << file_size << std::endl;
	size_t file_size_n = htonl(file_size);
	memcpy(temp, &file_size_n, sizeof(uint32_t));
	memcpy(temp + sizeof(uint32_t), &filename[0], filename.size());
	ikcp_send(m_kcp, temp, sizeof(temp));

	char buf[1024];
	size_t BUFFER_SIZE = 1024;
	size_t totalBytesRead = 0;
	while (totalBytesRead < file_size) {
		size_t s = std::min(file_size - totalBytesRead, BUFFER_SIZE);
		file.read(buf, s);
		std::streamsize bytesRead = file.gcount();
		totalBytesRead += bytesRead;
		ikcp_send(m_kcp, buf, s);
		// std::cout <<"send: " << s << "\n";
	}
	file.close();

	while (!finishSend.load()) {
		KCP::isleep(10);
	}
	while (ikcp_waitsnd(m_kcp) > 0) {
		KCP::isleep(10);
	}
	KCP::isleep(10);
	
	return;
	auto start = std::chrono::high_resolution_clock::now();
	auto end = start + std::chrono::seconds(3);
	while (true) {
		if (s_state == TCP_CLOSING) {
			return;
		}
		start_waving();
		if (std::chrono::high_resolution_clock::now() >= end) {
			std::cout << "waving timeout break" << std::endl;
			return;
		}
	}
}

void KcpClient::start_hand_shake() {
	// std::cout <<"start hand shake" << std::endl;
	assert(s_state == TCP_CLOSED);
	char data[1024] = {};

	build_ip_tcp_header(data, "", 0, 0, 0, 1, 0);
	s_state = TCP_SYN_SEND;
	struct sockaddr_in dest;
	setAddr(s_ip, s_port, &dest);
	int ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
	// std::cout <<"ret: " << ret << std::endl;
	if (ret < 0) {
        perror("sendto error");
    }

	struct sockaddr_in recv_dest;
	setAddr(s_ip, s_port, &recv_dest);
	socklen_t addrlen = sizeof(sockaddr_in);
	tcp_info info;
	while (true) {
		ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&recv_dest, &addrlen);
		if (ret < 0) {
			perror("recvfrom");
		}
		prase_tcp_packet(data, ret, &info);

		if (info.port_dst == c_port) {
			if (info.fin) {
				std::cout << "server fin start break" << std::endl;
				s_state = TCP_ESTABLISHED;
				start_waving();
				return;
			}
			break;
		}
	}
	if (info.ack_seq != seq + 1) {
		printf("invalid ack_seq: %d\n", info.ack_seq);
		Close();
		return;
	}
	seq++;
	server_ack_seq.store(info.seq + 1);
	build_ip_tcp_header(data, "", 0, 1, 0, 0, 0);
	ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*)&dest, addrlen);
	s_state = TCP_ESTABLISHED;

	send_file();
}

void KcpClient::waving_send_fin() {
	int ret = 0;
	char data[41] = {};
	struct sockaddr_in dest;
	setAddr(s_ip, s_port, &dest);
	
	// std::cout <<"s_state: " << s_state << std::endl;
		// client fin
		// std::cout << "TCP_ESTABLISHED" << std::endl;
	build_ip_tcp_header(data, "", 0, 1, 0, 0, 1);
	ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
	if (ret < 0) {
		perror("sendto");
		return;
	}
	s_state = TCP_FIN_WAIT1;
}

void KcpClient::waving_recv_ack() {
	std::cout << c_port << " TCP_FIN_WAIT1" << std::endl;
	int ret = 0;
	char data[41] = {};
	struct sockaddr_in dest;
	socklen_t addrlen = sizeof(sockaddr_in);
	setAddr(s_ip, s_port, &dest);
	fd_set read_fds;
	struct timeval timeout;
	auto start = std::chrono::high_resolution_clock::now();
	auto end = start + std::chrono::seconds(3);

	while (true) {
		FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
		timeout.tv_sec = 3;
        timeout.tv_usec = 0;
		// server ack
		tcp_info info;
		ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
		if (ret == -1) {
            perror("select");
            return;
        } else if (ret == 0) {
			std::cout << "timeout" << std::endl;
            return;
        }
		if (FD_ISSET(fd, &read_fds)) {
			ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
			if (ret < 0) {
				perror("recvfrom TCP_FIN_WAIT1");
				return;
			}
			prase_tcp_packet(data, ret, &info);
			
			if (info.rst) {
				s_state = TCP_CLOSING;
				return;
			}
			if (info.port_dst == c_port && info.ack_seq == seq + CLIENT_SUM_SEND.load() + 1) {
				assert(info.ack);
				if (info.fin) {
					server_ack_seq.store(info.seq + 1);
					seq++;
					s_state = TCP_CLOSE_WAIT;
				} else {
					s_state = TCP_FIN_WAIT2;
				}
				return;
			}
		}

		if (std::chrono::high_resolution_clock::now() >= end) {
			s_state = TCP_ESTABLISHED;
			return;
		}
	}
}

void KcpClient::waving_recv_fin() {
	std::cout << c_port << " TCP_FIN_WAIT2" << std::endl;
    int ret = 0;
	char data[41] = {};
	struct sockaddr_in dest;
	socklen_t addrlen = sizeof(sockaddr_in);
	setAddr(s_ip, s_port, &dest);
	fd_set read_fds;
	struct timeval timeout;

	auto start = std::chrono::high_resolution_clock::now();
	auto end = start + std::chrono::seconds(3);
	while (true) {
		FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
		timeout.tv_sec = 3;
        timeout.tv_usec = 0;
		// server ack
		tcp_info info;
		ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
		if (ret == -1) {
            perror("select");
            return;
        } else if (ret == 0) {
			std::cout << "timeout" << std::endl;
            return;
        }
		if (FD_ISSET(fd, &read_fds)) {
			ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
			if (ret < 0) {
				perror("recvfrom TCP_FIN_WAIT2");
				return;
			}
			prase_tcp_packet(data, ret, &info);

			if (info.port_dst == c_port) {
				if (info.rst) {
					s_state = TCP_CLOSING;
					return;
				}
				assert(info.fin);
				server_ack_seq.store(info.seq + 1);
				seq++;
				s_state = TCP_CLOSE_WAIT;
				return;
			}
		}

		if (std::chrono::high_resolution_clock::now() >= end) {
			s_state = TCP_ESTABLISHED;
			return;
		}
	}
}

void KcpClient::waving_send_ack() {
	//client ack
	int ret = 0;
	char data[41] = {};
	struct sockaddr_in dest;
	setAddr(s_ip, s_port, &dest);
	build_ip_tcp_header(data, "", 0, 1, 0, 0, 0);
	ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
	
	// std::cout << "TCP_CLOSE_WAIT: " << ret << std::endl;
	if (ret < 0) {
		perror("TCP_CLOSE_WAIT");
		return;
	}
	s_state = TCP_CLOSING;
}

void KcpClient::start_waving() {

	switch(s_state) {
		case TCP_ESTABLISHED:
			waving_send_fin();
			break;
		case TCP_FIN_WAIT1:
			waving_recv_ack();
			break;
		case TCP_FIN_WAIT2:
			waving_recv_fin();
			break;
		case TCP_CLOSE_WAIT:
			waving_send_ack();
			break;
		default:
			break;
	}
	return;


	int ret = 0;
	char data[41] = {};
	struct sockaddr_in dest;
	socklen_t addrlen = sizeof(sockaddr_in);
	setAddr(s_ip, s_port, &dest);
	
	// std::cout <<"s_state: " << s_state << std::endl;
	if (s_state == TCP_ESTABLISHED) { 
		// client fin
		// std::cout << "TCP_ESTABLISHED" << std::endl;
		build_ip_tcp_header(data, "", 0, 1, 0, 0, 1);
		ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
		if (ret < 0) {
			perror("sendto");
			return;
		}
		s_state = TCP_FIN_WAIT1;
	}
	
	fd_set read_fds;
	struct timeval timeout;

	auto start = std::chrono::high_resolution_clock::now();
	auto end = start + std::chrono::seconds(3);
	while (true) {
		FD_ZERO(&read_fds);
        FD_SET(fd, &read_fds);
		timeout.tv_sec = 3;
        timeout.tv_usec = 0;
		// server ack
		tcp_info info;
		ret = select(fd + 1, &read_fds, NULL, NULL, &timeout);
		if (ret == -1) {
            perror("select");
            return;
        } else if (ret == 0) {
			std::cout << "timeout" << std::endl;
            return;
        }

		if (s_state == TCP_FIN_WAIT1) {
			if (FD_ISSET(fd, &read_fds)) {
				ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
				if (ret < 0) {
					perror("recvfrom TCP_FIN_WAIT1");
					return;
				}
				prase_tcp_packet(data, ret, &info);
				
				if (info.rst) {
					s_state = TCP_CLOSING;
					return;
				}
				if (info.port_dst == c_port && info.ack_seq == seq + CLIENT_SUM_SEND.load() + 1) {
					assert(info.ack);
					if (info.fin) {
						server_ack_seq.store(info.seq + 1);
						seq++;
						s_state = TCP_CLOSE_WAIT;
					} else {
						s_state = TCP_FIN_WAIT2;
						continue;
					}
					
				}
			}
			// while (true) {
			// 	ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
			// 	if (ret < 0) {
			// 		perror("recvfrom TCP_FIN_WAIT1");
			// 		return;
			// 	}
			// 	prase_tcp_packet(data, ret, &info);
			// 	if (info.port_dst == c_port && info.ack_seq == seq + CLIENT_SUM_SEND.load() + 1) {
			// 		break;
			// 	}
			// }
			// assert(info.ack);
			// if (info.fin) {
			// 	server_ack_seq.store(info.seq + 1);
			// 	seq++;
			// 	s_state = TCP_CLOSE_WAIT;
			// } else {
			// 	s_state = TCP_FIN_WAIT2;
			// }
			// continue;
		}

		if (s_state == TCP_FIN_WAIT2) {
			//server fin
			
			if (FD_ISSET(fd, &read_fds)) {
				std::cout << "TCP_FIN_WAIT2" << std::endl;
				ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
				if (ret < 0) {
					perror("recvfrom");
					return;
				}
				prase_tcp_packet(data, ret, &info);

				if (info.port_dst == c_port) {
					std::cout << info.rst << " " << ntohl(info.rst) << std::endl;
					if (info.rst) {
						s_state = TCP_CLOSING;
						return;
					}
					assert(info.fin);
					server_ack_seq.store(info.seq + 1);
					seq++;
					s_state = TCP_CLOSE_WAIT;
				}
			}

			// std::cout << fd << " TCP_FIN_WAIT2" << std::endl;
			// while (true) {
			// 	ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
			// 	if (ret < 0) {
			// 		perror("recvfrom");
			// 		return;
			// 	}
			// 	prase_tcp_packet(data, ret, &info);
			// 	if (info.rst) {
			// 		s_state = TCP_CLOSING;
			// 		return;
			// 	}

			// 	if (info.port_dst == c_port) {
			// 		std::cout << info.rst << " " << ntohl(info.rst) << std::endl;
			// 		if (info.rst) {
			// 			s_state = TCP_CLOSING;
			// 			return;
			// 		}
			// 		if (info.fin) {
			// 			server_ack_seq.store(info.seq + 1);
			// 			seq++;
			// 			break;
			// 		}
			// 	}
			// }
		}

		if (s_state == TCP_CLOSE_WAIT) {
			//client ack
			build_ip_tcp_header(data, "", 0, 1, 0, 0, 0);
			ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
			
			// std::cout << "TCP_CLOSE_WAIT: " << ret << std::endl;
			if (ret < 0) {
				perror("TCP_CLOSE_WAIT");
				return;
			}
			s_state = TCP_CLOSING;
			return;
		}
		if (std::chrono::high_resolution_clock::now() > end) {
			std::cout << "timeout" << std::endl;
			s_state = TCP_CLOSING;
			return;
		}
	}
}

void KcpClient::Close() {
	if (!m_kcp) {
		return;
	}
	//std::this_thread::sleep_for(std::chrono::seconds(1));
	ikcp_release(m_kcp);
	m_kcp = nullptr;
	close(fd);
}

}