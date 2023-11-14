#pragma once

#include "Buffer.hpp"
#include "BufferProxy.hpp"
#include "Concept.hpp"
#include "IChannelAdapter.hpp"
#include <assert.h>
#include <bitset>
#include <functional>
#include <gperftools/tcmalloc.h>
#include <liburing.h>
#include <liburing/io_uring.h>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdexcept>
#include <string.h>
#include <sys/poll.h>
#include <thread>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace ye {

constexpr auto RING_LENGTH = 32; // FIXME: need measure
// class BufferProxy;

template <typename T> T *instance();
template <> BufferProxy *instance();
class Emiter {
public:
  Emiter();
  ~Emiter();

  enum class tag : int { IN, OUT };
  struct ChannelDataWapper {
    IChannel *channel;
    std::variant<uintptr_t, int, tag> data;
  };

  inline auto writeBuffer(IChannel *connector, std::vector<Buffer *> buffers) {
    assert(connector->type() == ChannelType::Connect);
    auto sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_writev(sqe, connector->fd(),
                         reinterpret_cast<struct iovec *>(buffers.data()),
                         buffers.size(), 0);
    auto wapper = reinterpret_cast<ChannelDataWapper *>(
        tc_malloc(sizeof(ChannelDataWapper)));
    memset(wapper, 0, sizeof(ChannelDataWapper));
    wapper->channel = connector;
    wapper->data = tag::OUT;
    io_uring_sqe_set_data(sqe, wapper);
    auto f = io_uring_submit(&ring_);
  }

  inline auto registerBuffer(Buffer::inner_buffer *buffer) & {
    struct iovec vec[1];
    vec[0].iov_base = (void *)buffer->data();
    vec[0].iov_len = buffer->size();
    io_uring_register_buffers(&ring_, vec, 1);
  }
  auto unregisterBuffer(Buffer *buffer) {}

  template <ListenAbleConcept T> inline void registerChannel(T *t) {
    auto fd = t->fd();
    auto sqe = io_uring_get_sqe(&ring_);

    auto wapper = reinterpret_cast<ChannelDataWapper *>(
        tc_malloc(sizeof(ChannelDataWapper)));
    memset(wapper, 0, sizeof(ChannelDataWapper));
    wapper->channel = t;
    io_uring_sqe_set_data(sqe, wapper);
    io_uring_prep_poll_add(sqe, fd, POLLIN);
    auto f = io_uring_submit(&ring_);
    spdlog::info("{}", f);
  }

  template <ConnectAdaptConcept T>
  inline void registerChannel(T *t, int fd, InetAddress addr) {
    auto sqe = io_uring_get_sqe(&ring_);
    if (!sqe) // FIXME: 如果申请不出来了是不是应该 将其 挂载 runable 等 sqe
              // 唤醒了再去 申请加入
      spdlog::error("[Emiter] get submit sqe obj fail");
    io_uring_prep_connect(sqe, fd, addr.getSockAddr(), sizeof(sockaddr));
    auto wapper = reinterpret_cast<ChannelDataWapper *>(
        tc_malloc(sizeof(ChannelDataWapper)));
    memset(wapper, 0, sizeof(ChannelDataWapper));
    wapper->channel = t;
    wapper->data = fd;
    io_uring_sqe_set_data(sqe, wapper);
    auto ret = io_uring_submit(&ring_);
  }

  template <ConnectAdaptConcept T> inline void writeable(T *t, int fd) {
    struct io_uring_sqe *sqe = io_uring_get_sqe(&ring_);
    if (!sqe) {
      spdlog::error("[Emiter] unable to get sqe\n");
      return;
    }
    auto wapper = reinterpret_cast<ChannelDataWapper *>(
        tc_malloc(sizeof(ChannelDataWapper)));
    memset(wapper, 0, sizeof(ChannelDataWapper));
    wapper->channel = t;
    wapper->data = fd;
    io_uring_prep_poll_add(sqe, fd, POLLOUT | POLLHUP | POLLERR);
    io_uring_sqe_set_data(sqe, wapper);
    auto ret = io_uring_submit(&ring_);
  }

  template <AcceptorConcept T>
  void registerChannel(Channel<T> *t, bool multishot = true,
                       bool fixed = false) {
    auto sqe = io_uring_get_sqe(&ring_);
    if (!sqe)
      spdlog::error("[Emiter]"
                    "get submit sqe obj fail");
    auto i = 0;
    if (fixed) {
      for (; i < fix_set_.size(); i++)
        if (fix_set_[i] == false)
          break;
      if (i != fix_set_.size()) {
        io_uring_prep_multishot_accept_direct(sqe, t->fd(), NULL, NULL,
                                              i /* FIXME it*/);
      } else {
        io_uring_prep_multishot_accept(sqe, t->fd(), (struct sockaddr *)nullptr,
                                       nullptr, 0);
      }
    } else
      io_uring_prep_accept(sqe, t->fd(), (struct sockaddr *)nullptr, nullptr,
                           SOCK_NONBLOCK);

    auto wapper = reinterpret_cast<ChannelDataWapper *>(
        tc_malloc(sizeof(ChannelDataWapper)));
    memset(wapper, 0, sizeof(ChannelDataWapper));
    wapper->channel = t;
    if (fixed && fix_set_.size() != i) {
      wapper->data = i;
    }
    io_uring_sqe_set_data(sqe, wapper);
    auto ret = io_uring_submit(&ring_);
    spdlog::info("[Emiter] register channel fd:{} ret:{}", t->fd(), ret);
  }

  void start();

  bool isNormalStatus() const &noexcept;
  void runAt(std::function<void()> fun);

private:
  bool is_quit_;
  io_uring ring_;
  IChannel *notify_;
  std::bitset<RING_LENGTH> fix_set_;
  std::mutex mtx_; // for runnable_
  std::vector<std::function<void()>> runnable_;
};

template <> Emiter *instance();
} // namespace ye
