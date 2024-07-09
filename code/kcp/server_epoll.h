#ifndef __SERVER_EPOLL_H__
#define __SERVER_EPOLL_H__

#include <map>
#include <memory>
#include "kcp_server.h"

namespace KCP {

#define MAX_EVENTS 1024

class ServerEpoll {
public:
    ServerEpoll();
    ~ServerEpoll();
    int setNonBlocking(int fd);
    int startServer();
    void startEpoll();

private:
    int listen_sock;
    int epoll_fd;
    const char* server_ip;
    uint16_t server_port;
    std::map<int, std::shared_ptr<KCP::KcpHandleClient>> clients;

};





}


#endif