#include "AcceptAdaptHandle.hpp"
#include "IChannelAdapter.hpp"
#include "ThreadPool.hpp"
#include <sys/select.h>

int main(int argc, char **argv) {

  ye::StaticThreadPool pool(10);
  auto emiter = pool.emiter();

  pool.emiter()->runAt([]() { std::printf("hello word\n"); });

  pool.emiter()->runAt([]() { std::printf("hellow word2\n"); });

  ye::AcceptAdaptHandle adapt{emiter, "192.168.131.140", 9901};

  // accept_channel.setConnect([&](auto connect) {
  //   std::printf("%s\n", __func__);
  //   connect.setEmiter(emiter);

  //   ye::Buffer buf;
  //   buf.appendInt32(33);
  //   ye::Chamber(connect, std::move(buf));
  // });

  select(0, 0, 0, 0, 0);
  return 0;
}
