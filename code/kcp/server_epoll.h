#ifndef __SERVER_EPOLL_H__
#define __SERVER_EPOLL_H__

#include <map>
#include <memory>
#include <mutex>
#include "kcp_server.h"

namespace KCP {

#define MAX_EVENTS 128

class ServerEpoll {
public:
    ServerEpoll(pid_t pid, const char* server_ip, uint16_t server_port);
    ~ServerEpoll();
    int setNonBlocking(int fd);
    int startServer();
    void* updateKcp();

    void create_thread();
    void send_file();

    void startEpoll();
    void setListenSock(int sock) { listen_sock = sock;}
    void setEpollFd(int fd) { epoll_fd = fd;}

private:
    pid_t m_pid;
    int listen_sock;
    int epoll_fd;
    const char* server_ip;
    uint16_t server_port;
    std::atomic<bool> stopFlag;
    std::mutex update_mutex;
    std::mutex kcp_mutex;
    std::map<int, std::shared_ptr<KCP::KcpHandleClient>> clients;
    
    std::unique_ptr<std::thread> update_thread;
 
};





}


#endif