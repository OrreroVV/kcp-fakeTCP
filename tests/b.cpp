#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        perror("socket");
        return 1;
    }
    std::cout << "sock: " << sock << std::endl;
    sockaddr_in dest;
    socklen_t dest_len = sizeof(sockaddr_in);
    dest.sin_family = AF_INET;
    dest.sin_port = htons(6666);
    inet_pton(AF_INET, "8.138.86.207", &dest.sin_addr);
    int ret = connect(sock, (sockaddr*)&dest, dest_len);
    if (ret == -1) {
        perror("connect");
    }
    const char *message = "Hello, Server!";
    int send_result = sendto(sock, message, strlen(message), 0, (sockaddr*)&dest, sizeof(dest));
    std::cout << "send: " << send_result << std::endl;
    if (send_result == -1) {
        perror("sendto");
        return 1;
    }

    std::cout << "Message sent to 8.138.86.207:6666" << std::endl;

    close(sock);
    return 0;
}
