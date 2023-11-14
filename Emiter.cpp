#include "Emiter.hpp"
#include "AcceptChannel.hpp"
// #include "BufferProxy.hpp"
#include "Chamber.hpp"
#include "Channel.hpp"
#include "ConnectAdapt.hpp"
#include "Connector.hpp"
#include "NofityChannel.hpp"
#include <spdlog/spdlog.h>

namespace ye {

static std::once_flag __buffer_once;
static constinit BufferProxy *__buffer_proxy = nullptr;

template <> BufferProxy *instance() {
  if (nullptr == __buffer_proxy)
    spdlog::critical("[instance]"
                     "BufferProxy instance is nullptr");
  return __buffer_proxy;
}

thread_local static constinit Emiter *__emiter = nullptr;

template <> Emiter *instance() {
  if (nullptr == __buffer_proxy)
    spdlog::critical("[instance]"
                     "Emiter instance is nullptr");
  return __emiter;
}

Emiter::Emiter() : is_quit_{false}, notify_(nullptr) {
  struct io_uring_params params;
  memset(&params, 0, sizeof(params));
  if (io_uring_queue_init_params(RING_LENGTH, &ring_, &params) < 0) {
    throw std::runtime_error("Failed to initialize io_uring queue");
  }
  std::call_once(__buffer_once,
                 [&]() { __buffer_proxy = new BufferProxy(this); });

  if (nullptr != __emiter) {
    spdlog::critical("[Emiter]"
                     "repeat create emiiter");
    return; // throw ??
  }
  __emiter = this;
  notify_ = new Channel<Notify>(this);
}

Emiter::~Emiter() { io_uring_queue_exit(&ring_); }

void Emiter::start() {

  while (!is_quit_ || io_uring_sq_space_left(&ring_) > 0) {
    struct io_uring_cqe *cqe;
    auto ret = io_uring_wait_cqes(&ring_, &cqe, 1, nullptr, nullptr);
    if (ret < 0) {
      if (ret == -4)
        continue;
      throw std::runtime_error("io_uring_wait_cqe error");
    }

    unsigned head;
    unsigned i = 0;

    io_uring_for_each_cqe(&ring_, head, cqe) {
      auto wapper =
          reinterpret_cast<ChannelDataWapper *>(io_uring_cqe_get_data(cqe));
      auto channel = reinterpret_cast<IChannel *>(wapper->channel);

      switch (channel->type()) {
      case ChannelType::Timer: {
        auto timer_channel = dynamic_cast<Channel<Timer> *>(channel);
        break;
      }
      case ChannelType::Connect: {
        // if (!(cqe->flags & IORING_CQE_F_BUFFER)) {
        //  spdlog::error("no buffer selected");
        //  continue;
        //}
        auto connector_channel = dynamic_cast<Channel<Connector> *>(channel);
        auto tag = std::get<Emiter::tag>(wapper->data);
        switch (tag) {
        case tag::IN:
        case tag::OUT:
        default:
          break;
        }

        if (cqe->res == 0)
          connector_channel->handleReadFinish(Buffer{});
        if (cqe->res < 0)
          connector_channel->handleReadFinish(Buffer{});
        if (cqe->res > 0) {
          auto bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
          auto buffer = instance<BufferProxy>()->operator[](bid);
          connector_channel->handleReadFinish(Buffer{});
        }

        break;
      }
      case ChannelType::ListenSocket: {
        // FIXME:
        static_assert(std::is_base_of_v<IChannel, Channel<Acceptor>> &&
                      !std::is_same_v<Channel<Acceptor>, IChannel>);
        auto accept_channel = dynamic_cast<Channel<Acceptor> *>(channel);
        [[assume(nullptr != accept_channel)]];
        if (cqe->res > 0)
          accept_channel->handleReadFinish(cqe->res, nullptr, {});
        else {
          spdlog::error("emmit error is {}", 0-cqe->res);
        }
        break;
      }
      case ChannelType::Event: {
        auto notify_channel = dynamic_cast<Channel<Notify> *>(channel);
        notify_channel->handleRead(0, {});
        break;
      }
      case ChannelType::ConnectAdapt: {
        auto connect_adapt_channel =
            dynamic_cast<Channel<ConnectAdaptHandle> *>(channel);
        [[assume(nullptr != connect_adapt_channel)]];
        auto fd = std::get<int>(wapper->data);
        if (cqe->res < 0) {
          if (cqe->res == -EINPROGRESS) {
            connect_adapt_channel->handleReadFinish(
                fd, nullptr,
                std::make_error_code(std::errc::operation_in_progress));
            continue;
          }
          connect_adapt_channel->handleReadFinish(
              fd, nullptr, std::error_code(-cqe->res, std::generic_category()));
        }
        spdlog::info("the connect cqe result is {}", cqe->res);
        if (cqe->res >= 0) { //!!! ignore poll event type
          std::error_code ec;
          Socket::getSocketError(fd, ec);
          if (ec) {
            connect_adapt_channel->handleReadFinish(fd, nullptr, ec);
            continue;
          }
          connect_adapt_channel->handleReadFinish(fd, nullptr, {});
        } else {
          std::error_code ec;
          Socket::getSocketError(fd, ec);
          spdlog::info("connect fail, beacuse {}", ec.message());
        }
        break;
      }
      case ChannelType::Unknow:
        [[fallthrough]];
      default: {
        spdlog::error("[Emiter]", "channel type Unreachable");

#ifdef __cpp_lib_unreachable
        std::unreachable();
#else
        assert(false);
#endif
      }
      }
      i++;
    }

    io_uring_cq_advance(&ring_, i);

    decltype(runnable_) tasks;
    {
      std::unique_lock lc(mtx_);
      tasks.swap(runnable_);
    }
    for (auto &task : tasks)
      task();
  }
}
bool Emiter::isNormalStatus() const &noexcept {
  return this == instance<Emiter>();
}
void Emiter::runAt(std::function<void()> fun) {
  if (not isNormalStatus()) {
    fun();
    return;
  }
  std::unique_lock lc(mtx_);
  runnable_.emplace_back(std::move(fun));
  Chamber(dynamic_cast<Channel<Notify> &>(*notify_));
}
} // namespace ye
