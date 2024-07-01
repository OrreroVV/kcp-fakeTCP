#include "code/kcp/kcp_client.h"
#include <iostream>
//#include "code/kcp_client.h"

using namespace std;

int main() {
    //kcp_client *pkcp_client = new kcp_client(0x1, (char*)"127.0.0.1", 9001, 9002);
    KCP::KCP_Client *pkcp_client = new KCP::KCP_Client;
    pkcp_client->create(0, 0x1, (char*)"127.0.0.1", 9001, 9002);
    std::string msg;
    while (cin >> msg) {
        //auto temp = pkcp_client->sendData(msg.c_str(),msg.size());
        auto temp = pkcp_client->sendData(msg);
        std::cout << "data_size: " << temp << std::endl;
    }

    
    return 0;
}