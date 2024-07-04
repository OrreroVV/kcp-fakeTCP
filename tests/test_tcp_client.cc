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
#include <thread>

#define PORT 6666
#define BUFFER_SIZE 1024
#define UDP_MTU 1400
const char* s_ip = "127.0.0.1";
const int s_port = 6666;
const char* c_ip = "127.0.0.1";
const int c_port = 6668;


unsigned short checksum(void *b, int len) {
    unsigned short *buf = (unsigned short *)b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2) {
        sum += *buf++;
    }

    if (len == 1) {
        sum += *(unsigned char *)buf;
    }

    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;

    return result;
}

// 伪首部用于 TCP 校验和计算
struct pseudo_header {
    uint32_t source_address;
    uint32_t dest_address;
    uint8_t placeholder;
    uint8_t protocol;
    uint16_t tcp_length;
};

typedef struct __cb_params__ {
	int fd;
	ikcpcb* m_kcp;
} cb_params;


uint16_t calculate_tcp_checksum(const struct iphdr* ip_header, const struct tcphdr* tcp_header, const char* data, size_t data_len) {
    struct pseudo_header psh;
    memset(&psh, 0, sizeof(struct pseudo_header));
    psh.source_address = ip_header->saddr;
    psh.dest_address = ip_header->daddr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr) + data_len);

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr) + data_len;
    char* pseudogram = new char[psize];

    memcpy(pseudogram, (char*) &psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcp_header, sizeof(struct tcphdr));
    memcpy(pseudogram + sizeof(struct pseudo_header) + sizeof(struct tcphdr), data, data_len);

    // 计算校验和
    uint16_t checksum = 0;
    uint16_t* ptr = (uint16_t*) pseudogram;

    for (int i = 0; i < psize / 2; i++) {
        checksum += ntohs(ptr[i]);
    }

    checksum = (checksum >> 16) + (checksum & 0xFFFF);
    checksum += (checksum >> 16);

    delete[] pseudogram;

    return (uint16_t) (~checksum);
}

struct tcp_def {
    int32_t fd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
};

