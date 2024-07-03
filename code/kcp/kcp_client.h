#ifndef __KCP_HANDLE_CLIENT__
#define __KCP_HANDLE_CLIENT__

#include <stdint.h>
#include <string>
#include <memory>
#include "ikcp.h"
#include "kcp_socket.h"

namespace KCP {

struct tcp_def {
    int32_t fd;
    struct sockaddr_in local_addr;
    struct sockaddr_in remote_addr;
};

typedef struct __cb_params__ {
	int fd;
	ikcpcb* m_kcp;
} cb_params;

class KcpHandleClient {
public:
    typedef std::shared_ptr<KcpHandleClient> ptr;

    KcpHandleClient();


    void SetAddr(const char *ip, short port, struct sockaddr_in* addr);


    ikcpcb* m_kcp;
    tcp_def* tcp_def;
private:

};


// class KCP_Client {
// public:

//     KCP_Client() {};
//     ~KCP_Client() {
//         delete m_kcp;
//         delete udp_def;
//     };

//     void create(int mod, IUINT32 conv, char* remoteIp, uint16_t remotePort, uint16_t localPort);
//     int sendData(std::string data);

//     inline bool getIsLoop() const { return m_isLoop; };
//     inline void setIsLoop(bool isLoop) { m_isLoop = isLoop; };

//     inline UDP_Def* getUdpDef() const { return udp_def; };
//     inline ikcpcb* getKcp() const { return m_kcp; };

//     bool m_isLoop;
//     ikcpcb* m_kcp;  
//     UDP_Def* udp_def;
// };

}

#endif