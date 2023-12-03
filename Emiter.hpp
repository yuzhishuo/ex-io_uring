#pragma once

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

class Dispatcher;

template <typename T> T *instance();
template <> BufferProxy *instance();

class Emiter {
public:
  Emiter();
  ~Emiter();

  void start();
  bool isNormalStatus() const &noexcept;
  void runAt(std::function<void()> fun);
  inline auto &dispatch() { return *dispatch_; }

private:
  bool is_quit_;
  io_uring ring_;
  IChannel *notify_;
  std::unique_ptr<Dispatcher> dispatch_;
  std::mutex mtx_; // for runnable_
  std::vector<std::function<void()>> runnable_;
};

template <> Emiter *instance();
} // namespace ye
