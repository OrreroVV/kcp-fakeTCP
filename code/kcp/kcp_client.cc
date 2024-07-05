#include "kcp_client.h"
#include <iostream>

namespace KCP {

int tcp_client_cb(const char *buffer, int len, ikcpcb *kcp, void *user)
{
	KcpClient *client = static_cast<KcpClient*>(user);

	// std::cout << "server_ack_seq: " << server_ack_seq << std::endl;

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

		// // std::cout << "tcp_check: " << htons(tcp_header.check) << std::endl;
		// 计算 TCP 校验和
		// 构造数据包

		char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len];
		std::memcpy(packet, &ip_header, sizeof(struct iphdr));
		std::memcpy(packet + sizeof(struct iphdr), &tcp_header, sizeof(struct tcphdr));
		std::memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), buffer + sended, data_len);

		struct sockaddr_in dest;
		setAddr(client->s_ip, client->s_port, &dest);

		ssize_t ret = sendto(client->fd, packet, sizeof(packet), 0, (struct sockaddr *)&dest, sizeof(sockaddr));
		std::cout << "ret client send: " << ret << std::endl;
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

	server_ack_seq.store(0);
	CLIENT_SUM_SEND.store(0);
	std::srand(static_cast<unsigned>(std::time(0)));
	// seq.store(rand());
	seq = rand();

	m_kcp = ikcp_create(0x1, this);
	assert(m_kcp);
}

KcpClient::~KcpClient() {
	Close();
}


void KcpClient::build_ip_tcp_header(char* data, const char* buffer, size_t data_len, int ack, int psh, int syn, int fin) {
	
	struct iphdr ip_header;
	std::memset(&ip_header, 0, sizeof(ip_header));
	ip_header.ihl = 5;
	ip_header.version = IPVERSION;
	ip_header.tos = 0;
	ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
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

	CLIENT_SUM_SEND.fetch_add(data_len);
	memcpy(data, &ip_header, IPV4_HEADER_SIZE);
	memcpy(data + IPV4_HEADER_SIZE, &tcp_header, TCP_HEADER_SIZE);
	memcpy(data + IP_TCP_HEADER_SIZE, buffer, data_len);
}

void KcpClient::run_tcp_client() {
	std::cout << "start run_tcp_client" << std::endl;
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
			// std::cout << "client recv len: " << len << std::endl;
			// ikcp_send(m_kcp, "", 0);
			

			if (len > (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr)))
			{
				
				// 如果不是ack包，发送确认收到
				char send_buffer[41] = {};
				build_ip_tcp_header(send_buffer, "", 0, 1, 1, 0, 0);
				::sendto(fd, send_buffer, IP_TCP_HEADER_SIZE, 0, (sockaddr*)&src, src_len);


				int ret = ikcp_input(m_kcp, buffer + sizeof(struct iphdr) + sizeof(struct tcphdr), len - (sizeof(struct iphdr) + sizeof(struct tcphdr)));
				if (ret < 0)
				{
					printf("ikcp_input error: %d\n", ret);
					continue;
				}

				char recv_buffer[2048] = {0};


				ret = ikcp_recv(m_kcp, recv_buffer, len);
				if (ret > 0)
				{
					printf("ikcp_recv ret = %d,buf=%s\n", ret, recv_buffer);
				}
			}
		} else if (!len) {
			std::cout << "start recv 0" << std::endl;
			KCP::tcp_info info;
			KCP::prase_tcp_packet(buffer, len, &info);
			if (info.port_dst != c_port) {
				continue;
			}
			if (info.fin) {
				s_state = TCP_FIN_WAIT1;
				break;
			}
		}
		KCP::isleep(1);
	}

	Close();
}

