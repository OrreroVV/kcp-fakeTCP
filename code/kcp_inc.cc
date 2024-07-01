#include "kcp_inc.h"

#define UDP_MTU 1460


/*
KCP回调函数，执行发送buffer数据
*/
int udp_sendData_loop(const char *buffer, int len, ikcpcb *kcp, void *user)
{
    UdpDef *pUdpClientDef = (UdpDef *)user;

    int sended = 0;
    while(sended < len){
        size_t s = (len - sended);
        if(s > UDP_MTU) s = UDP_MTU;
        ssize_t ret = ::sendto(pUdpClientDef->fd, buffer + sended, s, MSG_DONTWAIT, (struct sockaddr*) &pUdpClientDef->remote_addr, sizeof(struct sockaddr));
        if(ret < 0){
            return -1;
        }
        sended += s;
    }
    return (size_t) sended;
}

int32_t CreateUdp(UdpDef *udp, uint32_t remoteIp, int32_t remotePort, int32_t plocalPort)
{
    if (udp == NULL) return -1;
    udp->fd = -1;


    udp->fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    fcntl(udp->fd, F_SETFL, O_NONBLOCK);//设置非阻塞
    if(udp->fd < 0)
    {
        printf("[CreateUdpClient] create udp socket failed,errno=[%d],remoteIp=[%u],remotePort=[%d]",errno,remoteIp,remotePort);
        return -1;
    }

    udp->remote_addr.sin_family = AF_INET;
    udp->remote_addr.sin_port = htons(remotePort);
    udp->remote_addr.sin_addr.s_addr = remoteIp;

    udp->local_addr.sin_family = AF_INET;
    udp->local_addr.sin_port = htons(plocalPort);
    udp->local_addr.sin_addr.s_addr = INADDR_ANY;

    //2.socket参数设置
    int opt = 1;
    setsockopt(udp->fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));//chw
    fcntl(udp->fd, F_SETFL, O_NONBLOCK);//设置非阻塞

    // 本地地址绑定监听
    if (bind(udp->fd, (struct sockaddr*) &udp->local_addr, sizeof(struct sockaddr_in)) < 0)
    {
        close(udp->fd);
        printf("[CreateUdpServer] Udp server bind failed,errno=[%d],plocalPort=[%d]",errno,plocalPort);
        return -2;
    }
    return 0;
}