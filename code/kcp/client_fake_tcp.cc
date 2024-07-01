#include "client_fake_tcp.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#define SRC_IP "192.168.1.100"
#define DST_IP "192.168.1.1"
#define SRC_PORT 12345
#define DST_PORT 80

#define TCP_PKT_LEN sizeof(struct iphdr) + sizeof(struct tcphdr)

unsigned short checksum(unsigned short *ptr, int nbytes) {
    unsigned long sum;
    unsigned short oddbyte;
    unsigned short answer;

    sum = 0;
    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }
    if (nbytes == 1) {
        oddbyte = 0;
        *((unsigned char *)&oddbyte) = *(unsigned char *)ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = (unsigned short)~sum;
    return answer;
}

unsigned short tcp_checksum(struct iphdr *ip, struct tcphdr *tcp) {
    // 计算校验和时需要伪造的头部
    struct pseudo_header {
        uint32_t source_address;
        uint32_t dest_address;
        uint8_t placeholder;
        uint8_t protocol;
        uint16_t tcp_length;
    } pseudo_header;

    // 填充伪造的 TCP 头部
    struct pseudo_header psh;
    memset(&psh, 0, sizeof(struct pseudo_header));
    
    psh.source_address = ip->saddr;
    psh.dest_address = ip->daddr;
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr));

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr);
    char *pseudogram = (char*)malloc(psize);

    memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcp, sizeof(struct tcphdr));

    tcp->check = checksum((unsigned short *)pseudogram, psize);
    free(pseudogram);

    return tcp->check;
}

int main() {
    // 创建原始套接字
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (sock < 0) {
        perror("socket creation failed");
        return 1;
    }

    // 设置 IP_HDRINCL 选项
    int on = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
        perror("setsockopt for IP_HDRINCL failed");
        close(sock);
        return 1;
    }
    // 构造 TCP 报文
    char packet[TCP_PKT_LEN];
    memset(packet, 0, TCP_PKT_LEN);
    struct iphdr *ip = (struct iphdr *)packet;
    struct tcphdr *tcp = (struct tcphdr *)(packet + sizeof(struct iphdr));

    // 设置 IP 头部
    ip->ihl = 5;
    ip->version = 4;
    ip->tos = 0;
    ip->tot_len = htons(TCP_PKT_LEN);
    ip->id = htonl(54321); // 假设的 ID
    ip->frag_off = 0;
    ip->ttl = 255;
    ip->protocol = IPPROTO_TCP;
    ip->saddr = inet_addr(SRC_IP);
    ip->daddr = inet_addr(DST_IP);

    // 设置 TCP 头部
    tcp->source = htons(SRC_PORT);
    tcp->dest = htons(DST_PORT);
    tcp->seq = htonl(123456); // 假设的序列号
    tcp->ack_seq = 0;
    tcp->doff = 5;  // TCP 头部长度
    tcp->syn = 1;   // 设置 SYN 标志
    tcp->window = htons(5840);  // 窗口大小
    tcp->check = 0; // 校验和先设为0，稍后再计算

    // 计算 IP 和 TCP 的校验和
    ip->check = 0;
    tcp->check = 0;
    ip->check = checksum((unsigned short *)packet, sizeof(struct iphdr));
    tcp->check = tcp_checksum(ip, tcp);

    // 发送伪造的 TCP 报文
    struct sockaddr_in dest_addr;
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr.s_addr = inet_addr(DST_IP);

    if (sendto(sock, packet, TCP_PKT_LEN, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
        perror("sendto failed");
        close(sock);
        return 1;
    }

    close(sock);
    return 0;
}
