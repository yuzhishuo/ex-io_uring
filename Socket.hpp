#pragma once
#include "IChannelAdapter.hpp"
#include "InetAddress.hpp"
#include "SocketsOps.hpp"
#include <cstdint>
#include <sys/socket.h>
#include <system_error>
namespace ye::Socket {
static std::error_code default_ec = {};

enum class Proto : int {

  kTcp = SOCK_STREAM,
  kUdp = SOCK_PACKET,
};

// FIXME: ADD NOBLOCK && BINDEVICE func
template <Proto type> int createSocket(std::error_code &ec) noexcept {
  int sock = ::socket(AF_INET, static_cast<int>(type), 0);
  if (sock == -1) {
    ec = std::make_error_code(static_cast<std::errc>(errno));
  } else {
    ec.clear();
  }

  return sock;
}

template <Proto type> int createSocket() {

  std::error_code ec;
  if (auto sock = createSocket<type>(ec); !ec) {
    return sock;
  }

  throw std::system_error(ec);
}

void bindSocket(int sock, InetAddress addr, std::error_code &ec);

void listenSocket(int sock, int bk, std::error_code &ec) noexcept;

void listenSocket(int sock, int bk);
void Close(IChannel &channel, std::error_code &ec = default_ec);
void Close(int fd, std::error_code &ec = default_ec);

int acceptConnection(int listenSocket, std::error_code &ec) noexcept;

int acceptConnection(int listenSocket);

void setSocket(int sock, int option, std::error_code &ec) noexcept;

void setSocket(int sock, int option);

// 设置 SO_REUSEADDR 选项
void setReuseAddr(int sock, std::error_code &ec) noexcept;

void setReuseAddr(int sock);

// 设置 SO_REUSEPORT 选项
void setReusePort(int sock, std::error_code &ec) noexcept;

void setReusePort(int sock);

// 设置 SO_KEEPALIVE 选项
void setKeepAlive(int sock, std::error_code &ec) noexcept;

void setKeepAlive(int sock);

// 设置 TCP_NODELAY 选项
void setTcpNoDelay(int sock, std::error_code &ec) noexcept;

// 设置 TCP_CORK 选项
void setTcpCork(int sock, std::error_code &ec) noexcept;

void setTcpCork(int sock);

void getSocketError(int sock, std::error_code &ec);

} // namespace ye::Socket
