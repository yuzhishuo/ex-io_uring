#pragma once
#include "IChannelAdapter.hpp"

namespace ye {
class Connector {
public:
  explicit Connector(int fd) : fd_{fd} {}
  explicit Connector(IChannel *channel, int fd) : Connector{fd} {}
  inline int fd() const &noexcept { return fd_; }

private:
  int fd_;
  IChannel *channel_;
};
} // namespace ye
