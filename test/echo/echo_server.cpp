#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define BUFFER_SIZE 4096

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <IP address> <port>" << std::endl;
        return 1;
    }

    const char* ip = argv[1];
    int port = std::stoi(argv[2]);

    // 创建socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }

    // 设置服务器地址
    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &(server_addr.sin_addr)) <= 0) {
        std::cerr << "Invalid IP address." << std::endl;
        return 1;
    }

    // 绑定socket到指定地址和端口
    if (bind(listenfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        return 1;
    }

    // 监听连接
    if (listen(listenfd, 5) < 0) {
        std::cerr << "Failed to listen." << std::endl;
        return 1;
    }

    std::cout << "Server is listening on " << ip << ":" << port << std::endl;

    // 接受连接
    int clientfd = accept(listenfd, NULL, NULL);
    if (clientfd < 0) {
        std::cerr << "Failed to accept connection." << std::endl;
        return 1;
    }

    // 接收客户端的数据
    char buffer[BUFFER_SIZE];
    std::memset(buffer, 0, sizeof(buffer));
    if (recv(clientfd, buffer, sizeof(buffer), 0) < 0) {
        std::cerr << "Failed to receive data from client." << std::endl;
        return 1;
    }

    std::cout << "Received message from client: " << buffer << std::endl;

    // 发送响应给客户端
    if (send(clientfd, buffer, strlen(buffer), 0) < 0) {
        std::cerr << "Failed to send data to client." << std::endl;
        return 1;
    }

    // 关闭连接
    close(clientfd);
    close(listenfd);

    return 0;
}