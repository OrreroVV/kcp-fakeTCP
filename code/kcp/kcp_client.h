#ifndef __KCP_CLIENT__
#define __KCP_CLIENT__

#include <stdint.h>
#include <string>
#include <memory>
#include <atomic>
#include "ikcp.h"
#include "kcp_socket.h"

#define UDP_MTU 1400

namespace KCP {


int tcp_client_cb(const char *buffer, int len, ikcpcb *kcp, void *user);

class KcpClient {
public:
    typedef std::shared_ptr<KcpClient> ptr;
    enum conn_state {
        tcp_closed = 0,
        tcp_syn_sent = 1,
        tcp_established = 2,
    };

    KcpClient(int fd, uint16_t c_port, uint16_t s_port, const char* c_ip, const char* s_ip, std::string file_path);
    ~KcpClient();
    
    void startClient();
    void run_tcp_client();
    void *client_loop();
    void kcp_client_start();

    void Close();

    int fd;
    int ip_id;
    int c_port;
    int s_port;
    const char* c_ip;
    const char* s_ip;
    std::string file_path;
    int s_state;
    ikcpcb* m_kcp;

    uint32_t seq;
    uint32_t server_ack_seq;
    uint32_t CLIENT_SUM_SEND;

    std::atomic<bool> stopFlag;
    std::unique_ptr<std::thread> kcp_loop_thread;
    std::unique_ptr<std::thread> kcp_client_thread;
private:

};



}

#endif