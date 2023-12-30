#include "Dispatcher.hpp"
#include "Buffer.hpp"
#include "BufferProxy.hpp"
#include "Chamber.hpp"
#include "Channel.hpp"
#include "ConnectAdapt.hpp"
#include "Connector.hpp"
#include "Dispatcher.hpp"
#include "NofityChannel.hpp"
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <ios>
#include <spdlog/spdlog.h>
#include <system_error>

#include "Common.hpp"

namespace ye {

void Dispatcher::once() {
  assert(instance<Emiter>() == emiter_);
  if (!quit_) {
    struct io_uring_cqe *cqe;
    auto ret = io_uring_wait_cqes(&ring_, &cqe, 1, nullptr, nullptr);
    if (ret < 0) {
      if (ret == -4)
        return; // TODO: return or continue?
      throw std::runtime_error("io_uring_wait_cqe error");
    }

    unsigned head;
    unsigned i = 0;

    io_uring_for_each_cqe(&ring_, head, cqe) {
      auto wapper =
          reinterpret_cast<ChannelDataWapper *>(io_uring_cqe_get_data(cqe));
      auto channel = reinterpret_cast<IChannel *>(wapper->channel);
      auto ud = wapper->ud;

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
        static_assert(std::is_base_of_v<IChannel, Channel<AcceptAdaptHandle>> &&
                      !std::is_same_v<Channel<AcceptAdaptHandle>, IChannel>);
        auto accept_channel =
            dynamic_cast<Channel<AcceptAdaptHandle> *>(channel);
        [[assume(nullptr != accept_channel)]];
        if (cqe->res > 0) {
          spdlog::info("[Listen] connect generate form {}", cqe->res);
          Buffer buf;
          buf.appendInt32(cqe->res);
          accept_channel->readable(buf);
        } else {
          spdlog::error("[Listen] emmit error is {} {}", 0 - cqe->res,
                        strerror(cqe->res));
          accept_channel->errorable(
              std::error_code(0 - cqe->res, std::system_category()));
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
        int fd = wapper->ud;
        if (cqe->res < 0) {
          if (cqe->res == -EINPROGRESS) {
            registerChannel(channel);
            break;
          } else {
          
            connect_adapt_channel->errorable(
                std::error_code(errno, std::system_category()));
            break;
          }
        }
        spdlog::info("the connect cqe result is {}", cqe->res);
        assert(cqe->res >= 0); //!!! ignore poll event type
        // std::error_code ec;
        // Socket::getSocketError(fd, ec);
        Buffer data;
        // if (ec) {
        //   spdlog::info("connect fail, beacuse {}", ec.message());
        //   data.appendInt32(-ud);
        //   connect_adapt_channel->readable(data);
        //   break;
        // }
        data.appendInt32(ud);
        connect_adapt_channel->readable(data);
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