#ifndef __KCP_HANDLE_CLIENT__
#define __KCP_HANDLE_CLIENT__

#include <stdint.h>
#include <string>
#include <memory>
#include "ikcp.h"
#include "kcp_socket.h"


#define UDP_MTU 1400

namespace KCP {

int tcp_server_send_cb(const char *buffer, int len, ikcpcb *kcp, void *user);

class KcpHandleClient {
public:
    typedef std::shared_ptr<KcpHandleClient> ptr;

    KcpHandleClient(int fd, int s_port, const char* s_ip, int c_port, const char* c_ip);
    ~KcpHandleClient();

    void start_kcp_server();

    void* run_tcp_server(void* args);
    static void* start_tcp_server_thread(void* context) {
        KcpHandleClient* it = (KcpHandleClient*) context;
        cb_params* param = new cb_params;
	    param->fd = it->fd;
	    param->m_kcp = it->m_kcp;
        return it->run_tcp_server(param);
    }

    
    int fd;
    int s_port;
    const char* s_ip;
    int c_port;
    const char* c_ip;
    ikcpcb* m_kcp;
    bool looping;
private:

};

}

#endif