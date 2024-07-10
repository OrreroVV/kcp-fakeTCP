#include <iostream>
#include <chrono>

int main() {
    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::seconds(3);

    while (std::chrono::high_resolution_clock::now() <= end) {
        // Your code here
        // For example, you can print a message or perform some calculations
        std::cout << "Running..." << std::endl;
    }

    std::cout << "Finished!" << std::endl;

    return 0;
}
