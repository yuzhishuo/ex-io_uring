#pragma once

#include "BufferProxy.hpp"
#include "Connector.hpp"
#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "IListenAble.hpp"
#include "IntrusiveList.hpp"
#include "RefCount.hpp"
#include "Timer.hpp"

#include <concepts>
#include <format>
#include <functional>
#include <memory>
#include <optional>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <system_error>
#include <utility>

#include "Common.hpp"

namespace ye {

template <>
class Channel<Connector>
    : public IChannel, public std::enable_shared_from_this<Channel<Connector>> {
public:
  Channel(IListenAble *listen, Emiter *emiter = nullptr,
          ConnectStatus status = ConnectStatus::NOTCONNECT)
      : IChannel{ChannelType::Connect}, listenable_{listen}, connect_status_{
                                                                 status} {
    connected_ = true;
    emiter_ = emiter;
  }

  auto getEmiter() const noexcept { return emiter_; }
  void setEmiter(Emiter *emiter) noexcept {
    if (connected_) [[unlikely]] {
      assert(false && "shouldn't set Emiter When connected");
    }
    emiter_ = emiter;
  }

  /// listenable_ 其实不准确 我更需要的是 有状态的 可连接的
  inline auto setConnectStatus(ConnectStatus status) noexcept {
    // need convert table
    connect_status_ = status;
  }

  inline auto getConnectStatus() const noexcept -> ConnectStatus {
    return connect_status_;
  }

  ~Channel() noexcept {}

  int fd() const &noexcept override { return listenable_->fd(); }

private:
  friend Emiter;
  void handleError(std::error_code ec) {}

  void handleReadFinish(Buffer &&buf) noexcept {
    if (buf.readableBytes()) [[unlikely]] {
      connected_ = false;
      if (on_close_)
        on_close_();
      return;
    }
    if (on_read_ && connected_) {
      on_read_(*this, std::move(buf));
      if (listen_) {
        // continue read
        instance<BufferProxy>()->warnup(this);
      }
    }
  }

  void handleWriteFinish(int res) { instance<BufferProxy>()->trigger(this); }

public:
  void setListen(bool listen = true) &noexcept { listen_ = listen; }
  void
  setRead(std::function<bool(Channel<Connector>, Buffer &&)> &&func) &noexcept {
    if (func)
      on_read_ = std::move(func);
  }

  void setError(std::function<void(std::error_code)> &&func) &noexcept {
    if (func)
      on_error_ = std::move(func);
  }

  void setColse(std::function<void()> &&func) &noexcept {
    if (func)
      on_close_ = std::move(func);
  }

private:
  IListenAble *listenable_;
  ConnectStatus connect_status_;
  bool listen_;
  Emiter *emiter_;
  bool connected_;
  std::function<void(Channel<Connector>, Buffer &&)> on_read_;
  std::function<void(std::error_code)> on_error_;
  std::function<void()> on_close_;
  std::function<void(int res)> on_write_finish_;
};

} // namespace ye
