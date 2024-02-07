#pragma once
#ifndef LULUYUZHI_UTILITY_H
#define LULUYUZHI_UTILITY_H

#include <arpa/inet.h>
#include <endian.h>

namespace ye::endian {

// the inline assembler code makes type blur,
// so we disable warnings for a while.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"

template <class T> inline T host2network(T host);

template <class T> inline T network2host(T host);

template <> inline uint64_t host2network(uint64_t host64) {
  return htobe64(host64);
}
template <> inline uint32_t host2network(uint32_t host32) {
  return htobe32(host32);
}
template <> inline uint16_t host2network(uint16_t host16) {
  return htobe16(host16);
}

template <> inline uint8_t host2network(uint8_t host16) { return (host16); }
template <> inline uint64_t network2host(uint64_t net64) {
  return be64toh(net64);
}
template <> inline uint32_t network2host(uint32_t net32) {
  return be32toh(net32);
}
template <> inline uint16_t network2host(uint16_t net16) {
  return be16toh(net16);
}
template <> inline uint8_t network2host(uint8_t net8) { return net8; }

template <> inline int64_t host2network(int64_t host64) {
  return htobe64(host64);
}
template <> inline int32_t host2network(int32_t host32) {
  return htobe32(host32);
}
template <> inline int16_t host2network(int16_t host16) {
  return htobe16(host16);
}

template <> inline int8_t host2network(int8_t host8) { return (host8); }

template <> inline int64_t network2host(int64_t net64) {
  return be64toh(net64);
}
template <> inline int32_t network2host(int32_t net32) {
  return be32toh(net32);
}
template <> inline int16_t network2host(int16_t net16) {
  return be16toh(net16);
}
template <> inline int8_t network2host(int8_t net8) { return net8; }

#pragma GCC diagnostic pop

} // namespace ye::endian

#endif
