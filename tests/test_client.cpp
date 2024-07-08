// client.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6666
#define SERVER_IP "8.138.86.207"
#define BUFFER_SIZE 1024


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
std::string file_path;
static short s_port = 6666;


void connectToServer(const std::string& server_ip, int server_port, int client_port) {
	int sock = 0;
    struct sockaddr_in serv_addr;

    // 创建 socket 文件描述符
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 将地址转换成二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
    }

    // 发送消息到服务器
    const char *message = "Hello from client";
    send(sock, message, strlen(message), 0);
    std::cout << "Message sent to server" << std::endl;

    // 关闭连接
    close(sock);
}

void testConcurrency(const std::string& server_ip, int server_port, int num_connections) {
    // num_connections = 1;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_connections; ++i) {
        threads.emplace_back(connectToServer, server_ip, server_port, (rand() % 65535 + 65535) % 65535);
    }

    for (auto& t : threads) {
        t.join();
    }
}

int main(int argc, char *argv[]) {

	if (file_path.empty()) {
			file_path = "/home/hzh/workspace/kcp-fakeTCP/logs/data.txt";
	}

    int num_connections = 10000; // 设置你要测试的并发连接数

    auto start_time = std::chrono::high_resolution_clock::now();
    testConcurrency(s_ip, s_port, num_connections);
    auto end_time = std::chrono::high_resolution_clock::now();  

    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "Completed " << num_connections << " connections in " << duration.count() << " seconds" << std::endl;

	return 0;
}