#pragma once
#include "Buffer.hpp"
#include "IChannelAdapter.hpp"
#include "IListenAble.hpp"
#include "InetAddress.hpp"
#include <functional>
#include <memory>
#include <utility>

namespace ye {

class Connector : public IListenAble {
public:
  explicit Connector(int fd);

  virtual int fd() const &noexcept { return fd_; }

  inline auto setClose(std::function<void()> fun) noexcept {
    on_close_ = std::move(fun);
  }

  inline auto setOnMessage(std::function<void(Buffer)> fun) {
    if (!fun) {
      return;
    }
    on_message_ = fun;
  }

  auto Chamber(Buffer &&buf) -> void;

private:
  int fd_;
  InetAddress ta_;
  std::shared_ptr<IChannel> channel_;
  std::function<void()> on_close_;
  std::function<void(Buffer)> on_message_;
};
} // namespace ye
