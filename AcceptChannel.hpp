#pragma once

#include "Accept.hpp"
#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "IListenAble.hpp"
#include "InetAddress.hpp"
#include "Timer.hpp"
#include <concepts>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <system_error>
#include <utility>

namespace ye {

template <> class Channel<Acceptor> : public IChannel {
public:
  Channel(Emiter *emiter, const ye::InetAddress &addr)
      : IChannel(ChannelType::ListenSocket), fd_{0}, emiter_{emiter} {
    std::error_code ec;
    fd_ = Socket::createSocket<Socket::Proto::kTcp>(ec);
    Socket::bindSocket(fd_, addr, ec);
    Socket::listenSocket(fd_, 10, ec);
    spdlog::info("listen in ip:{}, port:{}, result:{}", addr.toIp(),
                 addr.port(), ec.message());
  }

  void handleReadFinish(int fd, Buffer *buffer, std::error_code ec) noexcept {

    // swappe the Buffer class for when lifetime over it could be reuse in
    // Buffer Proxy
    if (ec && on_connect_error_) [[unlikely]] {
      on_connect_error_(ec);
      return;
    }

    if (on_connect_) [[likely]] {
      on_connect_(createConnector(fd));
    } else {
      spdlog::warn("[connect]"
                   "connect function donot set");
    }
  }

  void setConnect(std::function<void(Connector *)> fun) &noexcept {
    if (on_connect_) {
      spdlog::warn("connect"
                   "alealy set the connect callback");
    }

    on_connect_ = std::move(fun);
  }

  int fd() const &noexcept { return fd_; }

private:
  using connector_type = Connector *;
  connector_type createConnector(int fd) { return new Connector(fd); }

private:
  int fd_;
  Emiter *emiter_;

  std::function<void(Connector *conn)> on_connect_;
  std::function<void(std::error_code ec)> on_connect_error_ =
      [](std::error_code ec) {};
};

} // namespace ye
