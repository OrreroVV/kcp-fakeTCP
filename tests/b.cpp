#include <stdio.h>
#include <stdlib.h>

#define DATA_SIZE 10000

int main() {
    FILE *file;
    char data[DATA_SIZE];

    // 填充数据
    for (int i = 0; i < DATA_SIZE; i++) {
        data[i] = 'A'; // 这里填充字符 'A'，可以根据需要修改
    }

    // 打开文件（创建文件）
    file = fopen("/home/hzh/workspace/kcp-fakeTCP/logs/data.txt", "wb");
    if (file == NULL) {
        perror("无法打开文件");
        exit(EXIT_FAILURE);
    }

    // 写入数据
    size_t written = fwrite(data, sizeof(char), DATA_SIZE, file);
    if (written != DATA_SIZE) {
        perror("写入文件失败");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    // 关闭文件
    fclose(file);

    printf("已成功写入10000个字节的数据到output.txt\n");

    return 0;
}
