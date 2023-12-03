#include "ConnectAdaptHandle.hpp"
#include "ThreadPool.hpp"

int main(int argc, char const *argv[]) {
  constexpr auto thread_nums = 1;
  ye::StaticThreadPool pool{thread_nums};
  ye::ConnectAdaptHandle adapt(pool.emiter(), "172.23.210.20");

  adapt.connect("172.23.210.20", 9901);

  while (1)
    sleep(100'000);
  return 0;
}
