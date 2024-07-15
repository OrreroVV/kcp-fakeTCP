#include <iostream>
#include <thread>
#include <vector>
#include <exception>

void threadFunction() {
    int x = 0;
    while (true) {
        x += 3;
        std::this_thread::sleep_for(std::chrono::seconds(100));
    }
}

int main() {
    std::vector<std::thread> threads;
    int threadCount = 0;

    try {
        while (true) {
            threads.push_back(std::thread(threadFunction));
            threadCount++;
            std::cout << "Created thread " << threadCount << std::endl;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        std::cerr << "Total threads created: " << threadCount << std::endl;
    }

    // Join all threads before exiting
    for (auto& thread : threads) {
        if (thread.joinable()) {
            thread.join();
        }
    }

    return 0;
}
