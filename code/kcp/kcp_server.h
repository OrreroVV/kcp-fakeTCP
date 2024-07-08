#ifndef __KCP_HANDLE_CLIENT__
#define __KCP_HANDLE_CLIENT__

#include <stdint.h>
#include <string>
#include <memory>
#include <atomic>
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
    void* run_tcp_server();

    std::string random_24();
    void Close();


    
    int fd;
    int s_port;
    const char* s_ip;
    int c_port;
    const char* c_ip;
    bool read_file;
    std::ofstream file;
    std::string prefix_path;
    uint32_t file_sended;
	uint32_t file_size;

    ikcpcb* m_kcp;
    std::atomic<bool> stopFlag;
    std::unique_ptr<std::thread> tcp_server_thread;
private:

};

}

#endif