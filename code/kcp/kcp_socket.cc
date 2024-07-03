#include "kcp_socket.h"
#include <iostream>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define UDP_MTU 1460

namespace KCP {

void prase_tcp_packet(const char* buffer,size_t len, tcp_info* info) {
    iphdr ip_header;
    memcpy(&ip_header, buffer, sizeof(ip_header));

    tcphdr tcp_header;
    memcpy(&tcp_header, buffer + sizeof(ip_header), sizeof(tcphdr));

    memset(info, 0, sizeof(tcp_info));
    info->ack = ntohs(tcp_header.ack);
    info->ack_seq = ntohl(tcp_header.ack_seq);
    info->port_src = ntohs(tcp_header.th_sport);
    info->port_dst = ntohs(tcp_header.th_dport);
    info->seq = ntohl(tcp_header.seq);
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

	while (sum >> 16)
	{
		sum = (sum & 0xffff) + (sum >> 16);
	}

	delete[] new_data;
	return static_cast<uint16_t>(~sum);
}





int32_t udp_Create(UDP_Def *udp, uint32_t remoteIP, uint16_t remotePort, uint16_t localPort) {
    if (udp == NULL) {
        return -1;
    }

    udp->fd = -1;

    udp->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    fcntl(udp->fd, F_SETFL, O_NONBLOCK);//设置非阻塞
    std::cout << "fd: " << udp->fd << std::endl;
    if (udp->fd < 0) {
        return -1;
    }

    udp->remote_addr.sin_family = AF_INET;
    udp->remote_addr.sin_port = htons(remotePort);
    udp->remote_addr.sin_addr.s_addr = remoteIP;

    udp->local_addr.sin_family = AF_INET;
    udp->local_addr.sin_port = htons(localPort);
    udp->local_addr.sin_addr.s_addr = INADDR_ANY;

    
    int opt = 1;
    setsockopt(udp->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//chw
    fcntl(udp->fd, F_SETFL, O_NONBLOCK);//设置非阻塞


    if (bind(udp->fd, (struct sockaddr *)&udp->local_addr, sizeof(struct sockaddr_in)) < 0) {
        close(udp->fd);
        printf("[CreateUdpServer] Udp server bind failed,errno=[%d],plocalPort=[%d]",errno,localPort);
       
        return -2;
    }

    return 0;
}

int udp_sendData_cb(const char *buffer, int len, ikcpcb *kcp, void *user) {
    UDP_Def* udp_def =  (UDP_Def*)user;

    int sended = 0;
    while (sended < len) {
        size_t s = (len - sended);
        if(s > UDP_MTU) s = UDP_MTU;
        ssize_t ret = ::sendto(udp_def->fd, buffer + sended, s, MSG_DONTWAIT, (struct sockaddr*) &udp_def->remote_addr, sizeof(struct sockaddr));
        if(ret < 0){
            return -1;
        }
        sended += s;
        // size_t siz = std::min(UDP_MTU, len - sended);
        // ssize_t ret = ::sendto(udp_def->fd, buffer + sended, siz, MSG_DONTWAIT, (struct sockaddr*)&udp_def->remote_addr, sizeof(struct sockaddr));
        
        // if (ret < 0) {
        //     return -1;
        // }
        // sended += siz;
    }

    return (size_t)sended;
}


};