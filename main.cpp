#include <iostream>
#include <thread>
#include <latch>
#include <chrono>
#include <mutex>

#include "src/util/spsc_queue.h"

#include <boost/lockfree/spsc_queue.hpp>

#include "src/client/ws_order_book_client.h"

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
  WsOrderBookClient ws_ob_client("wss://api.gemini.com/v2/marketdata");
  ws_ob_client.set_update_callback([](double best_bid_px, double best_bid_qty, double best_ask_px, double best_ask_qty) {
    /*
    std::cout << "Ask: " << best_ask_px << " " << best_ask_qty << std::endl;
    std::cout << "---- " << 10000.0 * (best_ask_px - best_bid_px)/best_bid_px << " ----" << std::endl;
    std::cout << "Bid: " << best_bid_px << " " << best_bid_qty << std::endl;
    std::cout << std::endl;
    */
  });
  ws_ob_client.start();

  while (true) {
    std::this_thread::sleep_for(1s);
  }

  return 0;
}
