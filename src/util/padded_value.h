#pragma once

namespace util {
  constexpr size_t CacheLineSize = 128;

  template<typename T>
  struct alignas(CacheLineSize) PaddedValue {
    T value;
    char padding[CacheLineSize - sizeof(value)]; // Padding to ensure no false sharing
  };
}