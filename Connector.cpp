#include "Connector.hpp"
#include "ConntectorChannel.hpp"
#include <netinet/in.h>
#include <spdlog/spdlog.h>
#include <sys/socket.h>

namespace ye {

Connector::Connector(int fd)
    : fd_{fd}, ta_{[](int fd) {
        struct sockaddr_in saddr;
        socklen_t len = sizeof(struct sockaddr_in);
        getpeername(fd, (struct sockaddr *)&saddr, &len);
        return saddr;
      }(fd)} {

  spdlog::info("new connector created, {}", ta_.toIpPort());

  auto conn = std::make_shared<Channel<Connector>>(this);

  // register conn
  channel_ = conn;
}
} // namespace ye