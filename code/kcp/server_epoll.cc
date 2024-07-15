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

#define RECV_MAX_SIZE 2048
#define SEND_MAX_SIZE 1024

ServerEpoll::ServerEpoll(pid_t pid, const char* server_ip, uint16_t server_port) : m_pid(pid), server_ip(server_ip), server_port(server_port){
    stopFlag.store(true);
}

ServerEpoll::~ServerEpoll() {
    for (auto [k, v] : clients) {
        close(k);
    }
    clients.clear();
    close(listen_sock);
}


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

    // 重地址
	int optval = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));

    // 设置SO_REUSEPORT选项
    optval = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        perror("setsockopt(SO_REUSEPORT)");
        exit(EXIT_FAILURE);
    }

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(server_port);
	// local.sin_addr.s_addr = inet_addr("0.0.0.0");
	

	if (bind(listen_sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(listen_sock, 4096) < 0) {
		perror("listen");
	}
    return 0;
}

void* ServerEpoll::updateKcp() {
    
    std::atomic<int>cnt = { 0 };
    while (!stopFlag.load()) {
        {
            std::lock_guard<std::mutex> lock(update_mutex);
            for (auto it = clients.begin(); it != clients.end();) {
                std::shared_ptr<KCP::KcpHandleClient> client = it->second;
                if (client->stopFlag.load()) {
                    it = clients.erase(it);
                    continue;
                }
                // std::cout << client->fd << " update ICKP update" << " waitsend: " << ikcp_waitsnd(client->m_kcp) << std::endl;
                {
                    std::lock_guard<std::mutex> lock(kcp_mutex);
                    ikcp_update(client->m_kcp, KCP::iclock());
                }

    

                ++it;
                continue;


                char buffer[RECV_MAX_SIZE + 10] {};
                int len;
                {
                    std::lock_guard<std::mutex> lock(kcp_mutex);
                    len = ikcp_recv(client->m_kcp, buffer, RECV_MAX_SIZE);
                }
                // std::cout << "recv len: " << len << std::endl;
                if (len > 0) {
                    std::cout << "recv len: " << len << std::endl;
                    
                    if (!client->read_file) {
                        assert(len >= 128 + 8);
                        client->read_file = true;

                        memcpy(&client->file_size, buffer, sizeof(uint32_t));
                        client->file_size = ntohl(client->file_size);
                        printf("File size: %u\n", client->file_size);

                        char file_name[128 + 10] = { 0 };
                        memcpy(file_name, buffer + sizeof(uint32_t), 128);
                        
                        
                        // printf("File name: %s\n", file_name);
                        client->filePath = client->prefix_path;
                        client->filePath += std::to_string(m_pid);
                        client->filePath += "_";
                        client->filePath += std::to_string(cnt.load());
                        client->filePath += ".txt";
                        cnt.fetch_add(1);
                        
                        // file_name = random_24();
                        // filePath = prefix_path + file_name;
                        client->file.open(client->filePath, std::ios::out | std::ios::binary);
                        
                        if (len > 8 + 128) {
                            client->file.write(buffer + 8 + 128, len - (8 + 128));
                            client->file_sended += len - (8 + 128);
                            // std::cout << "recv size: " << client->file_sended << std::endl;
                        }
                        continue;
                    }
                    client->file.write(buffer, len);
                    client->file_sended += len;
                    // std::cout << "recv size: " << client->file_sended << std::endl;
                    // std::cout << "file_sended: " << client->file_sended << "fd: " << fd << std::endl;
                    if (client->file_sended >= client->file_size) {
                        printf("File %s received completely, c_port: %d \n", client->filePath.c_str(), client->c_port);
                        client->file.close();
                        // flagSended = true;
                        std::string msg = "send_finish";
                        
                        std::lock_guard<std::mutex> lock(kcp_mutex);
                        ikcp_send(client->m_kcp, msg.c_str(), msg.size());
                    }
                }


            }
        }
        KCP::isleep(10);
    }
    return nullptr;
}

