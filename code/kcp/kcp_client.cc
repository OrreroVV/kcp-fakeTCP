#include "kcp_client.h"
#include <iostream>

namespace KCP {

void setAddr(const char *ip, short port, struct sockaddr_in* addr) {
	assert(addr);
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = inet_addr(ip);
};



// void* run_udp(void* arg) {
//     KCP_Client* client = (KCP_Client*) arg;
//     ssize_t len = 0;
//     char buffer[2048] = { 0 };
//     socklen_t src_len = sizeof(struct sockaddr_in);

//     while (client->getIsLoop()) {
//         ikcp_update(client->getKcp(), iclock());
        
//         struct sockaddr_in src;
//         memset(&src, 0, src_len);
//         len = recvfrom(client->getUdpDef()->fd, buffer, 2048, 0, (struct sockaddr*)&src, &src_len);
//         if (len > 0) {
//             int ret = ikcp_input(client->getKcp(), buffer, len);
//             if (ret < 0) {
//                 printf("ikcp_input error: %d\n", ret);
//                 continue;
//             }

//             char recv_buffer[2048] = { 0 };
//             ret = ikcp_recv(client->getKcp(), recv_buffer, len);
//             if(ret >= 0) {
//                 printf("ikcp_recv ret = %d,buf=%s\n",ret, recv_buffer);
//             }
//         }
//         isleep(1);
//     }

//     close(client->getUdpDef()->fd);
//     return nullptr;
// }

// void KCP_Client::create(int mod, IUINT32 conv, char* remoteIp, uint16_t remotePort, uint16_t localPort) {
    
//     udp_def = new UDP_Def;

//     uint32_t remoteIp_uint = inet_addr(remoteIp);
//     udp_Create(udp_def, remoteIp_uint, remotePort, localPort);

//     m_kcp = ikcp_create(conv, (void*)udp_def);
//     //ikcp_setoutput(m_kcp, udp_sendData_cb);
//     m_kcp->output = udp_sendData_cb;

//     int sndwnd = 128, rcvwnd = 128;
//     ikcp_wndsize(m_kcp, sndwnd, rcvwnd);

//     switch (mod) {
//         case 0:
//             ikcp_nodelay(m_kcp, 0, 10, 0, 0);
//         case 1:
//             break;
//             ikcp_nodelay(m_kcp, 0, 10, 0, 1);
//             break;
//         case 2:
//             // 启动快速模式
//             // 第二个参数 nodelay-启用以后若干常规加速将启动
//             // 第三个参数 interval为内部处理时钟，默认设置为 10ms
//             // 第四个参数 resend为快速重传指标，设置为2
//             // 第五个参数 为是否禁用常规流控，这里禁止
//             ikcp_nodelay(m_kcp, 2, 10, 2, 1);
//             m_kcp->rx_minrto = 10;
//             m_kcp->fastresend = 1;
//             break;

//         default:
//             assert(false);
//             break;
//     }


//     m_isLoop = true;
//     pthread_t tid;
//     pthread_create(&tid, NULL, run_udp, this);
// }

// int KCP_Client::sendData(std::string data) {
//     int ret = ikcp_send(m_kcp, data.c_str(), data.size());
//     return ret;
// }

}