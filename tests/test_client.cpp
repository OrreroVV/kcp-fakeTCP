// client.cpp
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 6666
#define SERVER_IP "8.138.86.207"
#define BUFFER_SIZE 1024

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};

    // 创建 socket 文件描述符
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // 将地址转换成二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address/ Address not supported" << std::endl;
        return -1;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return -1;
    }

    // 发送消息到服务器
    const char *message = "Hello from client";
    send(sock, message, strlen(message), 0);
    std::cout << "Message sent to server" << std::endl;

    // 读取服务器的响应
    int valread = read(sock, buffer, BUFFER_SIZE);
    std::cout << "Received from server: " << buffer << std::endl;
    std::cout << "valread: " << valread << std::endl;
    // 关闭连接
    close(sock);
    return 0;
}
