#ifndef SPSC_QUEUE_V1_H
#define SPSC_QUEUE_V1_H

#include <vector>

namespace utils {
  template <typename T>
  // Version 1 for now, this one has some issues.
  // Reference: https://www.boost.org/doc/libs/1_60_0/boost/lockfree/spsc_queue.hpp
  class SpscQueueV1 final {
  public:
    explicit SpscQueueV1(std::size_t maxElems) {
      store_(maxElems, T());
    }

    SpscQueueV1() = delete;
    SpscQueueV1(const SpscQueueV1 &) = delete;
    SpscQueueV1(const SpscQueueV1 &&) = delete;
    SpscQueueV1 &operator=(const SpscQueueV1 &) = delete;
    SpscQueueV1 &operator=(const SpscQueueV1 &&) = delete;

    auto getNextToWrite() noexcept {
      return &store_[nextWriteIdx_];
    }

    auto updateWriteIdx() noexcept {
      nextWriteIdx_ = (nextWriteIdx_ + 1) % store_.size();
      ++numElems_;
    }

    auto getNextToRead() const noexcept {
      return (size() ? &store_[nextReadIdx_] : nullptr);
    }

    auto updateReadIndex() noexcept {
      nextReadIdx_ = (nextReadIdx_ + 1) % store_.size();
      --numElems_;
    }

    auto size() const noexcept {
      return numElems_.load();
    }
  private:
    std::vector<T> store_;
    std::atomic<size_t> nextWriteIdx_ = {0};
    std::atomic<size_t> nextReadIdx_ = {0};
    std::atomic<size_t> numElems_ = {0};
  };
}

#endif //SPSC__QUEUE_H
