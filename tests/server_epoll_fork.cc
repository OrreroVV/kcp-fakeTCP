#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <string>
#include <iostream>
#include <cstring>
#include <cstdlib>
#include <pcap.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <code/kcp/ikcp.h>
#include <code/kcp/kcp_socket.h>
#include "code/kcp/kcp_server.h"
#include "code/kcp/kcp_client.h"
#include "code/kcp/server_epoll.h"
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
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <vector>

#define UDP_MTU 1400
#define FORK_MAX_NUMS 4

static const char *s_ip = "8.138.86.207";
static short s_port = 6666;
// static const char *c_ip = "192.168.61.243";

// void test(int sock, int c_port) {
// 	std::unique_ptr<KCP::KcpClient> client(new KCP::KcpClient(sock, c_port, s_port, c_ip, s_ip, file_path));
// 	client->start_hand_shake();
// }

int create_socket() {
    int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (listen_sock < 0) {
		perror("socket");
		return -1;
	}

	 // 设置SO_REUSEPORT选项
    int optval = 1;
    if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)) == -1) {
        perror("setsockopt(SO_REUSEPORT)");
        exit(EXIT_FAILURE);
    }

	struct sockaddr_in local;
	local.sin_family = AF_INET;
	local.sin_addr.s_addr = INADDR_ANY;
	local.sin_port = htons(s_port);
	// local.sin_addr.s_addr = inet_addr("0.0.0.0");
	

	if (bind(listen_sock, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		return -1;
	}

	if (listen(listen_sock, 4096) < 0) {
		perror("listen");
	}
    return listen_sock;
}

int main(int argc, char *argv[])
{

    // std::vector<std::unique_ptr<KCP::ServerEpoll>> servers;

    for (int i = 0; i < FORK_MAX_NUMS; i++) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (!pid) {
			int listen_sock = create_socket();

			// int epoll_fd = epoll_create1(0);
			// if (epoll_fd == -1)
			// {
			// 	perror("epoll_create1");
			// 	close(listen_sock);
			// 	exit(1);
			// }
			// std::cout << "pid: " << getpid() << " " << epoll_fd << std::endl;

    		printf("run in server fd: %d\n", listen_sock);
	        std::unique_ptr<KCP::ServerEpoll> serverEpoll(new KCP::ServerEpoll(getpid(), s_ip, s_port));
            // servers.push_back(std::move(serverEpoll));
            serverEpoll->setListenSock(listen_sock);
			// serverEpoll->setEpollFd(epoll_fd);
            serverEpoll->startEpoll();
			return 0;
        }
    }

	// 父进程继续运行或退出，根据需要进行管理
    for (int i = 0; i < FORK_MAX_NUMS; ++i) {
        wait(nullptr);
    }
		
	return 0;
}
