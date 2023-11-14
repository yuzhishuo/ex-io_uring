#include "BufferProxy.hpp"

#include "Emiter.hpp"
#include <spdlog/spdlog.h>
#include <utility>

namespace ye {
BufferProxy::BufferProxy(Emiter *Emiter) : current_index_{0}, Emiter_{Emiter} {}

uint8_t *BufferProxy::operator[](size_t i) { return in_use_buffers_[i]; }

uint8_t *BufferProxy::allocate(size_t length) {

  if (length == kInitialSize && un_use_buffers_.size() > 0) [[unlikely]] {
    auto node = un_use_buffers_.extract(un_use_buffers_.begin());

    auto res = in_use_buffers_.insert(std::move(node));
    assert(res.inserted);
    return res.position->second;
  } else if (length != kInitialSize and
             un_use_cutom_buffers.contains(length) > 0) {
    auto range = un_use_cutom_buffers.equal_range(length);
    if (range.first != un_use_cutom_buffers.end() and
        range.first != range.second) {
      auto it = range.first;
      auto buffer = it->second;
      uint64_t index = *(reinterpret_cast<uint64_t *>(buffer) - 1);
      auto res = in_use_buffers_.insert(std::make_pair(index, buffer));
      assert(res.second);
      return res.first->second;
    }
  }

  uint8_t *buffer_space = reinterpret_cast<uint8_t *>(
      tc_malloc(sizeof(Buffer::inner_buffer) + sizeof(size_t) + length));

  uint64_t *index = reinterpret_cast<uint64_t *>(buffer_space);

  *index = current_index_;

  buffer_space += sizeof(size_t) /*+ sizeof(Buffer::inner_buffer)*/;

  new (buffer_space) Buffer::inner_buffer{
      .next = nullptr,
      .prev = nullptr,
      .buf =
          reinterpret_cast<char *>(buffer_space + sizeof(Buffer::inner_buffer)),
      .len = length,
  };

  in_use_buffers_.emplace_hint(
      in_use_buffers_.end(), current_index_++,
      reinterpret_cast<Buffer::inner_buffer *>(buffer_space));
  Emiter_->registerBuffer(
      reinterpret_cast<Buffer::inner_buffer *>(buffer_space));

  return buffer_space;
}

void BufferProxy::dallocate(Buffer::inner_buffer *buffer) {

  uint64_t index = *(reinterpret_cast<uint64_t *>(buffer) - 1);
  if (buffer->size() != kInitialSize) {
    un_use_cutom_buffers.insert(std::make_pair(index, buffer));
  }
  un_use_buffers_.insert(std::make_pair(index, buffer));
}

BufferProxy::~BufferProxy() {
  if (!in_use_buffers_.empty()) {
    spdlog::error("[Buffer]", "Buffer not freed");
  }

  std::for_each(std::begin(un_use_buffers_), std::end(un_use_buffers_),
                [](auto &pair) {
                  ::tc_free(pair.second);
                  pair.second = nullptr;
                });
}

void BufferProxy::inject(IChannel *channel, Buffer &&buffer) {
  auto fd = channel->fd();
  if (auto &write_list = delay_write_[fd]; !write_list.empty()) {
    write_list.push_back(new Buffer(std::move(buffer)));
    return;
  }
  auto buf = new Buffer(std::move(buffer));
  delay_write_[fd].push_front(buf);
  // TODO: register writer
  instance<Emiter>()->writeBuffer(channel, {buf});
}

void BufferProxy::trigger(IChannel *channel) {

  auto fd = channel->fd();
  auto &write_list = delay_write_[fd];
  if (!write_list.empty()) {
    // TODO: merge a func
    while (write_list.pop_front())
      ;
    write_list.replacement();
  }
  std::vector<Buffer *> bufs;
  write_list.foreach ([&](auto buf) { bufs.push_back(buf); });
  instance<Emiter>()->writeBuffer(channel, std::move(bufs));
}

void BufferProxy::warnup(IChannel *channel) {

  if (warmup_buffers_.contains(channel->fd())) {
    spdlog::warn("repeaction warnup");
    return;
  }
  auto bufer_raw = allocate();
  warmup_buffers_.insert_or_assign(channel->fd(), bufer_raw);

  // instance<Emiter>()->read()
}

} // namespace ye
