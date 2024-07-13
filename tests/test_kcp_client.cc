#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>

// 校验和计算函数
unsigned short checksum(void *b, int len) {
    unsigned short *buf = (unsigned short*)b;
    unsigned int sum = 0;
    unsigned short result;

    for (sum = 0; len > 1; len -= 2)
        sum += *buf++;
    if (len == 1)
        sum += *(unsigned char *)buf;
    sum = (sum >> 16) + (sum & 0xFFFF);
    sum += (sum >> 16);
    result = ~sum;
    return result;
}

// 构建IP和TCP头部
void build_ip_tcp_header(char *packet, const char *src_ip, const char *dst_ip, int src_port, int dst_port) {
    struct iphdr *iph = (struct iphdr *)packet;
    struct tcphdr *tcph = (struct tcphdr *)(packet + sizeof(struct iphdr));

    // IP头部
    iph->ihl = 5;
    iph->version = 4;
    iph->tos = 0;
    iph->tot_len = sizeof(struct iphdr) + sizeof(struct tcphdr);
    iph->id = htonl(54321); // Id of this packet
    iph->frag_off = 0;
    iph->ttl = 255;
    iph->protocol = IPPROTO_TCP;
    iph->check = 0; // Set to 0 before calculating checksum
    iph->saddr = inet_addr(src_ip);
    iph->daddr = inet_addr(dst_ip);

    // TCP头部
    tcph->source = htons(src_port);
    tcph->dest = htons(dst_port);
    tcph->seq = 0;
    tcph->ack_seq = 0;
    tcph->doff = 5; // tcp header size
    tcph->fin = 0;
    tcph->syn = 1;
    tcph->rst = 0;
    tcph->psh = 0;
    tcph->ack = 0;
    tcph->urg = 0;
    tcph->window = htons(5840); // maximum allowed window size
    tcph->check = 0; // leave checksum 0 now, filled later by pseudo header
    tcph->urg_ptr = 0;

    // 计算TCP校验和
    struct pseudo_header {
        u_int32_t source_address;
        u_int32_t dest_address;
        u_int8_t placeholder;
        u_int8_t protocol;
        u_int16_t tcp_length;
    };

    struct pseudo_header psh;
    psh.source_address = inet_addr(src_ip);
    psh.dest_address = inet_addr(dst_ip);
    psh.placeholder = 0;
    psh.protocol = IPPROTO_TCP;
    psh.tcp_length = htons(sizeof(struct tcphdr));

    int psize = sizeof(struct pseudo_header) + sizeof(struct tcphdr);
    char *pseudogram = (char*)malloc(psize);

    memcpy(pseudogram, (char *)&psh, sizeof(struct pseudo_header));
    memcpy(pseudogram + sizeof(struct pseudo_header), tcph, sizeof(struct tcphdr));

    tcph->check = checksum((unsigned short *)pseudogram, psize);
    free(pseudogram);

    iph->check = checksum((unsigned short *)packet, iph->tot_len);
}

int main() {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if (sock < 0) {
        perror("socket error");
        return 1;
    }

    // 设置IP_HDRINCL选项，表示自己构造IP头部
    int one = 1;
    const int *val = &one;
    if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof(one)) < 0) {
        perror("setsockopt error");
        return 1;
    }

    // 数据包
    char packet[4096];
    memset(packet, 0, sizeof(packet));

    // 构建IP和TCP头部
    const char *src_ip = "192.168.61.243"; // 替换为本地有效IP地址
    const char *dst_ip = "8.168.86.207";  // 服务器的公网IP地址
    int src_port = 12345; // 源端口
    int dst_port = 6666;    // 目标端口
    build_ip_tcp_header(packet, src_ip, dst_ip, src_port, dst_port);

    // 目的地址
    struct sockaddr_in dest;
    dest.sin_family = AF_INET;
    dest.sin_port = htons(dst_port);
    dest.sin_addr.s_addr = inet_addr(dst_ip);

    // 发送数据包
    int ret = sendto(sock, packet, sizeof(struct iphdr) + sizeof(struct tcphdr), 0, (struct sockaddr *)&dest, sizeof(dest));
    if (ret < 0) {
        perror("sendto error");
        return 1;
    }
    std::cout << "ret: " << ret << std::endl;

    close(sock);
    return 0;
}
