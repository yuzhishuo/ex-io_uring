#include "AcceptAdaptHandle.hpp"
#include "Buffer.hpp"
#include "Chamber.hpp"
#include "Connector.hpp"
#include "IChannelAdapter.hpp"
#include "ThreadPool.hpp"
#include <condition_variable>
#include <mutex>
#include <spdlog/spdlog.h>
#include <sys/select.h>
int main(int argc, char **argv) {

  ye::StaticThreadPool pool(10);
  auto emiter = pool.emiter();

  ye::Connector *con;
  std::mutex mu;
  std::condition_variable cv;

  pool.emiter()->runAt([]() { std::printf("hello word\n"); });

  pool.emiter()->runAt([]() { std::printf("hellow word2\n"); });

  ye::AcceptAdaptHandle adapt{emiter, "192.168.131.140", 9901};
  adapt.setOnNewConnected([&](auto *conn) {
    con = conn;
    conn->setClose([]() {});
    conn->setOnMessage([](ye::Buffer) {});
    spdlog::trace("new connect");
  });

  std::unique_lock lock(mu);
  cv.wait(lock);

  ye::Buffer buf;
  buf.appendInt32(13);
  con->Chamber(std::move(buf));
  select(0, 0, 0, 0, 0);
  return 0;
}
