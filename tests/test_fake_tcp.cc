#include "code/fake_tcp.h"
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
#include <thread>

static const char *s_ip = "127.0.0.1";
static short s_port = 6666;
static const char *c_ip = "127.0.0.1";
static short c_port = 6668;
static int ip_id = 0;


struct tcp_def {
    int32_t fd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
};

typedef struct __cb_params__ {
	int fd;
	ikcpcb* m_kcp;
} cb_params;

#define UDP_MTU 1400

int tcp_server_send_cb(const char *buffer, int len, ikcpcb *kcp, void *user) {
	tcp_def* def =  (tcp_def*)user;
	std::cout << "sending: " << len << std::endl;
    int sended = 0;
    while (sended < len) {
        size_t s = (len - sended);
        if(s > UDP_MTU) s = UDP_MTU;
        ssize_t ret = send(def->fd, buffer + sended, s, 0);
        if(ret < 0){
            return -1;
        }
        sended += s;
    }

    return (size_t)sended;
}

void* run_tcp_server(void* args) {

	std::cout << "start tcp_server thread: " << std::endl;

	cb_params* param = (cb_params*) args;

	int fd = param->fd;
	ikcpcb* m_kcp = param->m_kcp;
	assert(fd && m_kcp);
	// struct sockaddr_in from_addr;
	// socklen_t from_size = sizeof(sockaddr_in);
	// SetAddr(s_ip, s_port, &from_addr);
	
	ssize_t len = 0;
	char buffer[2048] = { 0 };
	socklen_t src_len = sizeof(struct sockaddr_in);

	while (true) {
		ikcp_update(m_kcp, KCP::iclock());
		struct sockaddr_in src;
		memset(&src, 0, src_len);
		len = read(fd, buffer, sizeof(buffer));
		if (len > 0) {
			int ret = ikcp_input(m_kcp, buffer, len);
			if (ret < 0) {
				printf("ikcp_input error: %d\n", ret);
				continue;
			}
			char recv_buffer[2048] = { 0 };
			ret = ikcp_recv(m_kcp, recv_buffer, len);
			if(ret >= 0) {
				printf("ikcp_recv ret = %d,buf=%s\n",ret, recv_buffer);
			}
		} else {
			perror("ikcp_recv");
		}
		KCP::isleep(100);
	}

	std::cout << "server: close --------------------------" << std::endl;
	close(fd);
	delete param;
	pthread_exit(NULL);
}

