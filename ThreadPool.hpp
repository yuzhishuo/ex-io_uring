#pragma once
#include "Emiter.hpp"
#include <condition_variable>
#include <format>
#include <memory>
#include <mutex>
#include <pthread.h>
#include <thread>
#include <vector>

namespace ye {

class StaticThreadPool {

public:
  StaticThreadPool(size_t num) : thread_nums_{num}, next_thread_{0} {
    trds_.reserve(num);
    emts_.reserve(num);
    for (int i = 0; i < num; i++) {
      int cond = 0;
      trds_.emplace_back([this, i, &cond]() {
        {
          Emiter *emit = nullptr;
          {
            std::unique_lock lck(mtx_);
            pthread_setname_np(pthread_self(),
                               std::format("thread_pool_{}", i).data());
            emit = &(*emts_.emplace_back(std::make_unique<Emiter>()));
            cond = 1;
          }
          cv_.notify_all();
          emit->start();
        }
      });
      std::unique_lock lck(mtx_);
      cv_.wait(lck, [&cond]() { return cond == 1; });
    }
  }

  inline auto emiter() { return emts_.at(getNextThread()).get(); }

private:
  size_t getNextThread() {
    auto ret = next_thread_;
    next_thread_ = ((next_thread_ + 1) % thread_nums_);
    return ret;
  }

private:
  std::condition_variable cv_;
  std::mutex mtx_;
  std::vector<std::unique_ptr<Emiter>> emts_;
  size_t thread_nums_;
  size_t next_thread_;
  std::vector<std::jthread> trds_;
};

template <> inline StaticThreadPool *instance() {
  static StaticThreadPool staticThreadPool(std::thread::hardware_concurrency());
  return &staticThreadPool;
}
} // namespace ye
