#pragma once
#include "Buffer.hpp"
#include "IChannelAdapter.hpp"

namespace ye {

enum class cb {
  kWrite,
};
class Notify;
class AcceptAdaptHandle;
class Connector;
template <> class Channel<Connector>;

auto Chamber(Channel<AcceptAdaptHandle> &channel) -> void;

auto Chamber(std::shared_ptr<IChannel> channel, Buffer &&buf) -> void;

auto Chamber(Channel<Notify> &channel) -> void;

} // namespace ye