#pragma once
#include "AcceptChannel.hpp"
#include <coroutine>

namespace ye {
class AcceptChannelAsync {
private:
  Channel<Acceptor> accept_;

public:
  AcceptChannelAsync(Emiter *Emiter, uint32_t ip, uint16_t port);
  ~AcceptChannelAsync();
};

} // namespace ye
