#pragma once

#include "Accept.hpp"
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

namespace ye {

template <> class Channel<Connector> : public IChannel {
public:
  Channel(Connector *conn, Emiter *emiter = nullptr)
      : IChannel{ChannelType::Connect} {
    inner_->connected_ = true;
    inner_ = new inner_wraper{};
    inner_->emiter_ = emiter;
    inner_->conn_ = conn;
    inner_->ref_count_.setFree(std::bind(&Channel::inner_free, this));
    inner_->ref_count_.Retain();
  }

  Channel(Channel &conn) : IChannel{ChannelType::Connect} {
    if (this == &conn)
      return;
    inner_ = conn.inner_;
    inner_->ref_count_.Retain();
  }

  Channel(Channel &&conn) : IChannel{ChannelType::Connect} {
    inner_ = conn.inner_;
    conn.inner_ = nullptr;
  }

  Channel &operator=(Channel &conn) {
    if (this == &conn)
      return *this;
    inner_ = conn.inner_;
    inner_->ref_count_.Retain();
    return *this;
  }

  Channel &operator=(Channel &&conn) {
    inner_ = conn.inner_;
    conn.inner_ = nullptr;
    return *this;
  }

  auto getEmiter() const noexcept { return inner_->emiter_; }
  void setEmiter(Emiter *emiter) noexcept {
    if (inner_->connected_) [[unlikely]] {
      assert(false && "shouldn't set Emiter When connected");
    }
    inner_->emiter_ = emiter;
  }

  ~Channel() noexcept {
    inner_->ref_count_.Release();
    if (inner_) {
      delete inner_;
    }
    inner_ = nullptr;
  }

  int fd() const &noexcept override { return inner_->conn_->fd(); }

private:
  void inner_free() { spdlog::error("Channel<Connector> free"); }

private:
  friend Emiter;
  void handleError(std::error_code ec) {}

  void handleReadFinish(Buffer &&buf) noexcept {
    if (buf.readableBytes()) [[unlikely]] {
      inner_->connected_ = false;
      if (inner_->on_close_)
        inner_->on_close_(*this);
      return;
    }
    if (inner_->on_read_ && inner_->connected_) {
      inner_->on_read_(*this, std::move(buf));
      if (inner_->listen_) {
        // continue read
        instance<BufferProxy>()->warnup(this);
      }
    }
  }

  void handleWriteFinish(int res) { instance<BufferProxy>()->trigger(this); }

public:
  void setListen(bool listen = true) &noexcept { inner_->listen_ = listen; }
  void
  setRead(std::function<bool(Channel<Connector>, Buffer &&)> &&func) &noexcept {
    if (func)
      inner_->on_read_ = std::move(func);
  }

  void setError(std::function<void(std::error_code)> &&func) &noexcept {
    if (func)
      inner_->on_error_ = std::move(func);
  }

  void setColse(std::function<void(Channel<Connector>)> &&func) &noexcept {
    if (func)
      inner_->on_close_ = std::move(func);
  }

private:
  struct inner_wraper {
    std::function<void(Channel<Connector> F, Buffer &&)> on_read_;
    std::function<void(std::error_code)> on_error_;
    std::function<void(Channel<Connector>)> on_close_;
    std::function<void(int res)> on_write_finish_;
    Connector *conn_;
    bool listen_;
    Emiter *emiter_;
    bool connected_; // TODO: need it ?
    ThreadSafeRefCountedBase ref_count_;
  };

  inner_wraper *inner_;
};

} // namespace ye
