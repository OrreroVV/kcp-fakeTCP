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

const int MAX_EVENTS = 10;

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
std::string file_path;
static short s_port = 6666;
std::map<int, std::unique_ptr<KCP::KcpClient>>clients;


void connectToServer(const std::string& server_ip, int server_port, int client_port) {
	// 打开原始套接字
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
	if (sock < 0) {
		perror("socket");
	}

	int on = 1;
	if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &on, sizeof(on)) < 0) {
		perror("setsockopt");
	}

	std::unique_ptr<KCP::KcpClient> client(new KCP::KcpClient(sock, client_port, s_port, c_ip, s_ip, file_path));
	client->start_hand_shake();
    clients[sock] = std::move(client);
}

void testConcurrency(const std::string& server_ip, int server_port, int num_connections) {
    // num_connections = 1;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_connections; ++i) {
        threads.emplace_back(connectToServer, server_ip, server_port, 20002 + i);
    }

    for (auto& t : threads) {
        t.join();
    }

}


int main(int argc, char *argv[])
{

	if (file_path.empty()) {
			file_path = "/home/hzh/workspace/kcp-fakeTCP/logs/data.txt";
	}

    int num_connections = 1; 

    auto start_time = std::chrono::high_resolution_clock::now();
    testConcurrency(s_ip, s_port, num_connections);
    auto end_time = std::chrono::high_resolution_clock::now();  

    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "Completed " << num_connections << " connections in " << duration.count() << " seconds" << std::endl;

	return 0;
}