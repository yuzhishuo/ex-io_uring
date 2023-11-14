#include "AcceptChannelAsync.hpp"

namespace ye {
AcceptChannelAsync::AcceptChannelAsync(Emiter *Emiter, uint32_t ip,
                                       uint16_t port)
    : accept_{Emiter, ip, port} {}

AcceptChannelAsync::~AcceptChannelAsync() {}
} // namespace ye