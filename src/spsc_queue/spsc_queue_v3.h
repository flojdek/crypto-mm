#ifndef SPSC_QUEUE_V3_H
#define SPSC_QUEUE_V3_H

#include <vector>

#include "../util/padded_atomic.h"

/*
 * Better API. Will not overwrite and have out-of-order reads and added padding to avoid some false sharing.
 */
namespace utils {
  template <typename T>
  class SpscQueueV3 final {
  public:
    explicit SpscQueueV3(std::size_t maxElems) : store_(maxElems, T()) {}

    SpscQueueV3() = delete;
    SpscQueueV3(const SpscQueueV3 &) = delete;
    SpscQueueV3(const SpscQueueV3 &&) = delete;
    SpscQueueV3 &operator=(const SpscQueueV3 &) = delete;
    SpscQueueV3 &operator=(const SpscQueueV3 &&) = delete;

    auto push(const T& elem) noexcept {
      const auto idx = nextWriteIdx_.atomic.load();
      const auto next_idx = increment(idx);
      if (next_idx != nextReadIdx_.atomic.load()) {
        store_[idx] = elem;
        nextWriteIdx_.atomic.store(next_idx);
        return true;
      }
      return false;
    }

    auto pop(T& elem) noexcept {
      const auto idx = nextReadIdx_.atomic.load();
      if (idx == nextWriteIdx_.atomic.load()) {
        return false;
      }
      elem = store_[idx];
      const auto next_idx = increment(idx);
      nextReadIdx_.atomic.store(next_idx);
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
