#include "Socket.hpp"
#include <arpa/inet.h>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <iostream>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string>
#include <system_error>
#include <unistd.h>
namespace ye::Socket {

void bindSocket(int sock, InetAddress addr, std::error_code &ec) {
  if (::bind(sock, addr.getSockAddr(), sizeof(addr)) == -1) {
    ec = std::make_error_code(static_cast<std::errc>(errno));
  } else {
    ec.clear();
  }
}

void listenSocket(int sock, int bk, std::error_code &ec) noexcept {
  if (::listen(sock, bk) == -1) {
    ec = std::make_error_code(static_cast<std::errc>(errno));
  }
}

void listenSocket(int sock, int bk) {
  std::error_code ec;
  listenSocket(sock, bk, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

int acceptConnection(int listenSocket, std::error_code &ec) noexcept {
  struct sockaddr_in clientAddr;
  socklen_t addrLen = sizeof(clientAddr);

  int clientSocket =
      ::accept(listenSocket, (struct sockaddr *)&clientAddr, &addrLen);
  if (clientSocket == -1) {
    ec = std::make_error_code(static_cast<std::errc>(errno));
  } else {
    ec.clear();

    // 获取客户端 IP 地址和端口号
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(clientAddr.sin_addr), clientIP, INET_ADDRSTRLEN);
    int clientPort = ntohs(clientAddr.sin_port);

    // 打印客户端信息
    printf("Accepted connection from %s:%d\n", clientIP, clientPort);
  }

  return clientSocket;
}

int acceptConnection(int listenSocket) {
  std::error_code ec;
  if (auto clientSocket = acceptConnection(listenSocket, ec); !ec) {
    return clientSocket;
  }
  throw std::system_error(ec);
}

void setSocket(int sock, int option, std::error_code &ec) noexcept {
  int optval = 1;
  int result = ::setsockopt(sock, SOL_SOCKET, option, &optval, sizeof(optval));
  if (result != 0) {
    ec = std::make_error_code(static_cast<std::errc>(errno));
  } else {
    ec.clear();
  }
}

void setSocket(int sock, int option) {
  std::error_code ec;
  setSocket(sock, option, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

// 设置 SO_REUSEADDR 选项
void setReuseAddr(int sock, std::error_code &ec) noexcept {
  setSocket(sock, SO_REUSEADDR, ec);
}

void setReuseAddr(int sock) {
  std::error_code ec;
  setReuseAddr(sock, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

// 设置 SO_REUSEPORT 选项
void setReusePort(int sock, std::error_code &ec) noexcept {
  setSocket(sock, SO_REUSEPORT, ec);
}

void setReusePort(int sock) {
  std::error_code ec;
  setReusePort(sock, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

void Close(int fd, std::error_code &ec) {
  if (auto res = ::close(fd); res < 0) {
    ec = std::error_code(errno, std::generic_category());
  }
}
void Close(IChannel &channel, std::error_code &ec) { Close(channel.fd(), ec); }

// 设置 SO_KEEPALIVE 选项
void setKeepAlive(int sock, std::error_code &ec) noexcept {
  setSocket(sock, SO_KEEPALIVE, ec);
}

void setKeepAlive(int sock) {
  std::error_code ec;
  setKeepAlive(sock, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

// 设置 TCP_NODELAY 选项
void setTcpNoDelay(int sock, std::error_code &ec) noexcept {
  setSocket(sock, TCP_NODELAY, ec);
}

void setTcpNoDelay(int sock) {
  std::error_code ec;
  setTcpNoDelay(sock, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

// 设置 TCP_CORK 选项
void setTcpCork(int sock, std::error_code &ec) noexcept {
  setSocket(sock, TCP_CORK, ec);
}

void setTcpCork(int sock) {
  std::error_code ec;
  setTcpCork(sock, ec);
  if (ec) {
    throw std::system_error(ec);
  }
}

void getSocketError(int sock, std::error_code &ec) {
  int code;
  socklen_t len = sizeof(code);
  if (auto ret = getsockopt(sock, SOL_SOCKET, SO_ERROR, &code, &len); ret < 0) {
    ec = std::error_code(errno, std::generic_category());
    return;
  }
  ec = std::error_code(errno, std::generic_category());
  ;
}
} // namespace ye::Socket
