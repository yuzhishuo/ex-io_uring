#pragma once

#include "Accept.hpp"
#include "Connector.hpp"
#include "ConntectorChannel.hpp"
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
  Channel(Emiter *emiter, std::string_view ip, u_int16_t port,
          bool multishot = true)
      : IChannel(ChannelType::ListenSocket), emiter_{emiter},
        acceptor_(emiter, ye::InetAddress{ip, port}) {
    emiter->registerChannel(this); // when register newconnect affter
  }
  /* int res -> return value */

  void handleReadFinish(int fd, Buffer *buffer, std::error_code ec) noexcept {

    // swappe the Buffer class for when lifetime over it could be reuse in
    // Buffer Proxy
    if (ec && on_connect_error_) [[unlikely]] {
      on_connect_error_(ec);
      return;
    }
    // new emiter for CONNECTOR
    auto newconnector = Channel<Connector>(acceptor_.createConnector(fd));

    auto self = this;
    newconnector.setColse([&](auto connect) {
      self->emiter_->runAt([&]() { self->connectors_.erase(connect.fd()); });
    });

    if (on_connect_) [[likely]] {
      try {
        on_connect_(newconnector);
        connectors_.emplace_hint(std::end(connectors_), fd, newconnector);
      } catch (...) {
        // acceptor_ remove the connector
        spdlog::error("[connect]"
                      "create connect fail, remove connect");
        if (connectors_.contains(fd))
          connectors_.erase(fd);
      }
    } else {
      spdlog::warn("[connect]"
                   "connect function donot set");
      // close(fd);
    }
  }

  void setConnect(std::function<void(Channel<Connector>)> fun) &noexcept {
    if (on_connect_) {
      spdlog::warn("connect"
                   "alealy set the connect callback");
    }

    on_connect_ = std::move(fun);
  }
  void setError(std::function<void(std::error_code)> fun) &noexcept {
    if (on_connect_) {
      spdlog::warn("connect"
                   "alealy set the error callback");
    }
    on_connect_error_ = std::move(fun);
  }

  int fd() const &noexcept override { return acceptor_.fd(); }

private:
  Acceptor acceptor_;
  Emiter *emiter_;
  std::map<int, Channel<Connector>> connectors_;
  std::function<void(Channel<Connector> conn)> on_connect_;
  std::function<void(std::error_code ec)> on_connect_error_ =
      [](std::error_code ec) {};
};
} // namespace ye
