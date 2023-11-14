#pragma once
#include "IListenAble.hpp"
#include <chrono>
#include <sys/timerfd.h>
#include <unistd.h>

namespace ye {

class Timer : public IListenAble {
public:
  explicit Timer() : timerfd_(timerfd_create(CLOCK_MONOTONIC, 0)) {
    if (timerfd_ == -1) {
      throw std::runtime_error("[Timer] Failed to create timerfd");
    }
  }

  ~Timer() noexcept {
    if (timerfd_ != -1) {
      close(timerfd_);
    }
  }

  bool runat(const std::chrono::system_clock::time_point &time) const & {
    std::chrono::nanoseconds duration = time - std::chrono::system_clock::now();
    return setTimer(duration.count(), 0);
  }

  bool run(const std::chrono::nanoseconds &duration) const & {
    return setTimer(duration.count(), 0);
  }

  bool runevery(const std::chrono::nanoseconds &duration) const & {
    return setTimer(duration.count(), duration.count());
  }

  bool runat_ms(const std::chrono::system_clock::time_point &time) const & {
    std::chrono::milliseconds duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            time - std::chrono::system_clock::now());
    return setTimer(duration.count(), 0);
  }

  bool run_ms(const std::chrono::milliseconds &duration) const & {
    return setTimer(duration.count(), 0);
  }

  bool runevery_ms(const std::chrono::milliseconds &duration) const & {
    return setTimer(duration.count(), duration.count());
  }

  int fd() const &noexcept override { return timerfd_; }

private:
  bool setTimer(long long initial, long long interval) const {
    struct itimerspec timerSpec;
    timerSpec.it_value.tv_sec = initial / 1000000000;
    timerSpec.it_value.tv_nsec = initial % 1000000000;
    timerSpec.it_interval.tv_sec = interval / 1000000000;
    timerSpec.it_interval.tv_nsec = interval % 1000000000;

    if (timerfd_settime(timerfd_, 0, &timerSpec, nullptr) == -1) {
      throw std::runtime_error("[Timer] Failed to set timerfd");
    }

    return true;
  }

private:
  const int timerfd_;
};

} // namespace ye
