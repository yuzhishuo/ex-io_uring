#pragma once
#include "IChannelAdapter.hpp"
#include "IListenAble.hpp"
#include "InetAddress.hpp"
#include <functional>
#include <memory>

namespace ye {

class Connector : public IListenAble {
public:
  explicit Connector(int fd);

  virtual int fd() const &noexcept { return fd_; }

  inline auto setClose(std::function<void()> fun) noexcept {
    on_close_ = std::move(fun);
  }

private:
  int fd_;
  InetAddress ta_;
  std::shared_ptr<IChannel> channel_;
  std::function<void()> on_close_;
};
} // namespace ye