void start_kcp_server(int fd, ikcpcb* m_kcp) {
	//char buf[65535];
	assert(m_kcp && fd);
	ikcp_setoutput(m_kcp, tcp_server_send_cb);

	int sndwnd = 128, rcvwnd = 128;
    ikcp_wndsize(m_kcp, sndwnd, rcvwnd);
	int mod = 0;
    switch (mod) {
        case 0:
            ikcp_nodelay(m_kcp, 0, 40, 0, 0);
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

	cb_params* param = new cb_params;
	param->fd = fd;
	param->m_kcp = m_kcp;
    pthread_t tid;
	if (pthread_create(&tid, NULL, run_tcp_server, param) != 0) {
        perror("pthread_create failed");
        return;
    }
}

void ServerRun(int fd, ikcpcb* m_kcp)
{
	start_kcp_server(fd, m_kcp);
	//发送数据
	// std::string msg;
	// std::cout << "send: ";
	// msg = "hello world";
    // while (true) {
    //     int ret = ikcp_send(m_kcp, msg.c_str(), msg.size());
    //     std::cout << "data_size: " << ret << std::endl;
	// 	KCP::isleep(1000);
    // }
}

enum conn_state {
	tcp_closed = 0,
	tcp_syn_sent = 1,
	tcp_established = 2,
};

static int s_state = tcp_closed;


// IP 校验和计算函数
uint16_t calculate_ip_checksum(const struct iphdr* iphdr_packet) {
    uint32_t sum = 0;
    int length = iphdr_packet->ihl * 4; 
    const uint16_t* buf = reinterpret_cast<const uint16_t*>(iphdr_packet);

    for (int i = 0; i < length / 2; ++i) {
        sum += ntohs(buf[i]);
    }

    if (length % 2) {
        sum += static_cast<uint16_t>(*(reinterpret_cast<const uint8_t*>(iphdr_packet) + length - 1));
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    return static_cast<uint16_t>(~sum);
}


struct pseudo_header {
    uint32_t source_address;
    uint32_t dest_address;
    uint8_t placeholder;
    uint8_t protocol;
    uint16_t tcp_length;
};

uint16_t calculate_tcp_checksum(const struct iphdr* iphdr_packet, const struct tcphdr* tcphdr_packet, const char* data, size_t data_len) {
    uint32_t sum = 0;

    pseudo_header pseudo_hdr;
    pseudo_hdr.source_address = iphdr_packet->saddr;
    pseudo_hdr.dest_address = iphdr_packet->daddr;
    pseudo_hdr.placeholder = 0;
    pseudo_hdr.protocol = IPPROTO_TCP;
    pseudo_hdr.tcp_length = htons(sizeof(struct tcphdr) + data_len);


    const uint16_t* pseudo_header_words = reinterpret_cast<const uint16_t*>(&pseudo_hdr);
    int pseudo_length = sizeof(pseudo_hdr);
    for (int i = 0; i < pseudo_length / 2; ++i) {
        sum += ntohs(pseudo_header_words[i]);
    }

    const uint16_t* tcp_packet = reinterpret_cast<const uint16_t*>(tcphdr_packet);
    int tcp_length = sizeof(struct tcphdr);
    for (int i = 0; i < tcp_length / 2; ++i) {
        sum += ntohs(tcp_packet[i]);
    }
	
    char* new_data = new char[data_len + 1];
    std::memcpy(new_data, data, data_len);
	if (data_len & 1) {
		new_data[data_len] = 0;
		data_len++;
	}

    const uint16_t* data_packet = reinterpret_cast<const uint16_t*>(new_data);
    int data_length = data_len;
    for (int i = 0; i < data_length / 2; ++i) {
        sum += ntohs(data_packet[i]);
    }

    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }

    delete[] new_data;
    return static_cast<uint16_t>(~sum);
}


uint32_t seq = 0, ack_seq = 0;

int tcp_client_cb(const char *buffer, int len, ikcpcb *kcp, void *user) {
	tcp_def* def =  (tcp_def*)user;


    int sended = 0;
    while (sended < len) {
        size_t s = (len - sended);
        if(s > UDP_MTU) s = UDP_MTU;

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
		ip_header.id = htons(++ip_id);
		ip_header.frag_off = 0;
		ip_header.ttl = 255;
		ip_header.protocol = IPPROTO_TCP;
		ip_header.check = 0; 

		ip_header.saddr = inet_addr(inet_ntoa(def->local_addr.sin_addr));
		ip_header.daddr = inet_addr(inet_ntoa(def->remote_addr.sin_addr));


		struct tcphdr tcp_header;
		std::memset(&tcp_header, 0, sizeof(tcp_header));
		tcp_header.source = htons(c_port);  // 源端口号
		tcp_header.dest = htons(s_port);    // 目标端口号
		tcp_header.seq = htonl(seq + data_len);          // 序列号
		tcp_header.ack_seq = htonl(1);      // 确认号
		tcp_header.doff = 5;                // TCP头部长度
		tcp_header.fin = 0;                 
		tcp_header.syn = 0;                 // SYN标志位
		tcp_header.rst = 0;                 // RST标志位
		tcp_header.psh = 0;                 // PSH标志位
		tcp_header.ack = 1;                 // ACK标志位
		tcp_header.urg = 0;                 // URG标志位
		tcp_header.window = htons(8192);    // 窗口大小  
		tcp_header.urg_ptr = 0;             // 紧急指针  
		tcp_header.check = 0;

		// ip_hdr.ip_sum = checksum(&ip_header, sizeof(ip_header));
		ip_header.check = calculate_ip_checksum(&ip_header);
		tcp_header.check = htons(calculate_tcp_checksum(&ip_header, &tcp_header, buffer, data_len));

		std::cout << "tcp_check: " << htons(tcp_header.check) << std::endl;
		// 计算 TCP 校验和
		// 构造数据包

		char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len];
		std::memcpy(packet, &ip_header, sizeof(struct iphdr));
		std::memcpy(packet + sizeof(struct iphdr), &tcp_header, sizeof(struct tcphdr));
		std::memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), buffer + sended, data_len);

		struct sockaddr_in dest;
		SetAddr(inet_ntoa(def->remote_addr.sin_addr), def->remote_addr.sin_port, &dest);				

		ssize_t ret = sendto(def->fd, packet, sizeof(packet), 0, (struct sockaddr*)&dest, sizeof(struct sockaddr));
		std::cout << "ret: " << ret << std::endl;
		if (ret < 0) {
			perror("sendto");
			break;
		}

        // ssize_t ret = send(def->fd, buffer + sended, s, 0);
        // if(ret < 0){
        //     return -1;
        // }
        sended += s;
    }

    return (size_t)sended;
}

void run_tcp_client(int fd, ikcpcb* m_kcp) {

	while (true)
	{
		struct sockaddr_in from_addr;
		socklen_t from_size = sizeof(sockaddr_in);
		SetAddr(s_ip, s_port, &from_addr);
		
		ssize_t len = 0;
		char buffer[2048] = { 0 };
		socklen_t src_len = sizeof(struct sockaddr_in);

		while (true) {
			ikcp_update(m_kcp, KCP::iclock());
			struct sockaddr_in src;
			memset(&src, 0, src_len);
			len = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &from_size);
			KCP::tcp_info info;
			KCP::prase_tcp_packet(buffer, len, &info);
			if (info.port_dst != s_port) {
				continue;
			}

			if (len > 0) {
				int ret = ikcp_input(m_kcp, buffer, len);
				if (ret < 0) {
					printf("ikcp_input error: %d\n", ret);
					continue;
				}

				char recv_buffer[2048] = { 0 };
				ret = ikcp_recv(m_kcp, recv_buffer, len);
				if(ret >= 0) {
					printf("ikcp_recv ret = %d,buf=%s\n",ret, recv_buffer);
				}
			}
			KCP::isleep(1);
    	}
	}
	close(fd);
}

