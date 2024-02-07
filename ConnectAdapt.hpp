#pragma once
#include "Buffer.hpp"
#include "Concept.hpp"
#include "Connector.hpp"
#include "ConntectorChannel.hpp"
#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "InetAddress.hpp"
#include "Socket.hpp"
#include <cstdlib>
#include <map>
#include <optional>
#include <spdlog/spdlog.h>
#include <string_view>
#include <system_error>
#include <utility>

namespace ye {

class StreamHandle {




private:
std::optional<Buffer> recv_;
std::optional<Buffer> send_;
};

class ConnectAdaptHandle;
template <> class Channel<ConnectAdaptHandle> : public IChannel {
public:
  explicit Channel(InetAddress addr)
      : IChannel(ChannelType::ConnectAdapt), addr_{addr} {}
  inline auto getInetAddress() const { return addr_; }
  int fd() const &noexcept { return -1; }

  inline virtual void readable(Buffer &data) {
    auto fd = data.readInt32();
    fd > 0 ? on_new_connect_(fd) : on_connect_fail_(-fd);
  }

  inline virtual void errorable(std::error_code ec) { std::unreachable(); }

  inline void setNewConnect(std::function<void(int)> fun) {
    if (fun)
      on_new_connect_ = fun;
  }

  inline void setConnectFail(std::function<void(int)> fun) {
    if (fun)
      on_connect_fail_ = fun;
  }
  using connector_type = Connector *;
  connector_type createConnector(int fd) { return new Connector(fd); }

private:
  Emiter *emiter_;
  InetAddress addr_;
  std::function<void(int)> on_new_connect_ = [](auto) {
    spdlog::warn("default connect success fun triger");
  };
  std::function<void(int)> on_connect_fail_ = [](auto) {
    spdlog::warn("default connect fail fun triger");
  };
};

// static_assert(ConnectAdaptConcept<Channel<ConnectAdaptHandle>>);

// using ConnectAdapt = Channel<ConnectAdaptHandle>;
} // namespace ye
