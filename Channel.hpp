#pragma once
#include "Dispatcher.hpp"
#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "Timer.hpp"
#include <spdlog/spdlog.h>
#include <system_error>
namespace ye {

template <> class Channel<Timer> : public IChannel {

  static constexpr size_t buffer_len = 1;

public:
  Channel(Emiter *Emiter)
      : IChannel(ChannelType::Timer), Emiter_{Emiter}, buffer_{0} {
    Emiter->dispatch().registerChannel(this);
  }

  ~Channel() { /* Emiter_->unregisterChannel(this);*/
  }

  int fd() const &noexcept override { return timer_.fd(); }

  void handleReadFinish(int, Buffer *, std::error_code) noexcept {}
  void handleWriteFinish(int res) {}

private:
  constexpr static ChannelType type_ = ChannelType::Timer;
  Timer timer_;
  Emiter *Emiter_;
  uint8_t buffer_[buffer_len];
};

} // namespace ye
