#pragma once

#include <assert.h>
#include <functional>
#include <utility>

namespace ye {

template <typename T, T *T::*Next, T *T::*Prev> class IntrusiveList {
public:
  IntrusiveList() noexcept : head_(nullptr), tail_(nullptr) {}

  IntrusiveList(IntrusiveList &&other) noexcept
      : head_(std::exchange(other.head_, nullptr)),
        tail_(std::exchange(other.tail_, nullptr)) {}

  ~IntrusiveList() { assert(empty()); }

  IntrusiveList &operator=(const IntrusiveList &) = delete;
  IntrusiveList &operator=(IntrusiveList &&) = delete;

  [[nodiscard]] bool empty() const noexcept {
    return head_ == nullptr or tail_ == nullptr;
  }

  void swap(IntrusiveList &other) noexcept {
    std::swap(head_, other.head_);
    std::swap(tail_, other.tail_);
  }

  void foreach (std::function<void(T *)> func) {
    auto tmp = head_;
    while (tmp) {
      func(tmp);
      tmp = tmp->*Prev;
    }
  }

  void push_back(T *item) noexcept {
    item->*Prev = nullptr;
    item->*Next = tail_;
    if (tail_ == nullptr) {
      tail_ = item;
    } else {
      tail_->*Prev = item;
    }
  }

  void push_front(T *item) noexcept {
    item->*Prev = nullptr;
    item->*Next = head_;
    if (head_ == nullptr) {
      head_ = item;
    } else {
      head_->*Prev = item;
    }
  }

  [[nodiscard]] T *pop_front() noexcept {
    if (head_ == nullptr)
      return nullptr;

    T *item = head_;
    head_ = item->*Prev;
    if (head_ != nullptr) {
      head_->*Next = item->*Next;
    }
    return item;
  }

  [[nodiscard]] T *pop_back() noexcept {
    if (tail_ == nullptr)
      return nullptr;
    T *item = tail_;
    tail_ = item->*Prev;
    if (tail_ != nullptr) {
      tail_->*Next = item->*Next;
    }
    return item;
  }

  void replacement() { std::swap(head_, tail_); }

private:
  T *head_;
  T *tail_;
};
} // namespace ye
