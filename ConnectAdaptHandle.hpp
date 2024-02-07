#pragma once

#include "Buffer.hpp"
#include "Concept.hpp"
#include "ConnectAdapt.hpp"
#include "Connector.hpp"

#include "Emiter.hpp"
#include "InetAddress.hpp"
#include "yedefine.hpp"
#include <functional>
#include <map>
#include <memory>
#include <string_view>

namespace ye {

struct ConnectAdaptHandle : public Meta<MetaType::TCPCONNECT> {

public:
  ConnectAdaptHandle(Emiter *emiter, std::string_view ip, uint16_t port = 0);
  inline y_return_code connect(std::string_view ip, uint16_t port = 0)
      __attribute__((always_inline)) {
    return connect(InetAddress{ip, port});
  }

  y_return_code connect(const InetAddress &addr);

  inline void setNewConnect(std::function<void(Connector &)> func)
      __attribute__((always_inline)) {
    if (func)
      on_new_connect_ = std::move(func);
  }

  inline void setOnMessage(std::function<void(Buffer)> func)
      __attribute__((always_inline)) {

    if (func)
      on_message_ = std::move(func);
  }

  inline void setOnClose(std::function<void(InetAddress, int)> func)
      __attribute__((always_inline)) {
    if (func)
      on_close_ = std::move(func);
  }

private:
  // Timer   // TODO: support connect timeout ?
  Emiter *emiter_;
  InetAddress sa_;
  std::unique_ptr<Channel<ConnectAdaptHandle>> adaptor_;
  std::map<int, Connector *> connectors_;
  std::map<int, Connector *> new_connectors_;
  std::function<void(Buffer)> on_message_;
  std::function<void(Connector &)> on_new_connect_;
  std::function<void(InetAddress, int)> on_close_;
};
} // namespace ye