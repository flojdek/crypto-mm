#ifndef PADDED_ATOMIC_H
#define PADDED_ATOMIC_H

#include <atomic>

namespace utils {
  constexpr size_t CacheLineSize = 128;

  template<typename T>
  struct alignas(CacheLineSize) PaddedAtomic {
    std::atomic<T> atomic;
    char padding[CacheLineSize - sizeof(std::atomic<T>)]; // Padding to ensure no false sharing
  };
}

#endif //PADDED_ATOMIC_H
