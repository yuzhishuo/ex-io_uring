#pragma once

#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "Notify.hpp"

namespace ye {

template <> class Channel<Notify> : public IChannel {
public:
  Channel(Emiter *emiter) : IChannel{ChannelType::Event}, notify_{false} {
    emiter->registerChannel(this);
  }

  int fd() const &noexcept override { return notify_.fd(); };
  void handleRead(int fd, std::error_code) { notify_.extinguish(); }
  void handleReadFinish(int fd, Buffer *, std::error_code) noexcept {
    // notify_.extinguish();
  }
  void handleWriteFinish(int res) {
    throw std::runtime_error("not implemented");
  }

private:
  Notify notify_;
};

} // namespace ye