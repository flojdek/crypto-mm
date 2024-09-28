#include <iostream>
#include <thread>
#include <latch>
#include <chrono>
#include <mutex>

#include "src/spsc_queue/spsc_queue_v1.h"

using namespace std::literals::chrono_literals;

std::latch latch(2);
std::mutex cout_mutex;

struct X {
  int x;
};

const int N = 1000000;

auto consumeFunction(utils::SpscQueueV1<X>& lfq) {

  latch.arrive_and_wait();

  size_t count{0};
  auto start = std::chrono::high_resolution_clock::now();
  while (count < N) {
    const auto d = lfq.getNextToRead();
    count++;
    lfq.updateReadIndex();
  }
  auto end = std::chrono::high_resolution_clock::now();   // End time
  std::chrono::duration<double, std::micro> duration = end - start; // Measure time in microseconds

  cout_mutex.lock();
  std::cout << "consumed " << count << " items, duration [micros] = " << duration.count() << ", tps="
    << (double)N/(duration.count() / 1000000) << std::endl;
  cout_mutex.unlock();
}

int main()
{
  utils::SpscQueueV1<X> qv1(N);

  std::thread t{[&qv1] { consumeFunction(qv1); }};

  latch.arrive_and_wait();

  auto start = std::chrono::high_resolution_clock::now();
  for (auto i = 0; i < 10 * N; i++) {
    *(qv1.getNextToWrite()) = X{.x = i};
    qv1.updateWriteIdx();
  }
  auto end = std::chrono::high_resolution_clock::now();   // End time
  std::chrono::duration<double, std::micro> duration = end - start; // Measure time in microseconds

  cout_mutex.lock();
  std::cout << "published " << N << " items, duration [micros] = " << duration.count() << ", tps="
    << (double)N/(duration.count() / 1000000) << std::endl;
  cout_mutex.unlock();

  std::this_thread::sleep_for(5s);

  return 0;
}
