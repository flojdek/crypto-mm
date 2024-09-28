#ifndef SPSC_QUEUE_V4_H
#define SPSC_QUEUE_V4_H

#include <vector>

#include "../util/padded_atomic.h"

/*
 * Better API. Will not overwrite and have out-of-order reads.
 * With padding and also with explicit memory ordering where we know it... should work.
 */
namespace utils {
  template <typename T>
  class SpscQueueV4 final {
  public:
    explicit SpscQueueV4(std::size_t maxElems) : store_(maxElems, T()) {}

    SpscQueueV4() = delete;
    SpscQueueV4(const SpscQueueV4 &) = delete;
    SpscQueueV4(const SpscQueueV4 &&) = delete;
    SpscQueueV4 &operator=(const SpscQueueV4 &) = delete;
    SpscQueueV4 &operator=(const SpscQueueV4 &&) = delete;

    auto push(const T& elem) noexcept {
      const auto idx = nextWriteIdx_.atomic.load(std::memory_order::relaxed);
      const auto next_idx = increment(idx);
      if (next_idx != nextReadIdx_.atomic.load(std::memory_order::acquire)) {
        store_[idx] = elem;
        nextWriteIdx_.atomic.store(next_idx, std::memory_order::release);
        return true;
      }
      return false;
    }

    auto pop(T& elem) noexcept {
      const auto idx = nextReadIdx_.atomic.load(std::memory_order::relaxed);
      if (idx == nextWriteIdx_.atomic.load(std::memory_order::acquire)) {
        return false;
      }
      elem = store_[idx];
      const auto next_idx = increment(idx);
      nextReadIdx_.atomic.store(next_idx, std::memory_order::release);
      return true;
    }

    size_t increment(size_t idx) const
    {
      return (idx + 1) % store_.capacity();
    }
  private:
    std::vector<T> store_;
    PaddedAtomic<size_t> nextWriteIdx_{0};
    PaddedAtomic<size_t> nextReadIdx_{0};
  };
}

#endif
