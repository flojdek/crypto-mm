#ifndef SPSC_QUEUE_V2_H
#define SPSC_QUEUE_V2_H

#include <vector>

/*
 * Better API. Will not overwrite and have out-of-order reads.
 */
namespace utils {
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
      const auto idx = nextWriteIdx_.load();
      const auto next_idx = increment(idx);
      if (next_idx != nextReadIdx_.load()) {
        store_[idx] = elem;
        nextWriteIdx_.store(next_idx);
        return true;
      }
      return false;
    }

    auto pop(T& elem) noexcept {
      const auto idx = nextReadIdx_.load();
      if (idx == nextWriteIdx_.load()) {
        return false;
      }
      elem = store_[idx];
      const auto next_idx = increment(idx);
      nextReadIdx_.store(next_idx);
      return true;
    }

    size_t increment(size_t idx) const
    {
      return (idx + 1) % store_.capacity();
    }
  private:
    std::vector<T> store_;
    std::atomic<size_t> nextWriteIdx_{0};
    std::atomic<size_t> nextReadIdx_{0};
  };
}

#endif
