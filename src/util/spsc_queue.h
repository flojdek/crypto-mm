#pragma once

#include <vector>

#include "padded_value.h"

namespace util {
  template<typename T>
  class SpscQueue final {
  public:
    explicit SpscQueue(std::size_t maxElems) : store_(maxElems, T()) {
    }

    SpscQueue() = delete;
    SpscQueue(const SpscQueue &) = delete;
    SpscQueue(const SpscQueue &&) = delete;
    SpscQueue &operator=(const SpscQueue &) = delete;
    SpscQueue &operator=(const SpscQueue &&) = delete;

    auto push(const T &elem) noexcept {
      const auto idx = nextWriteIdx_.value.load(std::memory_order::relaxed);
      const auto next_idx = increment(idx);
      if (next_idx != nextReadIdx_.value.load(std::memory_order::acquire)) {
        store_[idx] = elem;
        nextWriteIdx_.value.store(next_idx, std::memory_order::release);
        return true;
      }
      return false;
    }

    auto pop(T &elem) noexcept {
      const auto idx = nextReadIdx_.value.load(std::memory_order::relaxed);
      if (idx == nextWriteIdx_.value.load(std::memory_order::acquire)) {
        return false;
      }
      elem = store_[idx];
      const auto next_idx = increment(idx);
      nextReadIdx_.value.store(next_idx, std::memory_order::release);
      return true;
    }

  private:
    size_t increment(size_t idx) const {
      if (idx == store_.capacity() - 1) { [[unlikely]]
        return 0;
      }
      return idx + 1;
    }

    std::vector<T> store_;
    PaddedValue<std::atomic<size_t> > nextWriteIdx_{0};
    PaddedValue<std::atomic<size_t> > nextReadIdx_{0};
  };
}