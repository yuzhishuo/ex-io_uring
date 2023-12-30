#pragma once

#include "Buffer.hpp"
#include "IListenAble.hpp"
#include <assert.h>
#include <cstdint>
#include <system_error>

namespace ye {
enum class ChannelType : uint8_t {
  Unknow = 0,
  Event = 1,
  ListenSocket = 2,
  ConnectAdapt = 3,
  Connect = 4,
  Timer = 5,
};

class Buffer;
class IChannel : public IListenAble /* need impl the Listenable ??  */ {

public:
  constexpr explicit IChannel(ChannelType type = ChannelType::Unknow)
      : type_{type} {
    assert(type != ChannelType::Unknow);
  }
  inline constexpr auto type() const &noexcept { return type_; }
  virtual ~IChannel() = default;
  inline virtual void readable(Buffer &data) {}
  inline virtual void errorable(const std::error_code &code) {}
  // virtual void handleReadFinish(int, Buffer *, std::error_code) noexcept
  // = 0; virtual void handleWriteFinish(int res) {
  //   throw std::runtime_error("not implemented");
  // }
  inline bool isAccpetable() const noexcept {
    return type_ == ChannelType::ListenSocket;
  }


private:
  const ChannelType type_;
};

template <typename T> class Channel; // FIXME: the handle Concept of T
} // namespace ye
