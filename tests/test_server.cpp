#include <iostream>
#include <memory>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <csignal>
#include <vector>

#define MAX_EVENTS 10
#define PORT 6666
#define FORK_MAX_NUMS 4

int create_socket() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_sock < 0) {
        perror("socket");
        return -1;
    }

    int optval = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        perror("setsockopt(SO_REUSEPORT)");
        close(listen_sock);
        return -1;
    }

    struct sockaddr_in local;
    local.sin_family = AF_INET;
    local.sin_addr.s_addr = INADDR_ANY;
    local.sin_port = htons(PORT);

    if (bind(listen_sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
        perror("bind");
        close(listen_sock);
        return -1;
    }

    if (listen(listen_sock, 4096) < 0) {
        perror("listen");
        close(listen_sock);
        return -1;
    }

    return listen_sock;
}

void start_epoll_loop(int listen_sock, int epoll_fd) {
    struct epoll_event event;
    event.events = EPOLLIN;
    event.data.fd = listen_sock;

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &event) == -1) {
        perror("epoll_ctl");
        close(listen_sock);
        close(epoll_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            close(listen_sock);
            close(epoll_fd);
            exit(EXIT_FAILURE);
        }

        for (int n = 0; n < nfds; ++n) {
            if (events[n].data.fd == listen_sock) {
                struct sockaddr_in client_addr;
                socklen_t client_len = sizeof(client_addr);
                int conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
                if (conn_sock == -1) {
                    perror("accept");
                    continue;
                }

                // 将新连接添加到 epoll 实例中
                event.events = EPOLLIN | EPOLLET;
                event.data.fd = conn_sock;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, conn_sock, &event) == -1) {
                    perror("epoll_ctl: conn_sock");
                    close(conn_sock);
                }
            } else {
                // 处理已连接的套接字上的数据
                char buffer[1024];
                int count = read(events[n].data.fd, buffer, sizeof(buffer));
                if (count == -1) {
                    perror("read");
                    close(events[n].data.fd);
                } else if (count == 0) {
                    // 客户端关闭连接
                    close(events[n].data.fd);
                } else {
                    // 处理读取到的数据
                    std::cout << "Read " << count << " bytes: " << std::string(buffer, count) << std::endl;
                }
            }
        }
    }

    close(epoll_fd);
    close(listen_sock);
}

int main() {
    for (int i = 0; i < FORK_MAX_NUMS; ++i) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {  // 子进程
            int listen_sock = create_socket();
            if (listen_sock < 0) {
                exit(EXIT_FAILURE);
            }

            int epoll_fd = epoll_create1(0);
            if (epoll_fd == -1) {
                perror("epoll_create1");
                close(listen_sock);
                exit(EXIT_FAILURE);
            }

            std::cout << "pid: " << getpid() << " epoll_fd: " << epoll_fd << std::endl;

            start_epoll_loop(listen_sock, epoll_fd);
            exit(EXIT_SUCCESS);
        }
    }

    // 父进程等待所有子进程退出
    for (int i = 0; i < FORK_MAX_NUMS; ++i) {
        wait(nullptr);
    }

    return 0;
}
