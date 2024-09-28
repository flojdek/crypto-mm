#ifndef SPSC_QUEUE_V2_H
#define SPSC_QUEUE_V2_H

#include <vector>
#include <new>

namespace utils {
  constexpr size_t CacheLineSize = 128;

  template<typename T>
  struct alignas(CacheLineSize) PaddedAtomic {
    std::atomic<T> atomic;
    char padding[CacheLineSize - sizeof(std::atomic<T>)]; // Padding to ensure no false sharing
  };

  template <typename T>
  class SpscQueueV2 final {
  public:
    explicit SpscQueueV2(std::size_t maxElems) : store_(maxElems, T()) {}

    SpscQueueV2() = delete;
    SpscQueueV2(const SpscQueueV2 &) = delete;
    SpscQueueV2(const SpscQueueV2 &&) = delete;
    SpscQueueV2 &operator=(const SpscQueueV2 &) = delete;
    SpscQueueV2 &operator=(const SpscQueueV2 &&) = delete;

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
