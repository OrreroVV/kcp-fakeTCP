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