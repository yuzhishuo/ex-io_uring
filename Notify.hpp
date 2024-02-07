#pragma once

#include "IListenAble.hpp"
#include <error.h>
#include <exception>
#include <mutex>
#include <spdlog/spdlog.h>
#include <string.h>
#include <sys/eventfd.h>
#include <system_error>
#include <unistd.h>

namespace ye {

class Notify : public IListenAble {
public:
  explicit Notify(bool trigger = false);

  void thrive() &noexcept;

public:
  int fd() const &noexcept override { return this->fd_; }

public:
  void extinguish() &noexcept;

  virtual ~Notify() noexcept;

private:
  int value_ = 1;
  std::mutex mut_;
  int fd_;
};

} // namespace ye
