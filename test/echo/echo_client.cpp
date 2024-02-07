#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <server IP address> <server port> <local IP address> <local port>" << std::endl;
        return 1;
    }

    const char* server_ip = argv[1];
    int server_port = std::stoi(argv[2]);
    const char* local_ip = argv[3];
    int local_port = std::stoi(argv[4]);

    // 创建socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // 设置本地地址
    struct sockaddr_in local_addr;
    std::memset(&local_addr, 0, sizeof(local_addr));
    local_addr.sin_family = AF_INET;
    local_addr.sin_port = htons(local_port);
    if (inet_pton(AF_INET, local_ip, &(local_addr.sin_addr)) <= 0) {
        std::cerr << "Invalid local IP address." << std::endl;
        return 1;
    }

    // 绑定socket到本地地址和端口
    if (bind(sockfd, (struct sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        std::cerr << "Failed to bind socket to local address." << std::endl;
        return 1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &(server_addr.sin_addr)) <= 0) {
        std::cerr << "Invalid server IP address." << std::endl;
        return 1;
    }

    // 连接到服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server." << std::endl;
        return 1;
    }

    // 向服务器发送数据
    char buffer[BUFFER_SIZE];
    std::cout << "Enter a message: ";
    std::cin.getline(buffer, BUFFER_SIZE);
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        std::cerr << "Failed to send data to server." << std::endl;
        return 1;
    }

    // 接收服务器的响应
    std::memset(buffer, 0, sizeof(buffer));
    if (recv(sockfd, buffer, sizeof(buffer), 0) < 0) {
        std::cerr << "Failed to receive data from server." << std::endl;
        return 1;
    }

    std::cout << "Server response: " << buffer << std::endl;

    // 关闭socket
    close(sockfd);

    return 0;
}