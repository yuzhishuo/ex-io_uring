#include "AcceptChannel.hpp"
#include "AcceptAdaptHandle.hpp"

namespace ye {
AcceptAdaptHandle::AcceptAdaptHandle(Emiter *emiter, std::string_view ip,
                                     uint16_t port)
    : accept_channel_{std::make_unique<Channel<AcceptAdaptHandle>>(
          emiter, ye::InetAddress(ip, port))} {
  accept_channel_->setConnect([this](int fd) { __on_connected(fd); });
}
void AcceptAdaptHandle::__on_connected(int fd) {
  accept_channel_->createConnector(fd);
}
} // namespace ye