#include <assert.h>
#include <atomic>
#include <functional>
#include <iostream>

/// A thread-safe version of \c RefCountedBase.
template <class Derived> class ThreadSafeRefCountedBase {
private:
  mutable std::atomic<int> RefCount;
  std::function<void()> func_;

public:
  ThreadSafeRefCountedBase(std::function<void()> func)
      : RefCount(0), func_(std::move(func)) {}

public:
  void Retain() const { RefCount.fetch_add(1, std::memory_order_relaxed); }

  void Release() const {
    int NewRefCount = RefCount.fetch_sub(1, std::memory_order_acq_rel) - 1;
    assert(NewRefCount >= 0 && "Reference count was already zero.");
    if (NewRefCount == 0)
      func_();
  }
};

template <typename T> struct IntrusiveRefCntPtrInfo {
  static void retain(T *obj) { obj->Retain(); }
  static void release(T *obj) { obj->Release(); }
};

class IntrusiveRefCntPtr {
  ThreadSafeRefCountedBase<IntrusiveRefCntPtr> *obj_;
  char *buf_;

public:
  IntrusiveRefCntPtr(char *str)
      : obj_(new ThreadSafeRefCountedBase<IntrusiveRefCntPtr>{[&]() {
          if (nullptr != buf_) {
            std::printf("buf release");
            delete buf_;
            buf_ = nullptr;
          }
        }}),
        buf_{str} {
    retain();
  }
  IntrusiveRefCntPtr(const IntrusiveRefCntPtr &S) : obj_(S.obj_), buf_(S.buf_) {
    retain();
  }
  IntrusiveRefCntPtr(IntrusiveRefCntPtr &&S) : obj_(S.obj_), buf_(S.buf_) {
    S.obj_ = nullptr;
    S.buf_ = nullptr;
  }

  ~IntrusiveRefCntPtr() { release(); }

  IntrusiveRefCntPtr &operator=(IntrusiveRefCntPtr S) {
    swap(S);
    return *this;
  }

  void swap(IntrusiveRefCntPtr &other) {
    ThreadSafeRefCountedBase<IntrusiveRefCntPtr> *tmp = other.obj_;
    other.obj_ = obj_;
    obj_ = tmp;
  }

  void reset() {
    release();
    obj_ = nullptr;
  }

  void resetWithoutRelease() { obj_ = nullptr; }

private:
  void retain() {
    if (obj_)
      obj_->Retain();
  }

  void release() {
    if (obj_)
      obj_->Release();
  }
};

int main(int argc, char **) {
  IntrusiveRefCntPtr t(new char[]{"123213213123123"});
  return 0;
}
