#include "ConnectAdaptHandle.hpp"
#include "ThreadPool.hpp"

#include <spdlog/spdlog.h>
#include <sys/select.h>

#include <liburing.h>


int main(int argc, char const *argv[]) {

  io_uring_probe * probe = io_uring_get_probe();// IORING_OP_CONNECT
  if(io_uring_opcode_supported(probe, IORING_OP_CONNECT)) {
    spdlog::info("current support IORING_OP_CONNECT");
  }
  constexpr auto thread_nums = 1;
  ye::StaticThreadPool pool{thread_nums};
  ye::ConnectAdaptHandle adapt(pool.emiter(), "192.168.131.140");

  adapt.connect("192.168.131.140", 9901);
  adapt.setNewConnect([](auto &conn) { spdlog::info("new connect callback"); });
  select(0, NULL, NULL, NULL, NULL);
  return 0;
}
