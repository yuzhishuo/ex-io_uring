#ifndef NETMONITOR_H
#define NETMONITOR_H
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <set>
#include <string>
#include <string_view>
#include <vector>

enum class NICStatus : uint8_t { NEW, DEL };

enum class IPAttribute : uint8_t { NEW, DEL };

class NetMonitor {

public:
  inline bool addNic(std::string nic) {
    std::unique_lock lock(mu_);
    auto [_, res] = nic_.insert(nic);
    return res;
  }

private:
  std::set<std::string> nic_;
  std::mutex mu_;
  std::function<void(std::string_view, uint32_t, IPAttribute)> on_ip_changed_;
  std::function<void(std::string_view, NICStatus)> on_nic_changed_;
};

#endif // NETMONITOR_H