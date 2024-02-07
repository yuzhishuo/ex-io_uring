#include <bits/confname.h>
#include <cstdlib>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
  const size_t pageSize = sysconf(_SC_PAGESIZE); // 获取系统页大小
  const size_t totalSize = 4098;                 // 分配的总内存大小
  const size_t alignedSize =
      (totalSize + pageSize - 1) & ~(pageSize - 1); // 对齐到页边界的大小

  void *ptr;
  if (posix_memalign(&ptr, pageSize, alignedSize) != 0) {
    // 内存分配失败的处理
    return 1;
  }

  // ptr 指向分配的内存，大小为 alignedSize 字节

  // 从第二个字节地址开始进行页对齐
  void *alignedPtr = static_cast<char *>(ptr) + 1;
  void *alignedPagePtr = reinterpret_cast<void *>(
      (reinterpret_cast<uintptr_t>(alignedPtr) + pageSize - 1) &
      ~(pageSize - 1));

  // 使用 alignedPagePtr 开始的内存进行操作

  // 释放内存
  free(ptr);

  return 0;
}