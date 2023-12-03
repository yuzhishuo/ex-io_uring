#pragma once
#include "Buffer.hpp"
#include "IListenAble.hpp"
#include "IntrusiveList.hpp"
#include "yendian.hpp"
#include <algorithm>
#include <array>
#include <assert.h>
#include <concepts>
#include <cstring>
#include <deque>
#include <exception>
#include <execution>
#include <functional>
#include <gperftools/tcmalloc.h>
#include <initializer_list>
#include <iterator>
#include <liburing.h>
#include <list>
#include <map>
#include <string_view>
#include <system_error>
#include <type_traits>

namespace ye {

// FIXME: BUFFER SIZE is n * blockSize
// FIXME: Embedding list struct
static const size_t kCheapPrepend = 0;
static constexpr size_t kInitialSize = 1024;
/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode

class Buffer {

public:
  struct inner_buffer {
    inner_buffer *next;
    inner_buffer *prev;
    char *buf;
    size_t len;
    char *begin() const { return buf; };
    char *end() const { return buf + len; }
    size_t size() const { return len; }
    char *data() const { return buf; }
  };

public:
  // write model
  explicit Buffer(size_t initialSize = kInitialSize);
  // read model
  explicit Buffer(std::initializer_list<Buffer::inner_buffer *> buffers);

  // implicit copy-ctor, move-ctor, dtor and assignment are fine
  // NOTE: implicit move-ctor is added in g++ 4.6

  // void swap(Buffer &rhs) {
  //   std::swap(data_, rhs.data_);
  //   std::swap(readerIndex_, rhs.readerIndex_);
  //   std::swap(writerIndex_, rhs.writerIndex_);
  // }

  auto size() const { return len_; }
  auto data() const { return buf_; }

  size_t readableBytes() const { return writerIndex_ - readerIndex_; }

  size_t writableBytes() const { return size() - writerIndex_; }

  size_t prependableBytes() const { return readerIndex_; }

  const char *peek() const { return begin() + readerIndex_; }

