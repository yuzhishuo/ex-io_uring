#include "Accept.hpp"
#include "AcceptChannel.hpp"
namespace ye {

Acceptor::Acceptor(Emiter *emiter, std::string_view ip, u_int16_t port,
                   bool multishot)
    : addr_{ip, port}, acceptor_(nullptr) {

  // factory
  auto accept = new Channel<Acceptor>(emiter, addr_);
  accept->setConnect([&](Connector *conn) {
    auto newconnector = Channel<Connector>(conn);

    auto self = this;
    newconnector.setColse(
        [&](auto connect) { self->connectors_.erase(connect.fd()); });

    if (on_connect_) [[likely]] {
      try {
        on_connect_(newconnector);
        connectors_.emplace_hint(std::end(connectors_), conn->fd(),
                                 newconnector);
      } catch (...) {
        // acceptor_ remove the connector
        spdlog::error("[connect]"
                      "create connect fail, remove connect");
        if (connectors_.contains(conn->fd()))
          connectors_.erase(conn->fd());
      }
    } else {
      spdlog::warn("[connect]"
                   "connect function donot set");
      // close(fd);
      // emiter->unregisterChannel
    }
    emiter->registerChannel(accept); // when register newconnect affter
  });
  acceptor_ = accept;
  emiter->registerChannel(accept); // when register newconnect affter
}
/* int res -> return value */

void Acceptor::setError(std::function<void(std::error_code)> fun) &noexcept {
  if (on_connect_error_) {
    spdlog::warn("connect"
                 "alealy set the error callback");
  }
  on_connect_error_ = std::move(fun);
}

void Acceptor::setConnect(
    std::function<void(Channel<Connector>)> fun) &noexcept {
  if (on_connect_) {
    spdlog::warn("connect"
                 "alealy set the connect callback");
  }
  on_connect_ = std::move(fun);
}

Acceptor::~Acceptor() {
  if (nullptr != acceptor_) {
    delete acceptor_;
  }
}

} // namespace ye
