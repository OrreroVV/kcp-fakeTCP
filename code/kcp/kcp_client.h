#ifndef __KCP_CLIENT__
#define __KCP_CLIENT__

#include <stdint.h>
#include <string>
#include <memory>
#include <atomic>
#include "ikcp.h"
#include "kcp_socket.h"

#define UDP_MTU 1400
#define BUFFER_SIZE 1024
#define IPV4_HEADER_SIZE (sizeof(struct iphdr))
#define TCP_HEADER_SIZE (sizeof(struct tcphdr))

// 计算 IP + TCP 头部的总大小
#define IP_TCP_HEADER_SIZE (IPV4_HEADER_SIZE + TCP_HEADER_SIZE)

namespace KCP {


int tcp_client_cb(const char *buffer, int len, ikcpcb *kcp, void *user);

class KcpClient {
public:
    typedef std::shared_ptr<KcpClient> ptr;
    enum conn_state {
        TCP_CLOSED = 0,
        TCP_SYN_SEND = 1,
        TCP_ESTABLISHED = 2,
        TCP_FIN_WAIT1 ,
        TCP_FIN_WAIT2 ,
        TCP_CLOSE_WAIT,
        TCP_CLOSING  ,
    };

    enum close_state {
    };

    KcpClient(int fd, uint16_t c_port, uint16_t s_port, const char* c_ip, const char* s_ip, std::string file_path);
    ~KcpClient();
    
    void start_hand_shake();
    void build_ip_tcp_header(char* data, const char* buffer, size_t data_len, int ack, int psh, int syn, int fin);
    void run_tcp_client();
    void *client_loop();
    void kcp_client_start();
    void send_file();
    void start_waving();

    int nonBlockingSend(const char *data, size_t len);


    void waving_send_fin();
    void waving_recv_ack();
    void waving_recv_fin();
    void waving_send_ack();
    
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
    // std::atomic<uint32_t> seq;
    std::atomic<uint32_t> server_ack_seq;
    // uint32_t server_ack_seq;
    std::atomic<uint32_t> CLIENT_SUM_SEND;
    //uint32_t CLIENT_SUM_SEND;

    std::atomic<bool> stopFlag;
    std::atomic<bool> finishSend;
    std::unique_ptr<std::thread> kcp_loop_thread;
    std::unique_ptr<std::thread> kcp_client_thread;
private:

};



}

#endif