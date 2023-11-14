#pragma once
#include "Concept.hpp"
#include "Connector.hpp"
#include "Emiter.hpp"
#include "IListenAble.hpp"
#include "InetAddress.hpp"
#include "Socket.hpp"
#include <assert.h>
#include <ios>
#include <map>
#include <memory>
#include <system_error>
namespace ye {

class Acceptor /*: public IListenAble*/ {
public:
  Acceptor(Emiter *Emiter, ye::InetAddress addr) : fd_{0}, addr_{addr} {
    std::error_code ec;
    fd_ = Socket::createSocket<Socket::Proto::kTcp>(ec);
    Socket::bindSocket(fd_, addr_, ec);
    Socket::listenSocket(fd_, 10, ec);
    spdlog::info("listen in ip:{}, port:{}, result:{}", addr_.toIp(),
                 addr_.port(), ec.message());
  }

  using connector_type = Connector *;
  int fd() const &noexcept { return fd_; }
  connector_type createConnector(int fd) { return new Connector(fd); }

private:
  int fd_;
  ye::InetAddress addr_;
};

static_assert(AcceptorConcept<Acceptor>);
}; // namespace ye