void* KcpClient::client_loop()
{
	std::cout << "start client loop" << std::endl;
	while (!stopFlag.load())
	{
		ikcp_update(m_kcp, KCP::iclock());
		KCP::isleep(1);
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
	assert(s_state == TCP_ESTABLISHED);
	kcp_client_start();

	// 打开文件并发送内容
	std::string filename = file_path.substr(file_path.find_last_of("/") + 1);

	std::ifstream file(file_path, std::ios::in | std::ios::binary);
	if (!file) {
		exit(1);
	}

	char temp[8 + 128]{};
	// 获取文件大小
	file.seekg(0, std::ios::end);
	uint32_t file_size = file.tellg();
	file.seekg(0, std::ios::beg);

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
		std::cout << "send: " << s << std::endl;
	}

	
	file.close();
	std::cout << "send file finished" << std::endl;
	while (ikcp_waitsnd(m_kcp) > 0) {
		KCP::isleep(100);
		std::cout << "waitsnd: " << ikcp_waitsnd(m_kcp) << std::endl;
	}
	std::this_thread::sleep_for(std::chrono::seconds(1));
	Close();
}

void KcpClient::start_hand_shake() {
	assert(s_state == TCP_CLOSED);
	char data[41] = {};

	build_ip_tcp_header(data, "", 0, 0, 0, 1, 0);
	s_state = TCP_SYN_SEND;
	struct sockaddr_in dest;
	setAddr(s_ip, s_port, &dest);
	int ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
	if (ret < 0) {
        exit(1);
    }

	struct sockaddr_in recv_dest;
	setAddr(s_ip, s_port, &recv_dest);
	socklen_t addrlen = sizeof(sockaddr_in);
	tcp_info info;
	while (true) {
		ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&recv_dest, &addrlen);
		if (ret < 0) {
			exit(1);
		}
		prase_tcp_packet(data, ret, &info);

		if (info.port_dst == c_port) {
			break;
		}
	}
	if (info.ack_seq != seq + 1) {
		printf("invalid ack_seq: %d\n", info.ack_seq);
	}
	seq++;
	server_ack_seq.store(info.seq + 1);
	build_ip_tcp_header(data, "", 0, 1, 0, 0, 0);
	ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*)&dest, addrlen);
	s_state = TCP_ESTABLISHED;

	send_file();
}

void KcpClient::start_waving() {
	std::cout << "start_waving" << std::endl;
	int ret = 0;
	char data[41] = {};
	struct sockaddr_in dest;
	socklen_t addrlen = sizeof(sockaddr_in);
	setAddr(s_ip, s_port, &dest);
	
	std::cout << "s_state: " << s_state << std::endl;
	if (s_state == TCP_ESTABLISHED) { 
		// client fin
		build_ip_tcp_header(data, "", 0, 1, 0, 0, 1);
		ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
		if (ret < 0) {
			exit(1);
		}
		s_state = TCP_FIN_WAIT1;
	}
	// server ack
	tcp_info info;
	if (s_state == TCP_FIN_WAIT1) {
		std::cout << "TCP_FIN_WAIT1" << std::endl;
		while (true) {
			ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
			if (ret < 0) {
				exit(1);
			}
			prase_tcp_packet(data, ret, &info);
			if (info.port_dst == c_port && info.ack_seq == seq + CLIENT_SUM_SEND.load() + 1) {
				break;
			}
		}
		assert(info.ack);
		s_state = TCP_FIN_WAIT2;
	}

	if (s_state == TCP_FIN_WAIT2) {
		//server fin
		
		std::cout << "TCP_FIN_WAIT2" << std::endl;
		while (true) {
			ret = recvfrom(fd, data, sizeof(data), 0, (sockaddr*)&dest, &addrlen);
			if (ret < 0) {
				exit(1);
			}
			prase_tcp_packet(data, ret, &info);
			if (info.port_dst == c_port && info.fin) {
				server_ack_seq.store(info.seq + 1);
				seq++;
				break;
			}
		}
		assert(info.fin);
		s_state = TCP_CLOSE_WAIT;
	}

	if (s_state == TCP_CLOSE_WAIT) {
		// client ack
		build_ip_tcp_header(data, "", 0, 1, 0, 0, 0);
		ret = sendto(fd, data, IP_TCP_HEADER_SIZE, 0, (sockaddr*) &dest, sizeof(sockaddr));
		if (ret < 0) {
			exit(1);
		}
		s_state = TCP_CLOSING;
	}
}

void KcpClient::Close() {
	if (stopFlag.load()) {
		return;
	}
	stopFlag.store(true);
	std::this_thread::sleep_for(std::chrono::seconds(2));
	ikcp_release(m_kcp);
	start_waving();
}

}