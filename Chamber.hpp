#pragma once
#include "Accept.hpp"
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

enum class cb {
  kWrite,
};

template <typename T> auto Chamber(T &t) -> void;

template <> inline auto Chamber(Channel<Acceptor> &channel) -> void {
  instance<Emiter>()->dispatch().registerChannel(&channel);
}

inline auto Chamber(Channel<Connector> &channel, Buffer &&buf) -> void {
  auto fd = channel.fd();
  //  instance<Emiter>()->registerBuffer
  instance<BufferProxy>()->inject(&channel, std::move(buf));
}

template <> inline auto Chamber(Channel<Notify> &channel) -> void {
  thread_local uint64_t value = 1;
  auto fd = channel.fd();
  instance<Emiter>();
  std::printf("%ld\n", write(fd, (void *)&value, sizeof(value)));
}

} // namespace ye