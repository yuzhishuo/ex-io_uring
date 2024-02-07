#pragma once

namespace ye {
class IListenAble {
public:
  virtual int fd() const &noexcept = 0;

public:
  virtual ~IListenAble() = default;
};

} // namespace ye
