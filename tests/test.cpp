#include <iostream>
#include <vector>
#include <thread>
#include <algorithm>
#include <mutex>

// 定义线程池大小，根据需要调整
const int NUM_THREADS = 20;

// 互斥量，用于保护共享资源
std::mutex mtx;

// 快速排序算法
void quickSort(std::vector<int>& vec, size_t left, size_t right) {
    if (left < right) {
        size_t i = left, j = right;
        int pivot = vec[(left + right) / 2];

        while (i <= j) {
            while (vec[i] < pivot) i++;
            while (vec[j] > pivot) j--;
            if (i <= j) {
                std::swap(vec[i], vec[j]);
                i++;
                j--;
            }
        }

        if (left < j) quickSort(vec, left, j);
        if (i < right) quickSort(vec, i, right);
    }
}

// 每个线程的排序函数
void sortThread(std::vector<int>& vec, size_t start, size_t end) {
    quickSort(vec, start, end - 1);
}

// 合并两个已排序的子数组
void mergeSortedParts(std::vector<int>& vec, size_t start1, size_t end1, size_t start2, size_t end2) {
    std::vector<int> merged(end2 - start1);

    std::merge(vec.begin() + start1, vec.begin() + end1,
               vec.begin() + start2, vec.begin() + end2,
               merged.begin());

    std::copy(merged.begin(), merged.end(), vec.begin() + start1);
}

// 主排序函数，将任务分配给多个线程
void parallelSort(std::vector<int>& vec) {
    std::vector<std::thread> threads;
    size_t chunk_size = vec.size() / NUM_THREADS;

    // 创建并启动线程
    for (int i = 0; i < NUM_THREADS - 1; ++i) {
        threads.emplace_back(sortThread, std::ref(vec), i * chunk_size, (i + 1) * chunk_size);
    }
    // 最后一个线程处理剩余部分
    threads.emplace_back(sortThread, std::ref(vec), (NUM_THREADS - 1) * chunk_size, vec.size());

    // 等待所有线程完成
    for (auto& thread : threads) {
        thread.join();
    }

    // 合并已排序的部分
    for (int i = 1; i < NUM_THREADS; ++i) {
        mergeSortedParts(vec, 0, i * chunk_size, i * chunk_size, std::min((i + 1) * chunk_size, vec.size()));
    }
}

int main() {
    // 初始化一个随机向量
    int n = 100000;
    std::vector<int> data(n, 0);

    for (int i = 0; i < n; i++) {
        data[i] = rand();
    }

     // 开始计时
    auto start = std::chrono::high_resolution_clock::now();

    // 使用多线程排序
    parallelSort(data);
    // quickSort(data, 0, n - 1);
    // 结束计时
    auto end = std::chrono::high_resolution_clock::now();

    // 计算排序耗时
    std::chrono::duration<double> elapsed_seconds = end - start;
    std::cout << "Time spent on sorting: " << elapsed_seconds.count() << " seconds" << std::endl;


    return 0;
}
