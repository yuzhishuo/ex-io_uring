#pragma once
#include "Buffer.hpp"
#include "Emiter.hpp"
#include <cassert>
#include <condition_variable>
#include <cstdint>
#include <error.h>
#include <liburing.h>
#include <mutex>
#include <spdlog/spdlog.h>
#include <stdlib.h>
#include <string.h>
#include <string_view>
#include <strings.h>
#include <sys/poll.h>

#include "Concept.hpp"
#include "IChannelAdapter.hpp"

namespace ye {

enum packaged_task_type {
  TASK_TYPE_CONNECT,
};
typedef struct packaged_task {
  int task_id;
  int task_type;
  void *data;
} packaged_task_t;

enum class tag : int { IN = 0, OUT = 1, OUT_FIN = OUT << 1 };
struct ChannelDataWapper {
  IChannel *channel;
  std::variant<uintptr_t, int, tag> data;
  intptr_t ud;
};

class Dispatcher {
private:
  io_uring ring_;
  Emiter *emiter_;
  std::condition_variable cv_;
  std::mutex mtx_;
  bool quit_;

public:
  Dispatcher(Emiter *emiter) : emiter_(emiter), quit_{false} {
    constexpr auto RING_LENGTH = 64; // FIXME: need measure?
    struct io_uring_params params;
    memset(&params, 0, sizeof(params));
    if (auto ret = io_uring_queue_init_params(RING_LENGTH, &ring_, &params);
        ret < 0) {
      fprintf(stderr,
              "queue_init failed: %s\n"
              "NB: This requires a kernel version >= 6.0\n",
              strerror(-ret));
      return;
    }
    // emiter_->runAt(std::bind(&Dispatcher::_once, this));
  }

  template <ListenAbleConcept T> inline void registerChannel(T *t) {

    if (emiter_ != instance<Emiter>()) {

    retry:
      emiter_->runAt([=, this]() { registerChannel(t); });
      return;
    }
    bool allocated_sqe = false;
    auto fd = t->fd();
  correction:
    auto sqe = io_uring_get_sqe(&ring_);
    if (!sqe) // FIXME: 如果申请不出来了是不是应该 将其 挂载 runable 等 sqe
              // 唤醒了再去 申请加入
    {
      if (io_uring_sq_ready(&ring_) > 0 && allocated_sqe == false) {
        io_uring_submit(&ring_);
        allocated_sqe = true;
        goto correction;
      }
      goto retry;

      spdlog::error("[Emiter] get submit sqe obj fail");
      return;
    }
    auto wapper = new ChannelDataWapper{
        .channel = t,
    };
    io_uring_sqe_set_data(sqe, wapper);
    io_uring_prep_poll_add(sqe, fd, POLLIN);
    auto f = io_uring_submit(&ring_); // 线程最后一起提交？
    spdlog::info("{}", f);
  }

  template <ConnectAdaptConcept T>
  inline void registerChannel(Channel<T> *t, int fd, const InetAddress &addr) {

    spdlog::info("register connect channel, ta: {}", addr.toIpPort());
    if (emiter_ != instance<Emiter>()) {
      emiter_->runAt([=, this]() { registerChannel(t, fd, addr); });
      return;
    }
    // 如果申请失败那么是不是可以抛到最后来等待所有的处理完在进行注册
    auto sqe = io_uring_get_sqe(&ring_);

    if (!sqe) // FIXME: 如果申请不出来了是不是应该 将其 挂载 runable 等 sqe
              // 唤醒了再去 申请加入
    {
      spdlog::error("[Emiter] get submit sqe obj fail");
      return;
    }
    auto wapper = new ChannelDataWapper{
        .channel = t,
        .ud = fd,
    };

    io_uring_prep_connect(sqe, fd, addr.getSockAddr(), sizeof(struct sockaddr));
    // io_uring_prep_connect(sqe, fd, (struct sockaddr*)&server_address,
    // sizeof(struct sockaddr));

    io_uring_sqe_set_data(sqe, wapper);
    auto ret = io_uring_submit(&ring_);
  }

  template <AcceptorConcept T>
  inline void registerChannel(Channel<T> *t, bool multishot = true,
                              bool fixed = false) {

    assert(t->isAccpetable());
    auto sqe = io_uring_get_sqe(&ring_);
    if (!sqe)
      spdlog::error("[Emiter]"
                    "get submit sqe obj fail");
    io_uring_prep_accept(sqe, t->fd(), (struct sockaddr *)nullptr, nullptr,
                         SOCK_NONBLOCK);
    auto wapper = new ChannelDataWapper{
        .channel = t,
    };
    io_uring_sqe_set_data(sqe, wapper);
    auto ret = io_uring_submit(&ring_);
    spdlog::info("[Emiter] register channel fd:{} ret:{}", t->fd(), ret);
  }

  inline auto writeBuffer(IChannel *connector, std::vector<Buffer *> buffers) {
    assert(connector->type() == ChannelType::Connect);
    auto sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_writev(sqe, connector->fd(),
                         reinterpret_cast<struct iovec *>(buffers.data()),
                         buffers.size(), 0);
    io_uring_sqe_set_data(sqe, NULL);
    auto f = io_uring_submit(&ring_);
  }

  inline auto inject(IChannel *channel, std::string_view str) {
    assert(channel->type() == ChannelType::Connect);
    auto sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_send(sqe, channel->fd(), str.data(), str.length(), 0);
    io_uring_sqe_set_data(sqe, NULL);
    auto f = io_uring_submit(&ring_);
  }

  inline auto inject(IChannel *channel, std::span<char> str) {
    assert(channel->type() == ChannelType::Connect);
    auto sqe = io_uring_get_sqe(&ring_);
    io_uring_prep_send(sqe, channel->fd(), str.data(), str.size(), 0);
    auto wapper = new ChannelDataWapper{
        .channel = channel,
        .data = tag::OUT,
    };
    io_uring_sqe_set_data(sqe, NULL);
    auto f = io_uring_submit(&ring_);
  }

  void once();
  ~Dispatcher() {
    std::unique_lock lock(mtx_);
    emiter_->runAt([&]() {
      io_uring_queue_exit(&ring_);
      quit_ = true;
      cv_.notify_all();
    });
    cv_.wait(lock, [&]() { return quit_; });
  }

private:
};

} // namespace ye