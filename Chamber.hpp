#pragma once
#include "AcceptChannel.hpp"
#include "Buffer.hpp"
#include "Connector.hpp"
#include "ConntectorChannel.hpp"
#include "IChannelAdapter.hpp"
#include "NofityChannel.hpp"
#include <liburing.h>
#include <unistd.h>

namespace ye {

enum class cb {
  kWrite,
};

// template <typename T> auto Chamber(T &t) -> void;

auto Chamber(Channel<AcceptAdaptHandle> &channel) -> void;

auto Chamber(Channel<Connector> &channel, Buffer &&buf) -> void;

auto Chamber(Channel<Notify> &channel) -> void;

} // namespace ye