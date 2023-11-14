#pragma once
#include "Concept.hpp"
#include "ConntectorChannel.hpp"
#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "InetAddress.hpp"
#include "Socket.hpp"

#include <map>
#include <string_view>

namespace ye {

struct ConnectAdaptHandle : public Handle {
public:
  explicit ConnectAdaptHandle(InetAddress addr) : addr_{addr} {}
  using connector_type = Connector *;
  connector_type createConnector(int fd) { return new Connector(fd); }
  inline auto getInetAddress() const { return addr_; }

public:
  InetAddress addr_;
};
static_assert(HandleConcept<ConnectAdaptHandle>);

template <> class Channel<ConnectAdaptHandle> : public IChannel {

public:
  Channel(Emiter *emiter, std::string_view ip, uint16_t port = 0)
      : IChannel{ChannelType::ConnectAdapt}, adaptor_{InetAddress{ip, port}},
        emiter_{emiter} {}

  inline void connect(std::string_view ip, uint16_t port = 0)
      __attribute__((always_inline)) {
    connect(InetAddress{ip, port});
  }

  void connect(InetAddress addr) {
    using namespace Socket;
    std::error_code ec;
    auto fd = createSocket<Proto::kTcp>(ec);
    bindSocket(fd, adaptor_.addr_, ec);
    if (ec) {
      spdlog::error("[Channel<ConnectAdapt>] create connect fail, %s",
                    ec.message());
      return;
    }
    auto newconnector = Channel<Connector>(adaptor_.createConnector(fd));
    newconnector.setColse(
        [&](Channel<Connector> connector) { connectors_.erase(fd); });
    assert(!connectors_.contains(fd));
    new_connectors_.insert_or_assign(fd, newconnector);
    emiter_->registerChannel(this, fd, addr);
  }

  int fd() const &noexcept override { return -1; }
  // TODO: remove
  void handleReadFinish(int fd, Buffer *, std::error_code ec) noexcept {
    if (ec) {
      if (ec == std::errc::operation_in_progress) {
        emiter_->writeable(this, fd);
        return;
      }
      spdlog::warn("[ConnectAdapt] connect fail");
      auto erase_num = connectors_.erase(fd);
      assert(erase_num > 0);
    }
    auto new_connect_node = connectors_.extract(fd);
    if (on_new_connect_) {
      auto ret = connectors_.insert(std::move(new_connect_node));
      on_new_connect_(ret.position->second);
    } else {
      using Socket::Close;
      if (on_close_)
        on_close_(adaptor_.getInetAddress(), fd);
      Close(*this);
    }
  };

  // virtual void handleWriteFinish(int res) {
  //   throw std::runtime_error("not implemented");
  // }

  inline void setNewConnect(std::function<void(Channel<Connector>)> func)
      __attribute__((always_inline)) {
    if (func)
      on_new_connect_ = std::move(func);
  }

  inline void setOnClose(std::function<void(InetAddress, int)> func)
      __attribute__((always_inline)) {
    if (func)
      on_close_ = std::move(func);
  }

private:
  // Timer   // TODO: support connect timeout ?
  std::map<int, Channel<Connector>> connectors_;
  std::map<int, Channel<Connector>> new_connectors_;
  std::function<void(Channel<Connector>)> on_new_connect_;
  std::function<void(InetAddress, int)> on_close_;
  ConnectAdaptHandle adaptor_;
  Emiter *emiter_;
};
static_assert(ConnectAdaptConcept<Channel<ConnectAdaptHandle>>);

using ConnectAdapt = Channel<ConnectAdaptHandle>;
} // namespace ye
