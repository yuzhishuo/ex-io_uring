#include "ConnectAdaptHandle.hpp"
#include "ConnectAdapt.hpp"
#include "ConntectorChannel.hpp"
#include "Dispatcher.hpp"
#include "Socket.hpp"
#include <memory>

namespace ye {

ConnectAdaptHandle::ConnectAdaptHandle(Emiter *emiter, std::string_view ip,
                                       uint16_t port)
    : emiter_{emiter}, sa_(ip, port) {

  adaptor_ =
      std::make_unique<Channel<ConnectAdaptHandle>>(InetAddress{ip, port});
  adaptor_->setNewConnect([this](auto fd) {
    assert(connectors_.contains(fd));
    on_new_connect_(*connectors_.at(fd));
  });
}

y_return_code ConnectAdaptHandle::connect(const InetAddress &addr) {
  using namespace Socket;
  std::error_code ec;
  auto fd = createSocket<Proto::kTcp>(true, ec);
  // 2357438656
  bindSocket(fd, sa_, ec);
  if (ec) {
    spdlog::error("[ConnectAdaptHandle] create connect fail, {}", ec.message());
    return Y_RETURN_CODE_NOK;
  }
  setReuseAddr(fd);
  setReusePort(fd);
  // FIXME: if register success
  auto new_connector = adaptor_->createConnector(fd);
  new_connector->setClose([&]() { connectors_.erase(fd); });
  assert(!connectors_.contains(fd));
  connectors_.insert_or_assign(fd, new_connector);
  emiter_->dispatch().registerChannel(adaptor_.get(), fd, addr);
  return Y_RETURN_CODE_OK;
}

} // namespace ye