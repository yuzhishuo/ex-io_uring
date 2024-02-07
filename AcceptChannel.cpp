#include "AcceptChannel.hpp"
#include "AcceptAdaptHandle.hpp"
#include "Socket.hpp"
namespace ye {

Channel<AcceptAdaptHandle>::Channel(Emiter *emiter, const ye::InetAddress &addr)
    : IChannel(ChannelType::ListenSocket), fd_{0}, emiter_{emiter} {
  std::error_code ec;
  fd_ = Socket::createSocket<Socket::Proto::kTcp>(true, ec);
  Socket::bindSocket(fd_, addr, ec);

  Socket::setReuseAddr(fd_);
  Socket::setReusePort(fd_);

  Socket::listenSocket(fd_, 10, ec);
  spdlog::info("listen in ip:{}, port:{}, result:{}", addr.toIp(), addr.port(),
               ec.message());

  emiter_->dispatch().registerChannel(this); // 这个部分可以在 ICHannel 中实现
}
} // namespace ye