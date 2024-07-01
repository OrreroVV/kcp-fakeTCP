#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

// 线程函数，参数为 void*
void *thread_function(void *arg) {
    int thread_id = *((int *)arg);  // 传递给线程的参数

    printf("Thread %d is running\n", thread_id);
    sleep(3);  // 模拟线程执行一段时间

    printf("Thread %d is exiting\n", thread_id);

    pthread_exit(NULL);  // 线程退出
}

void test() {
    pthread_t tid;
    int thread_arg = 1;  // 传递给线程的参数，这里用整数1作为示例
        // 创建线程
    if (pthread_create(&tid, NULL, thread_function, (void *)&thread_arg) != 0) {
        perror("pthread_create failed");
        return;
    }
}

int main() {
    test();


    // 主线程继续执行其他任务
    printf("Main thread continues to run\n");

    // // 等待新创建的线程结束
    // if (pthread_join(tid, NULL) != 0) {
    //     perror("pthread_join failed");
    //     return 1;
    // }
    sleep(5);
    printf("Main thread exiting\n");

    return 0;
}

// #include <iostream>
// #include "code/kcp_client.h"

// using namespace std;

// int main() {
//     kcp_client *pkcp_client = new kcp_client(0x1, (char*)"127.0.0.1", 9002, 9001);
//     pkcp_client->create(0x1, (char*)"127.0.0.1", 9002, 9001);
//     std::string msg;
//     while (cin >> msg) {
//         auto temp = pkcp_client->sendData(msg.c_str(),msg.size());
//         std::cout << "data_size: " << temp << std::endl;
//     }

    
//     return 0; 
// }