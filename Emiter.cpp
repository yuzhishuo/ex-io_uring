#include "Emiter.hpp"
#include "AcceptChannel.hpp"
// #include "BufferProxy.hpp"
#include "Chamber.hpp"
#include "Channel.hpp"
#include "ConnectAdapt.hpp"
#include "Connector.hpp"
#include "Dispatcher.hpp"
#include "NofityChannel.hpp"

#include <spdlog/spdlog.h>

namespace ye {

static std::once_flag __buffer_once;
static constinit BufferProxy *__buffer_proxy = nullptr;

template <> BufferProxy *instance() {
  if (nullptr == __buffer_proxy)
    spdlog::critical("[instance]"
                     "BufferProxy instance is nullptr");
  return __buffer_proxy;
}

thread_local static constinit Emiter *__emiter = nullptr;

template <> Emiter *instance() {
  if (nullptr == __buffer_proxy)
    spdlog::critical("[instance]"
                     "Emiter instance is nullptr");
  return __emiter;
}

Emiter::Emiter()
    : is_quit_{false}, notify_(nullptr),
      dispatch_(std::make_unique<Dispatcher>(this)) {
  struct io_uring_params params; 
  memset(&params, 0, sizeof(params));
  auto RING_LENGTH = 39;
  if (io_uring_queue_init_params(RING_LENGTH, &ring_, &params) < 0) {
    throw std::runtime_error("Failed to initialize io_uring queue");
  }
  std::call_once(__buffer_once,
                 [&]() { __buffer_proxy = new BufferProxy(this); });

  if (nullptr != __emiter) {
    spdlog::critical("[Emiter]"
                     "repeat create emiiter");
    return; // throw ??
  }
  __emiter = this;
  notify_ = new Channel<Notify>(this);
  dispatch_->registerChannel(notify_);
}

Emiter::~Emiter() { io_uring_queue_exit(&ring_); }

void Emiter::start() {
  while (!is_quit_) {

    decltype(runnable_) tasks;
    {
      std::unique_lock lc(mtx_);
      tasks.swap(runnable_);
    }

    try {
      for (auto &task : tasks)
        task();
    } catch (...) {
      spdlog::warn("[Emiter] "
                   "genneate exception");
    }

    dispatch_->once();
  }
}
bool Emiter::isNormalStatus() const &noexcept {
  return this == instance<Emiter>();
}
void Emiter::runAt(std::function<void()> fun) {
  if (isNormalStatus()) {
    fun();
    return;
  }
  std::unique_lock lc(mtx_);
  runnable_.emplace_back(std::move(fun));
  Chamber(dynamic_cast<Channel<Notify> &>(*notify_));
}
} // namespace ye