  const char *findCRLF() const {
    // FIXME: replace with memmem()?
    const char *crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  const char *findCRLF(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    // FIXME: replace with memmem()?
    const char *crlf = std::search(start, beginWrite(), kCRLF, kCRLF + 2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  const char *findEOL() const {
    const void *eol = memchr(peek(), '\n', readableBytes());
    return static_cast<const char *>(eol);
  }

  const char *findEOL(const char *start) const {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const void *eol = memchr(start, '\n', beginWrite() - start);
    return static_cast<const char *>(eol);
  }

  // retrieve returns void, to prevent
  // string str(retrieve(readableBytes()), readableBytes());
  // the evaluation of two functions are unspecified
  void retrieve(size_t len) {
    assert(len <= readableBytes());
    if (len < readableBytes()) {
      readerIndex_ += len;
    } else {
      retrieveAll();
    }
  }

  void retrieveUntil(const char *end) {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  void retrieveInt64() { retrieve(sizeof(int64_t)); }

  void retrieveInt32() { retrieve(sizeof(int32_t)); }

  void retrieveInt16() { retrieve(sizeof(int16_t)); }

  void retrieveInt8() { retrieve(sizeof(int8_t)); }

  void retrieveAll() {
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  void append(const std::string_view str) { append(str.data(), str.size()); }

  void append(const char * /*restrict*/ data, size_t len) {
    ensureWritableBytes(len);
    std::copy(data, data + len, beginWrite());
    hasWritten(len);
  }

  void append(const void * /*restrict*/ data, size_t len) {
    append(static_cast<const char *>(data), len);
  }

  void ensureWritableBytes(size_t len) {
    if (writableBytes() < len) {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  char *beginWrite() { return begin() + writerIndex_; }

  const char *beginWrite() const { return begin() + writerIndex_; }

  void hasWritten(size_t len) {
    assert(len <= writableBytes());
    writerIndex_ += len;
  }

  void unwrite(size_t len) {
    assert(len <= readableBytes());
    writerIndex_ -= len;
  }

  ///
  /// Append int64_t using network endian
  ///
  void appendInt64(int64_t x) {
    int64_t be64 = endian::host2network(x);
    append(&be64, sizeof be64);
  }

  ///
  /// Append int32_t using network endian
  ///
  void appendInt32(int32_t x) {
    int32_t be32 = endian::host2network(x);
    append(&be32, sizeof be32);
  }

  void appendInt16(int16_t x) {
    int16_t be16 = endian::host2network(x);
    append(&be16, sizeof be16);
  }

  void appendInt8(int8_t x) { append(&x, sizeof x); }

  ///
  /// Read int64_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int64_t readInt64() {
    int64_t result = peekInt64();
    retrieveInt64();
    return result;
  }

  ///
  /// Read int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int32_t readInt32() {
    int32_t result = peekInt32();
    retrieveInt32();
    return result;
  }

  int16_t readInt16() {
    int16_t result = peekInt16();
    retrieveInt16();
    return result;
  }

  int8_t readInt8() {
    int8_t result = peekInt8();
    retrieveInt8();
    return result;
  }

  ///
  /// Peek int64_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int64_t)
  int64_t peekInt64() const {
    assert(readableBytes() >= sizeof(int64_t));
    int64_t be64 = 0;
    ::memcpy(&be64, peek(), sizeof be64);
    return endian::network2host(be64);
    return 0;
  }

  ///
  /// Peek int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int32_t peekInt32() const {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, peek(), sizeof be32);
    return endian::network2host(be32);
    return 0;
  }

  int16_t peekInt16() const {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, peek(), sizeof be16);
    return endian::network2host(be16);
    return 0;
  }

  int8_t peekInt8() const {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();
    return x;
  }

  ///
  /// Prepend int64_t using network endian
  ///
  void prependInt64(int64_t x) {
    int64_t be64 = endian::host2network(x);
    prepend(&be64, sizeof be64);
  }

  ///
  /// Prepend int32_t using network endian
  ///
  void prependInt32(int32_t x) {
    int32_t be32 = endian::host2network(x);
    prepend(&be32, sizeof be32);
  }

  void prependInt16(int16_t x) {
    int16_t be16 = endian::host2network(x);
    prepend(&be16, sizeof be16);
  }

  void prependInt8(int8_t x) { prepend(&x, sizeof x); }

  void prepend(const void * /*restrict*/ data, size_t len) {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char *d = static_cast<const char *>(data);
    std::copy(d, d + len, begin() + readerIndex_);
  }

  size_t internalCapacity() const { return size(); }

  void makeSpace(size_t len) {}

private:
  char *begin() const { return buf_; };
  char *end() const { return buf_ + len_; }

private:
  // IntrusiveList<inner_buffer, &inner_buffer::next, &inner_buffer::prev>
  //     data_list_;
  char *buf_;
  size_t writerIndex_;
  size_t len_;
  size_t readerIndex_;

  Buffer *next;
  Buffer *perv;

  static const char kCRLF[];
  friend IntrusiveList<Buffer, &Buffer::next, &Buffer::perv>;

private:
  void __static_assert() {
    static_assert(offsetof(iovec, iov_base) == offsetof(Buffer, buf_) and
                  offsetof(iovec, iov_len) == offsetof(Buffer, writerIndex_) and
                  sizeof(iovec::iov_base) == sizeof(Buffer::buf_) and
                  sizeof(iovec::iov_len) == sizeof(Buffer::writerIndex_));
  }

public:
  using BufferList = IntrusiveList<Buffer, &Buffer::next, &Buffer::perv>;
};
static_assert(std::is_standard_layout_v<Buffer> and
              // std::is_trivially_copy_assignable_v<Buffer> and
              // std::is_trivially_copyable_v<Buffer> and
              // std::is_trivially_move_constructible_v<Buffer> and
              // std::is_trivially_destructible_v<Buffer> and
              not std::has_virtual_destructor_v<Buffer>);

} // namespace ye