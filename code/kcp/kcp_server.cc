#include "kcp_server.h"
#include <iostream>

namespace KCP {

int tcp_server_send_cb(const char *buffer, int len, ikcpcb *kcp, void *user) {
    KcpHandleClient* client = static_cast<KcpHandleClient*>(user);

    int sended = 0;
	// std::cout << "server send cb len: " << len << std::endl;
	sockaddr_in client_addr;
	setAddr(client->c_ip, client->c_port, &client_addr);
	socklen_t addrlen = sizeof(client_addr);

    while (sended < len) {
        size_t s = (len - sended);
        if(s > UDP_MTU) s = UDP_MTU;
        ssize_t ret = sendto(client->fd, buffer + sended, s, 0, (sockaddr*)&client_addr, addrlen);
		// std::cout << "server send to: " << ret << std::endl;
        if(ret < 0){
            return -1;
        }
        sended += s;
    }
    return (size_t)sended;
}

KcpHandleClient::KcpHandleClient(int fd, uint16_t s_port, const char* s_ip, uint16_t c_port, const char* c_ip)
    :fd(fd), s_port(s_port), s_ip(s_ip), 
    c_port(c_port), c_ip(c_ip) {
	
	std::cout << "c_port: " << c_port << std::endl;
    m_kcp = ikcp_create(c_port, (void*)this);

    read_file = false;
    prefix_path = "/home/hzh/workspace/work/bin/";
    file_sended = 0;
	file_size = 0;
	filePath = "";

    assert(m_kcp);
}


KcpHandleClient::~KcpHandleClient() {
	std::cout << "~" << fd << std::endl;
	Close();
}

// 生成24位随机字符串的函数
std::string random_24() {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string random_string;
    random_string.reserve(24);

    // 初始化随机数生成器
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (size_t i = 0; i < 24; ++i) {
        random_string += characters[std::rand() % characters.size()];
    }
	return random_string;
}

void* KcpHandleClient::run_tcp_server_loop() {
	while (!stopFlag.load()) {
		ikcp_update(m_kcp, KCP::iclock());
		KCP::isleep(1);
	}
	return nullptr;
}

void* KcpHandleClient::run_tcp_server() {
    std::cout << "run_tcp_server" << std::endl;
	assert(fd && m_kcp);
	// struct sockaddr_in from_addr;
	// socklen_t from_size = sizeof(sockaddr_in);
	// SetAddr(s_ip, s_port, &from_addr);
	
	std::string prefix_path = "/home/hzh/workspace/work/bin/";
	std::string file_path;
	ssize_t len = 0;
	char buffer[2048] = { 0 };
    std::ofstream file;
    if (!file) {
        exit(1);
    }
	bool read_file = false;
	struct sockaddr_in src;
	// socklen_t src_len = sizeof(struct sockaddr_in);
	setAddr(c_ip, c_port, &src);
	uint32_t file_sended = 0;
	uint32_t file_size = 0;
	// bool flagSended = false;
	int cnt = -1;
	while (!stopFlag.load()) {
		cnt++;
		if (cnt % 1000 == 0) {
			std::cout << "looping" << std::endl;
		}
		ikcp_update(m_kcp, KCP::iclock());
		// KCP::isleep(1);
		// continue;
		
		len = read(fd, buffer, sizeof(buffer));
		if (len > 0) {
			std::cout << "read c_port: " << c_port << "len: " << len << std::endl;
			int ret = ikcp_input(m_kcp, buffer, len);
			if (ret < 0) {
				printf("ikcp_input error: %d\n", ret);
				continue;
			}
			// std::string send_buffer = "hello world";
			// ret = send(fd, send_buffer.c_str(), send_buffer.size(), 0);
			// if (ret == -1) {
			// 	perror("send");
			// }

			// 发送8 + 128字节确认文件大小，文件名
			char recv_buffer[2048] = { 0 };
			ret = ikcp_recv(m_kcp, recv_buffer, len);
			
			std::cout << "server ikcp_recv ret: " << ret << std::endl;
			if(ret > 0) {
				std::cout << "start recv file" << std::endl;
				if (!read_file) {
					assert(ret >= 128 + 8);
					read_file = true;

					memcpy(&file_size, recv_buffer, sizeof(uint32_t));
					file_size = ntohl(file_size);
					printf("File size: %u\n", file_size);

					char file_name[128] {};
					memcpy(file_name, recv_buffer + sizeof(uint32_t), sizeof(file_name));
					printf("File name: %s\n", file_name);
					
    				file_path = prefix_path + random_24() + ".txt";
					// file_name = random_24();
    				// file_path = prefix_path + file_name;
					file.open(file_path, std::ios::out | std::ios::binary);

					if (len > 8 + 128) {
						file.write(recv_buffer + 8 + 128, ret - (8 + 128));
						file_sended += ret - (8 + 128);
					}
					continue;
				}
				file.write(recv_buffer, ret);
				file_sended += ret;
				if (file_sended >= file_size) {
					printf("File %s received completely c_port: %d\n\n", file_path.c_str(), c_port);
					// flagSended = true;
				}
				// printf("ikcp_recv ret = %d,buf=%s\n",ret, recv_buffer);

			} else if (!ret) {
				std::cout << "ret = 0" << std::endl;
			}
		} else if (!len) {
			std::cout << "len: 0" << std::endl;
		} else {
			
		}
		KCP::isleep(10);
	}

	std::cout << "fd: " << fd << "tcp_server end" << std::endl;
	file.close();
	Close();
	return nullptr;
}

void KcpHandleClient::start_kcp_server() {
	//char buf[65535];
	assert(m_kcp && fd);
	ikcp_setoutput(m_kcp, tcp_server_send_cb);

	int sndwnd = 128, rcvwnd = 128;
    ikcp_wndsize(m_kcp, sndwnd, rcvwnd);
	int mod = 0;
    switch (mod) {
        case 0:
            ikcp_nodelay(m_kcp, 0, 40, 0, 0);
        case 1:
            break;
            ikcp_nodelay(m_kcp, 0, 10, 0, 1);
            break;
        case 2:
            // 启动快速模式
            // 第二个参数 nodelay-启用以后若干常规加速将启动
            // 第三个参数 interval为内部处理时钟，默认设置为 10ms
            // 第四个参数 resend为快速重传指标，设置为2
            // 第五个参数 为是否禁用常规流控，这里禁止
            ikcp_nodelay(m_kcp, 2, 10, 2, 1);
            m_kcp->rx_minrto = 10;
            m_kcp->fastresend = 1;
            break;

        default:
            assert(false);
            break;
    }


    stopFlag.store(false);
	
	// tcp_server_thread = std::unique_ptr<std::thread>(new std::thread(&KcpHandleClient::run_tcp_server, this));
	tcp_server_thread = std::unique_ptr<std::thread>(new std::thread(&KcpHandleClient::run_tcp_server_loop, this));
	tcp_server_thread->detach(); 
}

std::string KcpHandleClient::random_24() {
    const std::string characters = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    std::string random_string;
    random_string.reserve(24);

    // 初始化随机数生成器
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    for (size_t i = 0; i < 24; ++i) {
        random_string += characters[std::rand() % characters.size()];
    }
	return random_string;
}

void KcpHandleClient::Close() {
	if (stopFlag.load()) {
		return;
	}
	stopFlag.store(true);
	KCP::isleep(10);
    ikcp_release(m_kcp);
	m_kcp = nullptr;
	close(fd);
}


}