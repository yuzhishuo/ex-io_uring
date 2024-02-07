#pragma once
#include "Concept.hpp"
#include "Emiter.hpp"
#include <functional>
#include "AcceptChannel.hpp"
#include <memory>
#include <string_view>
namespace ye {
class AcceptAdaptHandle: public Meta<MetaType::TCPACCEPT> {
public:
  AcceptAdaptHandle(Emiter *emiter, std::string_view ip, uint16_t port = 0);

public:
  void setOnNewConnected(std::function<void(Connector*)> fun) {
    if (fun)
      on_new_connected_ = fun;
  }

  void setOnDissconnected(std::function<void(Connector*)> fun) {
    if (fun)
      on_new_connected_ = fun;
  }

private:
  void __on_connected(int fd);

private:
  std::unique_ptr<Channel<AcceptAdaptHandle>> accept_channel_;
  std::function<void(Connector*)> on_new_connected_;
  std::function<void(Connector*)> on_dissconnected_;
};

} // namespace ye
