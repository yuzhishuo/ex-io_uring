#pragma once
#include "Concept.hpp"
#include "Connector.hpp"
#include "ConntectorChannel.hpp"
#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "InetAddress.hpp"
#include "Socket.hpp"
#include <map>
#include <string_view>

namespace ye {
class ConnectAdaptHandle;
template <> class Channel<ConnectAdaptHandle> : public IChannel {
public:
  explicit Channel(InetAddress addr)
      : IChannel(ChannelType::ConnectAdapt), addr_{addr} {}
  inline auto getInetAddress() const { return addr_; }
  int fd() const &noexcept { return -1; }

  inline void handleReadFinish(int fd, Buffer *, std::error_code ec) noexcept {
    if (ec) {
      if (ec == std::errc::operation_in_progress) {
        // emiter_->writeable(this, fd);
        return;
      }
      spdlog::warn("[ConnectAdapt] connect fail");
      // auto erase_num = connectors_.erase(fd);
      // assert(erase_num > 0);
    }
    // auto new_connect_node = connectors_.extract(fd);
    if (on_new_connect_) {
      // auto ret = connectors_.insert(std::move(new_connect_node));
      // on_new_connect_(ret.position->second);
      on_new_connect_(createConnector(fd));
    } else {
      using Socket::Close;
      if (on_close_)
        on_close_(getInetAddress(), fd);
    }
  };

  using connector_type = Connector *;
  connector_type createConnector(int fd) { return new Connector(fd); }

private:
private:
  Emiter *emiter_;
  InetAddress addr_;
  std::function<void(Channel<Connector> F, Buffer &&)> on_read_;
  std::function<void(std::error_code)> on_error_;
  std::function<void(InetAddress, int)> on_close_;
  std::function<void(Connector *)> on_new_connect_;
  std::function<void(int res)> on_write_finish_;
};

// static_assert(ConnectAdaptConcept<Channel<ConnectAdaptHandle>>);

// using ConnectAdapt = Channel<ConnectAdaptHandle>;
} // namespace ye