int tcp_client_cb(const char *buffer, int len, ikcpcb *kcp, void *user) {
	tcp_def* def =  (tcp_def*)user;
    // std::cout << "send_cb fd: "<< def->fd << std::endl;

    int sended = 0;
    while (sended < len) {
        size_t s = (len - sended);
        if(s > UDP_MTU) s = UDP_MTU;

		int data_len = s;

        struct sockaddr_in dest;
		SetAddr(inet_ntoa(def->remote_addr.sin_addr), def->remote_addr.sin_port, &dest);				

		ssize_t ret = sendto(def->fd, buffer + sended, data_len, 0, (struct sockaddr*)&dest, sizeof(struct sockaddr));
		//ssize_t ret = send(def->fd, buffer + sended, data_len, 0);
		// std::cout << "send ret: " << ret << std::endl;
		if (ret < 0) {
			perror("sendto");
			break;
		}
		/*
		模拟TCP
		*/
	
		// // 构造 IP 头部
		// struct iphdr ip_header;
		// std::memset(&ip_header, 0, sizeof(ip_header));
		// ip_header.ihl = 5;
		// ip_header.version = IPVERSION;
		// ip_header.tos = 0;
		// ip_header.tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len;
		// ip_header.id = htons(++ip_id);
		// ip_header.frag_off = 0;
		// ip_header.ttl = 255;
		// ip_header.protocol = IPPROTO_TCP;
		// ip_header.check = 0; 

		// ip_header.saddr = inet_addr(inet_ntoa(def->local_addr.sin_addr));
		// ip_header.daddr = inet_addr(inet_ntoa(def->remote_addr.sin_addr));


		// struct tcphdr tcp_header;
		// std::memset(&tcp_header, 0, sizeof(tcp_header));
		// tcp_header.source = htons(c_port);  // 源端口号
		// tcp_header.dest = htons(s_port);    // 目标端口号
		// tcp_header.seq = htonl(seq + data_len);          // 序列号
		// tcp_header.ack_seq = htonl(1);      // 确认号
		// tcp_header.doff = 5;                // TCP头部长度
		// tcp_header.fin = 0;                 
		// tcp_header.syn = 0;                 // SYN标志位
		// tcp_header.rst = 0;                 // RST标志位
		// tcp_header.psh = 0;                 // PSH标志位
		// tcp_header.ack = 1;                 // ACK标志位
		// tcp_header.urg = 0;                 // URG标志位
		// tcp_header.window = htons(8192);    // 窗口大小  
		// tcp_header.urg_ptr = 0;             // 紧急指针  
		// tcp_header.check = 0;

		// // ip_hdr.ip_sum = checksum(&ip_header, sizeof(ip_header));
		// ip_header.check = calculate_ip_checksum(&ip_header);
		// tcp_header.check = htons(calculate_tcp_checksum(&ip_header, &tcp_header, buffer, data_len));

		// std::cout << "tcp_check: " << htons(tcp_header.check) << std::endl;
		// // 计算 TCP 校验和
		// // 构造数据包

		// char packet[sizeof(struct iphdr) + sizeof(struct tcphdr) + data_len];
		// std::memcpy(packet, &ip_header, sizeof(struct iphdr));
		// std::memcpy(packet + sizeof(struct iphdr), &tcp_header, sizeof(struct tcphdr));
		// std::memcpy(packet + sizeof(struct iphdr) + sizeof(struct tcphdr), buffer + sended, data_len);

		// struct sockaddr_in dest;
		// SetAddr(inet_ntoa(def->remote_addr.sin_addr), def->remote_addr.sin_port, &dest);				

		// ssize_t ret = sendto(def->fd, packet, sizeof(packet), 0, (struct sockaddr*)&dest, sizeof(struct sockaddr));
		// std::cout << "ret: " << ret << std::endl;
		// if (ret < 0) {
		// 	perror("sendto");
		// 	break;
		// }

        // ssize_t ret = send(def->fd, buffer + sended, s, 0);
        // if(ret < 0){
        //     return -1;
        // }
        sended += s;
    }

    return (size_t)sended;
}

