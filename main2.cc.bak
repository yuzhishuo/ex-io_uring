#include "AcceptChannel.hpp"
#include "Chamber.hpp"
#include "Concept.hpp"
#include "ConnectAdapt.hpp"
#include "IChannelAdapter.hpp"
#include "ThreadPool.hpp"

int main(int argc, char **argv) {

  ye::StaticThreadPool pool(10);
  auto emiter = pool.emiter();

  pool.emiter()->runAt([]() { std::printf("hello word\n"); });

  pool.emiter()->runAt([]() { std::printf("hellow word2\n"); });

  ye::Channel<ye::Acceptor> accept_channel{emiter, "172.23.210.20", 9901};

  accept_channel.setConnect([&](auto connect) {
    std::printf("%s\n", __func__);
    connect.setEmiter(emiter);

    ye::Buffer buf;
    buf.appendInt32(33);
    ye::Chamber(connect, std::move(buf));
  });

  while (1) {
    sleep(300);
  };
  return 0;
}
