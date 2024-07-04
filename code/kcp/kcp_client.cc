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
		tcp_header.seq = htonl(client->seq + client->CLIENT_SUM_SEND); // 序列号
		tcp_header.ack_seq = htonl(client->server_ack_seq);	   // 确认号
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
		// std::cout << "ret client send: " << ret << std::endl;
		if (ret < 0)
		{
			perror("sendto");
			break;
		}
		client->CLIENT_SUM_SEND += data_len;

		// ssize_t ret = send(def->fd, buffer + sended, s, 0);
		// if(ret < 0){
		//     return -1;
		// }
		sended += s;
	}

	return (size_t)sended;
}

KcpClient::KcpClient(int fd, uint16_t c_port, uint16_t s_port, const char* c_ip, const char* s_ip, std::string file_path)
 	:fd(fd), c_port(c_port), s_port(s_port), c_ip(c_ip), s_ip(s_ip), file_path(file_path){
	s_state = tcp_closed;
	server_ack_seq = 0;
	CLIENT_SUM_SEND = 0;
	m_kcp = ikcp_create(0x1, this);
	assert(m_kcp);
}

KcpClient::~KcpClient() {
	ikcp_release(m_kcp);
	Close();
}


void KcpClient::startClient() {
	char buf[65536] = {0};
	struct sockaddr_in server_addr;
	setAddr(s_ip, s_port, &server_addr);

	struct in_addr client_in_addr, server_in_addr;
	inet_aton(c_ip, &client_in_addr);
	inet_aton(c_ip, &server_in_addr);

	uint8_t syn = 0, ack = 0;
	int ack_seq = 0;
	struct pkt_info info;

	while (true) {
		char *data = ReserveHdrSize(buf);
		ssize_t _s = 0;

		printf("s_state: %d  ----------------------\n", s_state);

		if (s_state == tcp_closed) {
			seq = 12345;
			syn = 1;
			s_state = tcp_syn_sent;
		} else if (s_state == tcp_syn_sent) {
			if (info.ack_seq != seq + 1) {
				printf("invalid ack_seq: %d\n", info.ack_seq);
				break;
			}

			seq++;
			syn = 0;
			ack_seq = info.seq + 1;
			ack = 1;
			s_state = tcp_established;
		} else {
			ack_seq = info.seq + info.data_len;
			seq += _s;
		}

		int total = BuildPkt(data, _s, &client_in_addr, c_port, &server_in_addr, s_port, seq, ack_seq, syn, ack);

		_s = sendto(fd, buf, total, 0, (struct sockaddr *)&server_addr, sizeof(server_addr));
		if (_s < 0) {
			perror("sendto");
			break;
		}

		if (s_state == tcp_established) {
			std::cout << "tcp_established" << std::endl;
			server_ack_seq = info.seq + 1;
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

			file_size = htonl(file_size);
			memcpy(temp, &file_size, sizeof(uint32_t));
			memcpy(temp + sizeof(uint32_t), &filename[0], filename.size());
			ikcp_send(m_kcp, temp, sizeof(temp));

			char buf[1024];
			size_t BUFFER_SIZE = 1024;
			while (!file.eof()) {
				file.read(buf, BUFFER_SIZE);
				ikcp_send(m_kcp, buf, BUFFER_SIZE);
			}

			
			file.close();

			std::cout << "send file finished" << std::endl;
			std::string msg = "send file finished";
			
			std::this_thread::sleep_for(std::chrono::seconds(2));
			Close();
			return;
			// ikcp_send(m_kcp, msg.c_str(), msg.size());
		}

		memset(buf, 0, sizeof(buf));
		struct sockaddr_in from_addr;
		setAddr(s_ip, s_port, &from_addr);
		socklen_t from_size;
		from_size = sizeof(struct sockaddr_in);

		if (s_state != tcp_established)
		{
			ssize_t sz = recv_from_addr(fd, buf, sizeof(buf), 0, &from_addr, &from_size, &info);
			if (sz <= 0)
			{
				break;
			}
		}
	}
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
			// std::cout << info.port_src << " " << info.port_dst << " " <<  info.seq << " " << info.ack_seq << std::endl;
			server_ack_seq = info.seq + len - (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr));
			// std::cout << "client recv len: " << len << std::endl;
			// ikcp_send(m_kcp, "", 0);
			if (len > (ssize_t)(sizeof(struct iphdr) + sizeof(struct tcphdr)))
			{
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
		}
		KCP::isleep(1);
	}
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

void KcpClient::Close() {
	stopFlag.store(true);
	std::this_thread::sleep_for(std::chrono::seconds(2));
}

}