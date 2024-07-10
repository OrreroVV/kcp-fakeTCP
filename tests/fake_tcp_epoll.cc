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

int setNonBlocking(int fd) {
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

static const char *s_ip = "8.138.86.207";
static const char *c_ip = "192.168.61.243";
static short s_port = 6666;

// void test(int sock, int c_port) {
// 	std::unique_ptr<KCP::KcpClient> client(new KCP::KcpClient(sock, c_port, s_port, c_ip, s_ip, file_path));
// 	client->start_hand_shake();
// }

int main(int argc, char *argv[])
{
	static short c_port = 12345;

	int server = 0;
	if (argc >= 2 && strncmp(argv[1], "-s", 2) == 0)
	{
		server = 1;
	}
	
	std::string file_path;
	if (argc >= 4) {
		// if (argc >= 4 && strncmp(argv[2], "-p", 2) == 0) {
		// 	c_port = atoi(argv[3]);
		// }
		file_path = argv[2];
		c_port = atoi(argv[3]);
	}

	printf("run in %s mode...\n", server ? "server" : "client");


	if (server) {
		std::unique_ptr<KCP::ServerEpoll> serverEpoll(new KCP::ServerEpoll(s_ip, s_port));
		serverEpoll->startEpoll();
	}
	else {
		if (file_path.empty()) {
			file_path = "/home/hzh/workspace/work/logs/data.txt";
		}
		int sock = KCP::StartFakeTcp(c_ip, c_port);
		std::unique_ptr<KCP::KcpClient> client(new KCP::KcpClient(sock, c_port, s_port, c_ip, s_ip, file_path));
		client->start_hand_shake();
		close(sock);
	}

	return 0;
}
