#include <iostream>
#include <fstream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <chrono>
#include <vector>
#include <thread>

#define SERVER_IP "8.138.86.207"
#define SERVER_PORT 6666
#define BUFFER_SIZE 1024

void sendFile(const char* filepath, const char* filename) {
    // 创建socket
    int sock = 0;
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation error" << std::endl;
        return;
    }

    // 服务器地址设置
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    // 将地址转换为二进制形式
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address / Address not supported" << std::endl;
        return;
    }

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection Failed" << std::endl;
        return;
    }

    // 打开文件
    std::ifstream file(filepath, std::ios::in | std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Failed to open file" << std::endl;
        close(sock);
        return;
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    std::streamsize fileSize = file.tellg();
    file.seekg(0, std::ios::beg);
    

    // 准备头信息：文件大小(8字节) + 文件名(128字节)
    char header[8 + 128] = {0};
    memcpy(header, &fileSize, sizeof(fileSize));
    // memcpy(header + 8, &filename, 128);

    // 发送头信息
    send(sock, header, 8 + 128, 0);


    // 发送文件内容
    char buffer[BUFFER_SIZE];
    while (!file.eof()) {
        file.read(buffer, BUFFER_SIZE);
        std::streamsize bytesRead = file.gcount();
        send(sock, buffer, bytesRead, 0);
        std::cout << sock << " send: " << bytesRead << std::endl;
    }
    

    // 关闭文件和socket
    file.close();
    close(sock);
}

void testConcurrency(const char* filepath, const char* filename, int num_connections) {
    // num_connections = 1;
    std::vector<std::thread> threads;
    for (int i = 0; i < num_connections; ++i) {
        threads.emplace_back(sendFile, filepath, filename);
    }

    for (auto& t : threads) {
        t.join();
    }

}

int main() {
    const char* filepath = "/home/hzh/workspace/kcp-fakeTCP/logs/data.txt";
    const char* filename = "data.txt"; // 该文件名会发送给服务器，可以与实际文件名不同
    // 开始计时
    // int num = 1;
    auto start = std::chrono::high_resolution_clock::now();
    sendFile(filepath, filename);
    // testConcurrency(filepath, filename, num);
    // 结束计时
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "File sent successfully in " << elapsed.count() << " seconds" << std::endl;
    return 0;
}