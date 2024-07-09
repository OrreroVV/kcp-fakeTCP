#include "server_epoll.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <cstdlib>
#include <unistd.h>

namespace KCP {

int ServerEpoll::setNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) {
        std::cerr << "Failed to get socket flags" << std::endl;
        return -1;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        std::cerr << "Failed to set non-blocking mode" << std::endl;
        return -1;
    }

    return 0;
}

int ServerEpoll::startServer() {
	listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock < 0) {
		perror("socket");   
        return -1;
	}


	int optval = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(server_port);
	// local.sin_addr.s_addr = inet_addr("0.0.0.0");
	

	if (bind(listen_sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
        return -1;
	}

	if (listen(listen_sock, 1024) < 0) {
		perror("listen");
        return -1;
	}
    return 0;
}

void ServerEpoll::startEpoll() {
    if (startServer() < 0) {
        return;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        close(listen_sock);
        exit(1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1)
    {
        perror("epoll_ctl: listen_sock");
        close(listen_sock);
        close(epoll_fd);
        exit(1);
    }

    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }
        std:: cout << "nfds: " << nfds << std::endl;
        for (int i = 0; i < nfds; ++i) {
            std::cout << "event_data_fd: " << events[i].data.fd << std::endl;
            if (events[i].data.fd == listen_sock) {
                struct sockaddr_in peer;
                socklen_t len = sizeof(peer);
                int fd = accept(listen_sock, (struct sockaddr *)&peer, &len);
                if (fd == -1) {
                    perror("accept");
                    continue;
                }
                
                std::cout << "New connection from: " << inet_ntoa(peer.sin_addr) << ":" << ntohs(peer.sin_port) <<
                " new_fd: " << fd << std::endl;
                
                uint32_t c_port = ntohs(peer.sin_port);
                char* c_ip = inet_ntoa(peer.sin_addr);
                // 设置新连接为非阻塞
                if (setNonBlocking(fd) == -1)
                {
                    close(fd);
                    continue;
                }

                ev.events = EPOLLIN | EPOLLHUP;
                ev.data.fd = fd;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
                    perror("epoll_ctl: conn_sock");
                    close(fd);
                    continue;
                }
                std::cout << "Epoll create handleClient" << std::endl;
                std::shared_ptr<KCP::KcpHandleClient>handleClient(new KCP::KcpHandleClient(fd, server_port, server_ip, c_port, c_ip));
                handleClient->start_kcp_server();
                clients[fd] = std::move(handleClient);

            } else {
                uint32_t state = events[i].events;
                int fd = events[i].data.fd;
                std::shared_ptr<KCP::KcpHandleClient> client = clients[fd];

                if (state & (EPOLLHUP)) {
                    std::cout << "closing, close fd" << std::endl;
                    clients.erase(clients.find(events[i].data.fd));
                    close(events[i].data.fd);
                    continue;
                }

                if (state & (EPOLLIN)) {
                    char recv_buffer[2048] = {};
                    int ret = read(fd, recv_buffer, 2048);
                    // std::cout << "read: " << ret << std::endl;
                    std::string filePath;
                    if (ret > 0) {
                        
                        int kcp_ret = ikcp_input(client->m_kcp, recv_buffer, ret);
                        if (kcp_ret < 0) {
                            printf("ikcp_input error: %d\n", ret);
                            continue;
                        }
                        char buffer[2048] {};
                        int len = ikcp_recv(client->m_kcp, buffer, ret);
                        // std::cout << "recv len: " << len << std::endl;
                        if (len > 0) {
                            // data
                        if (!client->read_file) {
                            assert(ret >= 128 + 8);
                            client->read_file = true;

                            memcpy(&client->file_size, buffer, sizeof(uint32_t));
                            client->file_size = ntohl(client->file_size);
                            // printf("File size: %u\n", client->file_size);

                            char file_name[128] = { 0 };
                            memcpy(file_name, buffer + sizeof(uint32_t), 128);
                            // printf("File name: %s\n", file_name);
                            
                            filePath = client->prefix_path + client->random_24() + ".txt";
                            // file_name = random_24();
                            // filePath = prefix_path + file_name;
                            client->file.open(filePath, std::ios::out | std::ios::binary);
                            
                            if (len > 8 + 128) {
                                client->file.write(buffer + 8 + 128, ret - (8 + 128));
                                client->file_sended += ret - (8 + 128);
                            }
                            continue;
                        }
                        client->file.write(buffer, ret);
                        client->file_sended += ret;
                        // std::cout << "file_sended: " << client->file_sended << "fd: " << fd << std::endl;
                        if (client->file_sended >= client->file_size) {
                            printf("File %s received completely\n", filePath.c_str());
                            client->file.close();
                            // flagSended = true;
                        }

                        }
                    } else if (!ret) {
                        std::cout << "closing, close fd" << fd << std::endl;
                        client->Close();
                        clients.erase(clients.find(fd));
                        close(fd);
                    } else {
                    }
                    

                }
            }
        }
    }
}

}