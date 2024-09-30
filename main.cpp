#include <iostream>
#include <thread>
#include <latch>
#include <chrono>
#include <mutex>

#include "src/util/spsc_queue.h"

#include <boost/lockfree/spsc_queue.hpp>

using namespace std::literals::chrono_literals;

std::latch latch(2);
std::mutex cout_mutex;

struct X {
  int x;
};

template <typename T>
using QueueT = util::SpscQueue<T>;

const int F = 10;
const int N = 100000;
const int M = F * N;

auto consumeFunction(QueueT<X>& lfq) {

  latch.arrive_and_wait();

  size_t count{0};
  auto start = std::chrono::high_resolution_clock::now();
  while (count < M) {
    X item{};
    if (lfq.pop(item)) count++;
  }
  auto end = std::chrono::high_resolution_clock::now();   // End time
  std::chrono::duration<double, std::micro> duration = end - start; // Measure time in microseconds

  cout_mutex.lock();
  std::cout << "consumed " << count << " items, duration [micros] = " << duration.count() << ", tps="
    << (double)M/(duration.count() / 1000000) << std::endl;
  cout_mutex.unlock();
}

int main()
{
  QueueT<X> lfq(N);

  std::thread t{[&lfq] { consumeFunction(lfq); }};

  latch.arrive_and_wait();

  auto start = std::chrono::high_resolution_clock::now();
  int i = 0;
  while (i < M) {
    while (!lfq.push(X{i}));
    i++;
  }
  auto end = std::chrono::high_resolution_clock::now();   // End time
  std::chrono::duration<double, std::micro> duration = end - start; // Measure time in microseconds

  cout_mutex.lock();
  std::cout << "pubished " << M << " items, duration [micros] = " << duration.count() << ", tps="
    << (double)M/(duration.count() / 1000000) << std::endl;
  cout_mutex.unlock();

  std::this_thread::sleep_for(1s);

  return 0;
}
