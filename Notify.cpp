#include "Notify.hpp"

namespace ye {

inline static constexpr size_t error_buf_len = 128;
thread_local char error_buf[error_buf_len] = {0};

Notify::Notify(bool trigger) {
  // canel SEMAPHORE that avoid muti walkup
  fd_ =
      eventfd(trigger ? 1 : 0, EFD_NONBLOCK | EFD_CLOEXEC /* | EFD_SEMAPHORE*/);
  if (fd_ < 0) {
    strerror_r(errno, error_buf, error_buf_len);
    throw std::system_error{errno, std::system_category(), error_buf};
  }
}

void Notify::thrive() &noexcept {
  std::unique_lock<std::mutex> lc(mut_);
  if (value_ == 1) {
    if (auto res = write(fd_, &value_, sizeof(value_)); res < 0) {
      if (res != EAGAIN) {
        strerror_r(errno, error_buf, error_buf_len);
        spdlog::warn("[Notify]", "write error (may happen muti-thread write)",
                     error_buf);
      }
    }
  }
}

//   public:
//     int fd() const &noexcept override { return this->fd_; }

void Notify::extinguish() &noexcept {
  std::unique_lock<std::mutex> lc(mut_);
  if (auto res = read(fd_, (void *)&value_, sizeof(value_)); res < 0) {
    if (res != EAGAIN) {
      strerror_r(errno, error_buf, error_buf_len);
      spdlog::warn("[Notify]", "read error (may happen muti-thread read)",
                   error_buf);
    }
  }
}

Notify::~Notify() noexcept { close(fd_); }
} // namespace ye