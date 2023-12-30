#pragma once

#include "Connector.hpp"
#include "Dispatcher.hpp"
#include <spdlog/spdlog.h>

namespace ye {
class AcceptAdaptHandle;
template <> class Channel<AcceptAdaptHandle> : public IChannel {
public:
  Channel(Emiter *emiter, const ye::InetAddress &addr);

  inline virtual void readable(Buffer &data) {

    auto socket_fd = data.readInt32();

    if (on_connect_) [[likely]] {
      on_connect_(socket_fd);
    } else {
      spdlog::warn("[connect]"
                   "connect function donot set");
    }
  }

  inline virtual void errorable(const std::error_code &ec) {
    if (ec && on_connect_error_) [[unlikely]] {
      on_connect_error_(ec);
      return;
    }
  }

  void setConnect(std::function<void(int)> fun) &noexcept {
    if (on_connect_) {
      spdlog::warn("connect"
                   "alealy set the connect callback");
    }

    on_connect_ = std::move(fun);
  }

  int fd() const &noexcept { return fd_; }

  using connector_type = Connector *;
  connector_type createConnector(int fd) { return new Connector(fd); }

private:
  int fd_;
  Emiter *emiter_;

  std::function<void(int)> on_connect_;
  std::function<void(std::error_code ec)> on_connect_error_ =
      [](std::error_code ec) {};
};

} // namespace ye
