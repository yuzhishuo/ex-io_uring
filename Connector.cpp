#include "Connector.hpp"
#include "ConntectorChannel.hpp"

namespace ye {

Connector::Connector(int fd) : fd_{fd} {

  auto conn = std::make_shared<Channel<Connector>>(this);

  // register conn
  channel_ = conn;
}
} // namespace ye