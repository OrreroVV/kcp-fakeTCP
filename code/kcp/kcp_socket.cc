#include "kcp_socket.h"
#include <iostream>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define UDP_MTU 1460

namespace KCP {

void prase_tcp_packet(const char* buffer, size_t len, tcp_info* info) {
    iphdr ip_header;
    memcpy(&ip_header, buffer, sizeof(iphdr));

    tcphdr tcp_header;
    memcpy(&tcp_header, buffer + sizeof(iphdr), sizeof(tcphdr));

    memset(info, 0, sizeof(tcp_info));
    info->ack = tcp_header.ack;
    info->ack_seq = ntohl(tcp_header.ack_seq);
    info->port_src = ntohs(tcp_header.th_sport);
    info->port_dst = ntohs(tcp_header.th_dport);
    info->seq = ntohl(tcp_header.seq);
	info->fin = tcp_header.fin;

	// std::cout << "ack: " << info->ack << std::endl;
    // std::cout << "seq: " << info->seq << std::endl;
    // std::cout << "port_src: " << info->port_src << " port_dst: " << info->port_dst << std::endl;
	// std::cout << "fin: " << info->fin << " " << tcp_header.fin << std::endl << std::endl << std::endl;

}

void setAddr(const char *ip, short port, struct sockaddr_in* addr) {
	assert(addr);
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
};

uint16_t calculate_ip_checksum(const struct iphdr *iphdr_packet) {
	uint32_t sum = 0;
	int length = iphdr_packet->ihl * 4;
	const uint16_t *buf = reinterpret_cast<const uint16_t *>(iphdr_packet);

	for (int i = 0; i < length / 2; ++i)
	{
		sum += ntohs(buf[i]);
	}

	if (length % 2)
	{
		sum += static_cast<uint16_t>(*(reinterpret_cast<const uint8_t *>(iphdr_packet) + length - 1));
	}

	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return static_cast<uint16_t>(~sum);
}


uint16_t calculate_tcp_checksum(const struct iphdr *iphdr_packet, const struct tcphdr *tcphdr_packet, const char *data, size_t data_len) {
	uint32_t sum = 0;

	pseudo_header pseudo_hdr;
	memset(&pseudo_hdr, 0, sizeof(pseudo_header));
	pseudo_hdr.source_address = iphdr_packet->saddr;
	pseudo_hdr.dest_address = iphdr_packet->daddr;
	pseudo_hdr.placeholder = 0;
	pseudo_hdr.protocol = IPPROTO_TCP;
	pseudo_hdr.tcp_length = htons(sizeof(struct tcphdr) + data_len);

	const uint16_t *pseudo_header_words = reinterpret_cast<const uint16_t *>(&pseudo_hdr);
	int pseudo_length = sizeof(pseudo_hdr);
	for (int i = 0; i < pseudo_length / 2; ++i)
	{
		sum += ntohs(pseudo_header_words[i]);
	}

	const uint16_t *tcp_packet = reinterpret_cast<const uint16_t *>(tcphdr_packet);
	int tcp_length = sizeof(struct tcphdr);
	for (int i = 0; i < tcp_length / 2; ++i)
	{
		sum += ntohs(tcp_packet[i]);
	}

	if (data_len) {
		char *new_data = new char[data_len + 1];
		std::memcpy(new_data, data, data_len);
		if (data_len & 1)
		{
			new_data[data_len] = 0;
			data_len++;
		}

		const uint16_t *data_packet = reinterpret_cast<const uint16_t *>(new_data);
		int data_length = data_len;
		for (int i = 0; i < data_length / 2; ++i)
		{
			sum += ntohs(data_packet[i]);
		}
		delete[] new_data;
	}

	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}

	return static_cast<uint16_t>(~sum);
}

int StartFakeTcp(const char *ip, short port) {
	// 打开原始套接字
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if (sock < 0) {
		perror("socket");
		return -1;
	}

	int on = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("setsockopt");
		return -1;
	}

	// // 设置 "don't fragment" 标志
    // on = IP_PMTUDISC_DO;  // IP_PMTUDISC_DO: 禁止分段
    // if (setsockopt(sock, IPPROTO_IP, IP_MTU_DISCOVER, &on, sizeof(on)) < 0) {
    //     perror("setsockopt failed");
    //     close(sock);
    //     return -1;
    // }


	// struct sockaddr_in local;
	// setAddr("127.0.0.1", port, &local);

	// if (bind(sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
	// 	perror("bind");
	// 	return -1;
	// }

	return sock;
}

int StartServer(const char *ip, short port) {
	int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock < 0) {
		perror("socket");
		return -1;
	}

	//     // 设置 socket 为非阻塞模式
    // int flags = fcntl(listen_sock, F_GETFL, 0);
    // if (flags == -1) {
    //     std::cerr << "Failed to get socket flags" << std::endl;
    //     return -1;
    // }

    // if (fcntl(listen_sock, F_SETFL, flags | O_NONBLOCK) == -1) {
    //     std::cerr << "Failed to set non-blocking mode" << std::endl;
    //     return -1;
    // }


	int optval = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(port);
	// local.sin_addr.s_addr = inet_addr("0.0.0.0");
	

	if (bind(listen_sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(listen_sock, 5) < 0) {
		perror("listen");
	}

	// //sleep(4);
	// struct sockaddr_in peer;
	// socklen_t len = sizeof(peer);	
	// int fd = accept(listen_sock, (struct sockaddr *)&peer, &len);
	// if (fd < 0) {
	// 	perror("accept");
	// }

	// printf("new connect: %s:%d\n", inet_ntoa(peer.sin_addr), ntohs(peer.sin_port));
	// close(listen_sock);
	return listen_sock;
}



};