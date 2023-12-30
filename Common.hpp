#pragma once

#include <cstdint>
enum class ConnectStatus : int8_t {
  NOTCONNECT,
  CONNECTTING,
  CONNECTTED,
  CONNECTFAIL,
};