void* run_tcp_client(void * args) {
    cb_params * param = (cb_params*)args;
    int fd = param->fd;
    ikcpcb* m_kcp = param->m_kcp;
    struct sockaddr_in from_addr;
    socklen_t from_size = sizeof(sockaddr_in);
    SetAddr(s_ip, s_port, &from_addr);
    
    ssize_t len = 0;
    char buffer[2048] = { 0 };
    socklen_t src_len = sizeof(struct sockaddr_in);

    while (true) {
        //ikcp_update(m_kcp, KCP::iclock());
        struct sockaddr_in src;
        memset(&src, 0, src_len);
        len = recvfrom(fd, buffer, sizeof(buffer), 0, (struct sockaddr *)&from_addr, &from_size);
        //std::cout << "len: " << len << std::endl;
        if (len > 0) {
            KCP::tcp_info info;
            KCP::prase_tcp_packet(buffer, len, &info);
            if (info.port_dst != s_port) {
                continue;
            }

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
	close(fd);
    delete param;
    pthread_exit(NULL);
}

void* run_loop(void* args) {
    cb_params * param = (cb_params*)args;
    ikcpcb* m_kcp = param->m_kcp;
    while (true) {
        ikcp_update(m_kcp, KCP::iclock());
        KCP::isleep(1);
    }
}


void kcp_client_start(int fd, ikcpcb* m_kcp) {
		//char buf[65535];
	ikcp_setoutput(m_kcp, tcp_client_cb);

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


    cb_params* params = new cb_params;
    params->fd = fd;
    params->m_kcp = m_kcp;

    pthread_t loop_tid;
    if (pthread_create(&loop_tid, NULL, run_loop, params) != 0) {
        perror("pthread_create");
    }


    pthread_t tid;
    if (pthread_create(&tid, NULL, run_tcp_client, params) != 0) {
        perror("pthread_create");
    };
}

int main() {
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    // const char* hello = "Hello from client";
    char buffer[BUFFER_SIZE] = {0};

    // 创建TCP socket
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    // // 设置 socket 为非阻塞模式
    // int flags = fcntl(sock, F_GETFL, 0);
    // if (flags == -1) {
    //     std::cerr << "Failed to get socket flags" << std::endl;
    //     return -1;
    // }

    // // 设置 socket 为非阻塞模式
    // if (fcntl(sock, F_SETFL, O_NONBLOCK) == -1) {
    //     std::cerr << "Failed to set non-blocking mode" << std::endl;
    //     return -1;
    // }

    // 设置服务器地址结构
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 将IPv4地址从点分十进制转换为网络字节序
    if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr)<=0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

 // 尝试连接到服务器
    int connect_status = connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
    if (connect_status == -1 && errno != EINPROGRESS) {
        exit(1);
    }
    

    // // 使用select进行非阻塞I/O操作
    // fd_set read_fds;
    // fd_set write_fds;
    // struct timeval timeout;
    // char buffer[1024];
    // int stdin_fd = fileno(stdin);
    // int max_fd = sock + 1;

    //  while (true) {
    //     FD_ZERO(&read_fds);
    //     FD_ZERO(&write_fds);

    //     FD_SET(sock, &read_fds);
    //     FD_SET(stdin_fd, &read_fds);

    //     if (connect_status == -1) {
    //         FD_SET(sock, &write_fds);
    //     }

    //     timeout.tv_sec = 5; // 设置超时时间为5秒
    //     timeout.tv_usec = 0;

    //     int select_status = select(max_fd, &read_fds, &write_fds, NULL, &timeout);
    //     if (select_status == -1) {
    //         perror("Error in select");
    //         break;
    //     } else if (select_status == 0) {
    //         std::cout << "Timeout occurred\n";
    //         break;
    //     }

    //     // 检查套接字是否可读
    //     if (FD_ISSET(sock, &read_fds)) {
    //         int bytes_received = recv(sock, buffer, sizeof(buffer), 0);
    //         if (bytes_received > 0) {
    //             buffer[bytes_received] = '\0';
    //             std::cout << "Received: " << buffer << std::endl;
    //         } else if (bytes_received == 0) {
    //             std::cout << "Server disconnected\n";
    //             break;
    //         } else {
    //             perror("Error receiving data");
    //             break;
    //         }
    //     }

    //     // 检查套接字是否可写
    //     if (FD_ISSET(sock, &write_fds)) {
    //         int optval;
    //         socklen_t optlen = sizeof(optval);
    //         if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &optval, &optlen) == 0) {
    //             if (optval == 0) {
    //                 std::cout << "Connected to server successfully\n";
    //                 connect_status = 0;
    //             } else {
    //                 std::cerr << "Error connecting to server\n";
    //                 break;
    //             }
    //         }
    //     }

    //     // 检查标准输入是否可读
    //     if (FD_ISSET(stdin_fd, &read_fds)) {
    //         fgets(buffer, sizeof(buffer), stdin);
    //         send(sock, buffer, strlen(buffer), 0);
    //     }
    // }
    // sleep(2);

	ikcpcb* ikcpcb;
	tcp_def* def = new tcp_def; 
    def->fd = sock;
    SetAddr(c_ip, c_port, &def->local_addr);
    SetAddr(s_ip, s_port, &def->remote_addr);
    ikcpcb = ikcp_create(0x1, (void*)def);
    kcp_client_start(sock, ikcpcb);
    

    // 发送消息给服务器
    std::string str;
    std::cout << "send: ";
    while (std::cin >> str) {
        int ret = ikcp_send(ikcpcb, str.c_str(), str.size());
        std::cout << "ret: " << ret << std::endl;
        if (ret < 0) { 
            perror("ikcp_input");
        }
        //send(sock, str.c_str(), str.size(), 0);
    }

    // 接收服务器的响应
    valread = read(sock, buffer, BUFFER_SIZE);
    std::cout << "服务器响应: " << buffer << "valread:  " << valread << std::endl;

    return 0;
}
