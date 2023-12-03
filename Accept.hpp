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

#include "Connector.hpp"
#include "ConntectorChannel.hpp"

namespace ye {

class Acceptor /*: public IListenAble*/
{

public:
  Acceptor(Emiter *emiter, std::string_view ip, u_int16_t port,
           bool multishot = true);
  /* int res -> return value */

  void setError(std::function<void(std::error_code)> fun) &noexcept;
  void setConnect(std::function<void(Channel<Connector> conn)>) &noexcept;
  ~Acceptor();

private:
  InetAddress addr_;
  IChannel *acceptor_; // unique
  std::map<int, Channel<Connector>> connectors_;
  std::function<void(Channel<Connector> conn)> on_connect_;
  std::function<void(std::error_code ec)> on_connect_error_ =
      [](std::error_code ec) {};
};
}; // namespace ye
