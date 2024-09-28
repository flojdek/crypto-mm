#ifndef SPSC_QUEUE_V1_H
#define SPSC_QUEUE_V1_H

#include <vector>

/*
 * This is like the bare-bones version 1. Pretty raw/crude. No safety nets.
 * Can overwrite items and have out-of-order reads. Shitty API.
 */
namespace utils {
  template <typename T>
  class SpscQueueV1 final {
  public:
    explicit SpscQueueV1(std::size_t maxElems) : store_(maxElems, T()) {}

    SpscQueueV1() = delete;
    SpscQueueV1(const SpscQueueV1 &) = delete;
    SpscQueueV1(const SpscQueueV1 &&) = delete;
    SpscQueueV1 &operator=(const SpscQueueV1 &) = delete;
    SpscQueueV1 &operator=(const SpscQueueV1 &&) = delete;

    auto getNextToWrite() noexcept {
      return &store_[nextWriteIdx_];
    }

    auto updateWriteIdx() noexcept {
      const auto currentWriteIdx = nextWriteIdx_.load();
      nextWriteIdx_.store((currentWriteIdx + 1) % store_.size());
      ++numElems_;
    }

    auto getNextToRead() const noexcept {
      return (size() ? &store_[nextReadIdx_] : nullptr);
    }

    auto updateReadIndex() noexcept {
      const auto currentReadIdx = nextReadIdx_.load();
      nextReadIdx_.store((currentReadIdx + 1) % store_.size());
      --numElems_;
    }

    auto size() const noexcept {
      return numElems_.load();
    }
  private:
    std::vector<T> store_;
    std::atomic<size_t> nextWriteIdx_{0};
    std::atomic<size_t> nextReadIdx_{0};
    std::atomic<size_t> numElems_{0};
  };
}

#endif