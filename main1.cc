#include "ConnectAdapt.hpp"
#include "ThreadPool.hpp"

int main(int argc, char const *argv[]) {

  ye::StaticThreadPool pool{10};
  ye::ConnectAdapt adapt(pool.emiter(), "172.23.210.20");

  adapt.connect("172.23.210.20", 9901);

  while (1)
    sleep(100'000);
  return 0;
}
