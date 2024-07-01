#ifndef __KCP_CLIENT_H__
#define __KCP_CLIENT_H__

#include <stdint.h>
#include <string>
#include "ikcp.h"
#include "kcp_socket.h"

namespace KCP {

class KCP_Client {
public:

    KCP_Client() {};
    ~KCP_Client() {
        delete m_kcp;
        delete udp_def;
    };

    void create(int mod, IUINT32 conv, char* remoteIp, uint16_t remotePort, uint16_t localPort);
    int sendData(std::string data);

    inline bool getIsLoop() const { return m_isLoop; };
    inline void setIsLoop(bool isLoop) { m_isLoop = isLoop; };

    inline UDP_Def* getUdpDef() const { return udp_def; };
    inline ikcpcb* getKcp() const { return m_kcp; };

    bool m_isLoop;
    ikcpcb* m_kcp;  
    UDP_Def* udp_def;
};

}

#endif