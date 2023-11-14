#pragma once
#include "IChannelAdapter.hpp"
#include "IListenAble.hpp"
#include "InetAddress.hpp"

#include <liburing.h>
#include <memory>
#include <system_error>
#include <type_traits>

namespace ye {
template <typename T>
concept HandleReadEvent = requires(T t, IListenAble &listener) {
  { t.handleReadEvent(listener) } -> std::same_as<void>;
};

template <typename T>
concept HandleWriteEvent = requires(T t) {
  { t.handleWriteEvent() } -> std::same_as<void>;
};

template <typename T>
concept HalfDuplexConcept = HandleReadEvent<T>;

template <typename T>
concept FullDuplexConcept = HandleReadEvent<T> && HandleWriteEvent<T>;

template <typename T>
concept SharedPtrConcept =
    std::is_convertible_v<T, std::shared_ptr<typename T::element_type>>;

template <typename T>
concept ListenAbleConcept = requires(T t) {
  { t.fd() } -> std::same_as<int>;
}
or std::is_base_of_v<IListenAble, T>;

template <typename T>
concept ChannelConcept = std::is_base_of_v<IChannel, T>;

template <typename T>
concept AcceptorConcept = requires(T t, int fd) {
  std::is_pointer_v<decltype(t.createConnector(fd))>;
};

template <typename T>
concept ConnectAdaptConcept = requires(T t) {
  {t.connect(InetAddress{"", 0})};
};

/*and FullDuplexConcept<
    typename std::remove_pointer<typename T::connector_type>::type>
    and ListenAbleConcept<T>;*/

class Handle {};
namespace {
[[maybe_unused]] extern struct : public Handle { int a_; } t;
static_assert(sizeof t == sizeof t.a_); // no EBO
static_assert(std::is_standard_layout_v<decltype(t)>, "warn");
}; // namespace

template <typename T>
concept HandleConcept =
    std::is_base_of_v<Handle, T> and std::is_standard_layout_v<
        T> /* and ListenAbleConcept<T> and std::is_trivial_v<T>*/;

template <typename T>
concept SeqRegister = true;

} // namespace ye
