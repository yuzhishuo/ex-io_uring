
#include "Chamber.hpp"
#include "Accept.hpp"
#include "AcceptAdaptHandle.hpp"
#include "AcceptChannel.hpp"
#include "Buffer.hpp"
#include "Concept.hpp"
#include "Connector.hpp"
#include "ConntectorChannel.hpp"
#include "Dispatcher.hpp"
#include "Emiter.hpp"
#include "IChannelAdapter.hpp"
#include "NofityChannel.hpp"
#include <liburing.h>
#include <unistd.h>

namespace ye {

auto Chamber(Channel<AcceptAdaptHandle> &channel) -> void {
  instance<Emiter>()->dispatch().registerChannel(&channel);
}

auto Chamber(Channel<Connector> &channel, Buffer &&buf) -> void {
  auto fd = channel.fd();
  //  instance<Emiter>()->registerBuffer
  instance<BufferProxy>()->inject(&channel, std::move(buf));
}


auto Chamber(Channel<Notify> &channel) -> void {
  thread_local uint64_t value = 1;
  auto fd = channel.fd();
  instance<Emiter>();
  std::printf("%ld\n", write(fd, (void *)&value, sizeof(value)));
}

} // namespace ye