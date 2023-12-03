#include "Dispatcher.hpp"
#include "Chamber.hpp"
#include "Channel.hpp"
#include "ConnectAdapt.hpp"
#include "Connector.hpp"
#include "Dispatcher.hpp"
#include "NofityChannel.hpp"

namespace ye {

enum class tag : int { IN, OUT };
struct ChannelDataWapper {
  IChannel *channel;
  std::variant<uintptr_t, int, tag> data;
};

void Dispatcher::once() {
  assert(instance<Emiter>() == emiter_);
  if (!quit_) {
    struct io_uring_cqe *cqe;
    auto ret = io_uring_wait_cqes(&ring_, &cqe, 1, nullptr, nullptr);
    if (ret < 0) {
      if (ret == -4)
        return; // TODO: return?
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
        // auto connector_channel = dynamic_cast<Channel<Connector> *>(channel);
        // auto tag_ = std::get<tag>(wapper->data);
        // switch (tag_) {
        // case tag::IN:
        // case tag::OUT:
        // default:
        //   break;
        // }

        // if (cqe->res == 0)
        //   connector_channel->handleReadFinish(Buffer{});
        // if (cqe->res < 0)
        //   connector_channel->handleReadFinish(Buffer{});
        // if (cqe->res > 0) {
        //   auto bid = cqe->flags >> IORING_CQE_BUFFER_SHIFT;
        //   auto buffer = instance<BufferProxy>()->operator[](bid);
        //   connector_channel->handleReadFinish(Buffer{});
        // }

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
          spdlog::error("emmit error is {}", 0 - cqe->res);
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
        assert(false && "unreachable");
#endif
      }
      }
      i++;
    }

    io_uring_cq_advance(&ring_, i);

    // emiter_->runAt(std::bind(&Dispatcher::once, this));
  }
}

} // namespace ye