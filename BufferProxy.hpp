#pragma once
#include "Buffer.hpp"

#include "IChannelAdapter.hpp"

namespace ye {
class Emiter;
class BufferProxy {

public:
  explicit BufferProxy(Emiter *Emiter);

  uint8_t *operator[](size_t i);

  uint8_t *allocate(size_t length = 4096);

  void dallocate(uint8_t *);
  void inject(IChannel *channel, Buffer &&buffer);
  void trigger(IChannel *channel);
  void warnup(IChannel *channel);
  ~BufferProxy();

private:
  std::map<size_t, uint8_t *> in_use_buffers_;
  std::map<size_t, uint8_t *> un_use_buffers_;
  std::map<ssize_t, Buffer::BufferList> delay_write_;
  std::map<int, uint8_t *> warmup_buffers_;
  std::multimap<size_t, Buffer::inner_buffer *> un_use_cutom_buffers;
  int current_index_;
  Emiter *Emiter_;
};
} // namespace ye