void kcp_client_start(int fd, ikcpcb* m_kcp) {
		//char buf[65535];
	ikcp_setoutput(m_kcp, tcp_client_cb);

	int sndwnd = 128, rcvwnd = 128;
	ikcp_wndsize(m_kcp, sndwnd, rcvwnd);
	int mod = 0;
	switch (mod) {
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

	std::thread server_thread(run_tcp_client, fd, m_kcp);
	server_thread.detach();
}

void ClientRun(int fd, ikcpcb* m_kcp)
{
	char buf[65536] = {0};
	struct sockaddr_in server_addr;
	SetAddr(s_ip, s_port, &server_addr);

	struct in_addr client_in_addr, server_in_addr;
	inet_aton(c_ip, &client_in_addr);
	inet_aton(c_ip, &server_in_addr);

	uint8_t syn = 0, ack = 0;
	struct pkt_info info;

	while (1)
	{
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
			kcp_client_start(fd, m_kcp);
			return;
			std::string msg;
			int sum = 0;
			while (std::cin >> msg) {
				ssize_t data_len = msg.size();

    			// 构造 IP 头部
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
				tcp_header.source = htons(c_port);  // 源端口号
				tcp_header.dest = htons(s_port);    // 目标端口号
				tcp_header.seq = htonl(seq + sum);          // 序列号
				tcp_header.ack_seq = htonl(info.seq + 1);      // 确认号
				tcp_header.doff = 5;                // TCP头部长度
				tcp_header.fin = 0;                 
				tcp_header.syn = 0;                 // SYN标志位
				tcp_header.rst = 0;                 // RST标志位
				tcp_header.psh = 1;                 // PSH标志位
				tcp_header.ack = 1;                 // ACK标志位
				tcp_header.urg = 0;                 // URG标志位
				tcp_header.window = htons(8192);    // 窗口大小  
				tcp_header.urg_ptr = 0;             // 紧急指针  
				tcp_header.check = 0;

				// ip_hdr.ip_sum = checksum(&ip_header, sizeof(ip_header));
				ip_header.check = calculate_ip_checksum(&ip_header);
				tcp_header.check = htons(calculate_tcp_checksum(&ip_header, &tcp_header, msg.c_str(), data_len));
				std::cout << "tcp_check: " << htons(tcp_header.check) << std::endl;
    			// 计算 TCP 校验和
				// 构造数据包

				char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len];
				std::memcpy(packet, &ip_header, sizeof(struct iphdr));
				std::memcpy(packet + sizeof(struct iphdr), &tcp_header, sizeof(struct tcphdr));
				std::memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), msg.c_str(), data_len);

				struct sockaddr_in dest;
				SetAddr(s_ip, s_port, &dest);				

				int ret = sendto(fd, packet, sizeof(packet), 0, (struct sockaddr*)&dest, sizeof(struct sockaddr));
				std::cout << "ret: " << ret << std::endl;
				if (ret < 0) {
					perror("sendto");
                    break;
				}
				sum += data_len;
			}
			// 关闭套接字
			close(fd);
		}

		memset(buf, 0, sizeof(buf));
		struct sockaddr_in from_addr;
		SetAddr(s_ip, s_port, &from_addr);
		socklen_t from_size;
		from_size = sizeof(struct sockaddr_in);

		
		if (s_state != tcp_established) {
			ssize_t sz = recv_from_addr(fd, buf, sizeof(buf), 0, &from_addr, &from_size, &info);
			if (sz <= 0) {
				break;
			}
		}


		
	}
}

int main(int argc, char *argv[])
{
	int server = 0;
	if (argc >= 2 && strncmp(argv[1], "-s", 2) == 0)
	{
		server = 1;
	}

	printf("run in %s mode...\n", server ? "server" : "client");

	int fd = server ? StartServer(s_ip, s_port) : StartFakeTcp(c_ip, c_port);
	ikcpcb* ikcpcb;
	std::cout << "fd: " << fd << std::endl;
	if (fd < 0)
	{
		return -1;
	}
	if (server)
	{
		tcp_def* def = new tcp_def; 
		def->fd = fd;
		SetAddr(s_ip, s_port, &def->local_addr);
		SetAddr(c_ip, c_port, &def->remote_addr);
		ikcpcb = ikcp_create(0x1, (void*)def);
		ServerRun(fd, ikcpcb);
	}
	else
	{
		//testClient(fd);
		tcp_def* def = new tcp_def; 
		def->fd = fd;
		SetAddr(c_ip, c_port, &def->local_addr);
		SetAddr(s_ip, s_port, &def->remote_addr);
		ikcpcb = ikcp_create(0x1, (void*)def);
		ClientRun(fd, ikcpcb);
	}

	std::cout << "looping" << std::endl;
	while (true) {
		sleep(1);
	}
	std::cout << "stop main: " << std::endl;
	Stop(fd);
	free(ikcpcb);
	return 0;
}