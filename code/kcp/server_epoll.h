#ifndef __SERVER_EPOLL_H__
#define __SERVER_EPOLL_H__

#include <map>
#include <memory>
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

private:
    int listen_sock;
    int epoll_fd;
    const char* server_ip;
    uint16_t server_port;
    std::atomic<bool> stopFlag;
    std::map<int, std::shared_ptr<KCP::KcpHandleClient>> clients;
    std::map<std::string, ikcpcb*> update_queue;
    std::unique_ptr<std::thread> update_thread;
};





}


#endif