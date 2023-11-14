#include "Buffer.hpp"
#include "BufferProxy.hpp"
#include "Emiter.hpp"

namespace ye {
Buffer::Buffer(size_t initialSize)
    : readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend) {
  buf_ = instance<BufferProxy>()->allocate(1024);
  // data_list_.push_back(data_);
  // assert(readableBytes() == 0);
  // assert(writableBytes() == initialSize);
  // assert(prependableBytes() == kCheapPrepend);
}

// read model
Buffer::Buffer(std::initializer_list<Buffer::inner_buffer *> buffers) {
  // for (auto buffer : buffers) {
  //   data_ = instance<BufferProxy>()->allocate(1024);
  //   data_list_.push_back(data_);
  // }
}
} // namespace ye