void ServerEpoll::create_thread() {
    stopFlag.store(false);
    update_thread = std::unique_ptr<std::thread>(new std::thread(&ServerEpoll::updateKcp, this));
    update_thread->detach();

    // update_thread = std::unique_ptr<std::thread>(new std::thread(&ServerEpoll::updateKcp, this));
    // update_thread->detach();
}

void ServerEpoll::send_file() {
    
}

void ServerEpoll::startEpoll() {

    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        perror("epoll_create1");
        close(listen_sock);
        exit(1);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, listen_sock, &ev) == -1) {
        perror("epoll_ctl: listen_sock");
        close(listen_sock);
        close(epoll_fd);
        exit(1);
    }

    create_thread();

    std::atomic<int>cnt = { 0 };


    struct epoll_event events[MAX_EVENTS];
    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }
        // std:: cout << "nfds: " << nfds << std::endl;
        for (int i = 0; i < nfds; ++i) {
            // std::cout << "event_data_fd: " << events[i].data.fd << std::endl;
            if (events[i].data.fd == listen_sock) {
                struct sockaddr_in peer;
                socklen_t len = sizeof(peer);
                int fd = accept(listen_sock, (struct sockaddr *)&peer, &len);
                // std::cout << "New connection recv pid: " << m_pid << "\n";
                if (fd == -1) {
                    perror("accept");
                    continue;
                }
                
                std::cout << "New connection from: " << inet_ntoa(peer.sin_addr) << ":" << ntohs(peer.sin_port) <<
                " new_fd: " << fd << "\n";
                uint32_t c_port = ntohs(peer.sin_port);
                char* c_ip = inet_ntoa(peer.sin_addr);
                // 设置新连接为非阻塞
                if (setNonBlocking(fd) == -1) {
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
                // std::cout << "Epoll create handleClient" << std::endl;
                std::shared_ptr<KCP::KcpHandleClient>handleClient(new KCP::KcpHandleClient(fd, server_port, server_ip, c_port, c_ip));
                handleClient->start_kcp_server();

                std::lock_guard<std::mutex> lock(update_mutex);
                clients[fd] = std::move(handleClient);


                // std::string file_path = "/home/hzh/workspace/work/logs/data_server.txt";

            	// // 打开文件并发送内容
	            // std::string filename = file_path.substr(file_path.find_last_of("/") + 1);

                // std::ifstream file(file_path, std::ios::in | std::ios::binary);
                // if (!file) {
                //     perror("send file");
                //     continue;
                // }

                // char temp[8 + 128]{};
                // // 获取文件大小
                // file.seekg(0, std::ios::end);
                // uint32_t file_size = file.tellg();
                // file.seekg(0, std::ios::beg);
                // // std::cout << "file_size: " << file_size << std::endl;
                // size_t file_size_n = htonl(file_size);
                

                // memcpy(temp, &file_size_n, sizeof(uint32_t));
                // memcpy(temp + sizeof(uint32_t), &filename[0], filename.size());
                // {
                //     std::lock_guard<std::mutex> lock(kcp_mutex);
                //     ikcp_send(clients[fd]->m_kcp, temp, sizeof(temp));
                // }

                // char buf[SEND_MAX_SIZE + 10];
                // size_t totalBytesRead = 0;
                // while (totalBytesRead < file_size) {
                //     size_t s = std::min(file_size - totalBytesRead, (size_t)SEND_MAX_SIZE);
                //     file.read(buf, s);
                //     std::streamsize bytesRead = file.gcount();
                //     totalBytesRead += bytesRead;
                //     {
                //         std::lock_guard<std::mutex> lock(kcp_mutex);
                //         int temp = ikcp_send(clients[fd]->m_kcp, buf, s);
                //     }
                //     // std::cout <<"send: " << s << "\n";
                // }
                // file.close();

                
            } else {
                uint32_t state = events[i].events;
                int fd = events[i].data.fd;
                std::shared_ptr<KCP::KcpHandleClient> client = clients[fd];

                if (state & (EPOLLHUP)) {
                    
                    // std::cout << "closing, close fd" << std::endl;
                    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
                        perror("epoll_ctl: EPOLL_CTL_DEL");
                    }
                    
                    client->stopFlag.store(true);
                    {
                        // std::lock_guard<std::mutex> lock(update_mutex);
                        // clients.erase(clients.find(events[i].data.fd));
                    }

                    // close(events[i].data.fd);
                    continue;
                }

                if (state & (EPOLLIN)) {
                    while (true) {
                        char recv_buffer[RECV_MAX_SIZE + 10] = {};
                        int ret = read(fd, recv_buffer, RECV_MAX_SIZE);
                        std::cout << "pid: " << m_pid << "read: " << ret << std::endl;
                        std::string filePath;
                        if (ret > 0) {
                            int kcp_ret;
                            {
                                std::lock_guard<std::mutex> lock(kcp_mutex);
                                kcp_ret = ikcp_input(client->m_kcp, recv_buffer, ret);
                            }
                            if (kcp_ret < 0) {
                                std::string msg = "send_finish";
                                send(client->fd, msg.c_str(), msg.size(), 0);
                                // printf("ikcp_input error: %d\n", ret);
                                continue;
                            }

                            while (true) {
                                char buffer[RECV_MAX_SIZE + 10] {};
                                int len;
                                {
                                    std::lock_guard<std::mutex> lock(kcp_mutex);
                                    len = ikcp_recv(client->m_kcp, buffer, ret);
                                }
                                if (len < 0) {
                                    break;
                                }
                                // std::cout << "len: " << len << std::endl;
                                std::cout << "recv len: " << len << std::endl;
                                if (len > 0) {
                                    // data
                                    if (!client->read_file) {
                                        assert(len >= 128 + 8);
                                        client->read_file = true;

                                        memcpy(&client->file_size, buffer, sizeof(uint32_t));
                                        client->file_size = ntohl(client->file_size);
                                        // printf("File size: %u\n", client->file_size);

                                        char file_name[128 + 10] = { 0 };
                                        memcpy(file_name, buffer + sizeof(uint32_t), 128);
                                        
                                        
                                        // printf("File name: %s\n", file_name);
                                        client->filePath = client->prefix_path;
                                        client->filePath += std::to_string(m_pid);
                                        client->filePath += "_";
                                        client->filePath += std::to_string(cnt.load());
                                        client->filePath += ".txt";
                                        cnt.fetch_add(1);
                                        
                                        // file_name = random_24();
                                        // filePath = prefix_path + file_name;
                                        client->file.open(client->filePath, std::ios::out | std::ios::binary);
                                        
                                        if (len > 8 + 128) {
                                            client->file.write(buffer + 8 + 128, len - (8 + 128));
                                            client->file_sended += len - (8 + 128);
                                            // std::cout << "recv size: " << client->file_sended << std::endl;
                                        }
                                        continue;
                                    }
                                    client->file.write(buffer, len);
                                    client->file_sended += len;
                                    // std::cout << "recv size: " << client->file_sended << std::endl;
                                    std::cout << "file_sended: " << client->file_sended << "fd: " << fd << std::endl;
                                    if (client->file_sended >= client->file_size) {
                                        printf("File %s received completely, c_port: %d \n", client->filePath.c_str(), client->c_port);
                                        client->file.close();
                                        // flagSended = true;
                                        std::string send_buffer = "send_finish";
                                        std::lock_guard<std::mutex> lock(kcp_mutex);
                                        ikcp_send(client->m_kcp, send_buffer.c_str(), send_buffer.size());
                                    }
                                }
                            }
                            
                        } else if (!ret) {
                            // std::cout << "closing, close fd" << fd << std::endl;
                            if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, nullptr) == -1) {
                                perror("epoll_ctl: EPOLL_CTL_DEL");
                            }
                            assert(client);
                            client->stopFlag.store(true);
                            break;
                            // client->Close();
                            {
                                // std::lock_guard<std::mutex> lock(update_mutex);
                                // clients.erase(clients.find(fd));
                            }
                            // close(fd);
                        } else {
                            break;
                        }
                    }

                }
            }
        }
    }
}

}