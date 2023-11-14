#pragma once
#include <assert.h>
#include <atomic>
#include <functional>

namespace ye {

class ThreadSafeRefCountedBase {
private:
  mutable std::atomic<int> RefCount;
  std::function<void()> func_;

public:
  ThreadSafeRefCountedBase() : RefCount(0) {}
  void setFree(std::function<void()> func) { func_ = std::move(func); }

public:
  void Retain() const { RefCount.fetch_add(1, std::memory_order_relaxed); }

  void Release() const {
    int NewRefCount = RefCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
    assert(NewRefCount >= 0 && "Reference count was already zero.");
    if (NewRefCount == 0 and func_)
      func_();
  }
};

} // namespace ye
