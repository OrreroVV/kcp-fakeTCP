#ifndef __SERVER_EPOLL_H__
#define __SERVER_EPOLL_H__

#include <map>
#include <memory>
#include <mutex>
#include "kcp_server.h"

namespace KCP {

#define MAX_EVENTS 1024

class ServerEpoll {
public:
    ServerEpoll(const char* server_ip, uint16_t server_port);
    ~ServerEpoll();
    int setNonBlocking(int fd);
    int startServer();
    void* updateKcp();
    void create_thread();
    void startEpoll();
    void setListenSock(int sock) { listen_sock = sock;}

private:
    int listen_sock;
    int epoll_fd;
    const char* server_ip;
    uint16_t server_port;
    std::atomic<bool> stopFlag;
    std::mutex update_mutex;
    std::map<int, std::shared_ptr<KCP::KcpHandleClient>> clients;
    std::unique_ptr<std::thread> update_thread;
};





}


#endif