#include "ConnectAdaptHandle.hpp"
#include "ConnectAdapt.hpp"
#include "ConntectorChannel.hpp"
#include "Dispatcher.hpp"
#include <memory>
namespace ye {

ConnectAdaptHandle::ConnectAdaptHandle(Emiter *emiter, std::string_view ip,
                                       uint16_t port)
    : emiter_{emiter} {

  adaptor_ =
      std::make_unique<Channel<ConnectAdaptHandle>>(InetAddress{ip, port});
}

void ConnectAdaptHandle::connect(const InetAddress &addr) {
  using namespace Socket;
  std::error_code ec;
  auto fd = createSocket<Proto::kTcp>(ec);
  bindSocket(fd, addr, ec);
  if (ec) {
    spdlog::error("[ConnectAdaptHandle] create connect fail, %s", ec.message());
    return;
  }

  // FIXME: if register success
  emiter_->dispatch().registerChannel(this, fd, addr);
  auto newconnector = adaptor_->createConnector(fd);
  newconnector->setClose([&]() { connectors_.erase(fd); });
  assert(!connectors_.contains(fd));
  new_connectors_.insert_or_assign(fd, newconnector);
}

} // namespace ye