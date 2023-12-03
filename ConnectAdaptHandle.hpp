#pragma once

#include "Concept.hpp"
#include "ConnectAdapt.hpp"
#include "Connector.hpp"
#include "ConntectorChannel.hpp"

#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "InetAddress.hpp"
#include "Socket.hpp"
#include <map>
#include <memory>
#include <string_view>

namespace ye {

// class ConnectAdaptHandle;
// template <> class Channel<ConnectAdaptHandle>;
struct ConnectAdaptHandle : public Handle {

public:
  ConnectAdaptHandle(Emiter *emiter, std::string_view ip, uint16_t port = 0);
  inline void connect(std::string_view ip, uint16_t port = 0)
      __attribute__((always_inline)) {
    connect(InetAddress{ip, port});
  }

  void connect(const InetAddress& addr);

  inline void setNewConnect(std::function<void(Connector &)> func)
      __attribute__((always_inline)) {
    if (func)
      on_new_connect_ = std::move(func);
  }

  inline void setOnClose(std::function<void(InetAddress, int)> func)
      __attribute__((always_inline)) {
    if (func)
      on_close_ = std::move(func);
  }

private:
  // Timer   // TODO: support connect timeout ?

  Emiter *emiter_;
  std::unique_ptr<Channel<ConnectAdaptHandle>> adaptor_;
  // TODO: remove connectors_ && new_connectors_
  std::map<int, Connector *> connectors_;
  std::map<int, Connector *> new_connectors_;
  std::function<void(Connector &)> on_new_connect_;
  std::function<void(InetAddress, int)> on_close_;
};
} // namespace ye