#include <iostream>
#include <thread>
#include <latch>
#include <chrono>
#include <mutex>

#include "src/util/spsc_queue.h"

#include <boost/lockfree/spsc_queue.hpp>
#include <gtest/gtest.h>

#include "src/client/ws_order_book_client.h"
#include "src/trading/feature_engine.h"
#include "src/trading/trading_engine.h"

using namespace std::literals::chrono_literals;

std::latch latch(2);
std::mutex cout_mutex;

struct X {
  int x;
};

template<typename T>
using QueueT = util::SpscQueue<T>;

const int F = 10;
const int N = 100000;
const int M = F * N;

auto consumeFunction(QueueT<X> &lfq) {
  latch.arrive_and_wait();

  size_t count{0};
  auto start = std::chrono::high_resolution_clock::now();
  while (count < M) {
    X item{};
    if (lfq.pop(item)) count++;
  }
  auto end = std::chrono::high_resolution_clock::now(); // End time
  std::chrono::duration<double, std::micro> duration = end - start; // Measure time in microseconds

  cout_mutex.lock();
  std::cout << "consumed " << count << " items, duration [micros] = " << duration.count() << ", tps="
      << (double) M / (duration.count() / 1000000) << std::endl;
  cout_mutex.unlock();
}

int main(int argc, char *argv[]) {
  for (int i = 1; i < argc; ++i) {
    if (std::string(argv[i]).find("--gtest") == 0 || std::string(argv[i]) == "--run_tests") {
      ::testing::InitGoogleTest(&argc, argv);
      return RUN_ALL_TESTS();
    }
  }

  util::SpscQueue<client::WsOrderBookClient::WsBestBidBestAskMsg> incoming_order_book_updates_queue(10000);
  util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryReqMsg> outgoing_order_entry_req_queue(10000);
  util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryResMsg> incoming_order_entry_res_queue(10000);

  client::WsOrderBookClient ws_order_book_client("wss://api.gemini.com/v2/marketdata",
                                                 incoming_order_book_updates_queue);
  ws_order_book_client.start();

  client::WsOrderEntryClient ws_order_entry_client("https://api.gemini.com", outgoing_order_entry_req_queue,
                                                   incoming_order_entry_res_queue,
                                                   "key", "secret");
  ws_order_entry_client.start();

  trading::FeatureEngine feature_engine;
  trading::OrderManager order_manager(outgoing_order_entry_req_queue);
  trading::MarketMaker market_maker(order_manager, feature_engine);
  trading::TradingEngine trading_engine(
    feature_engine,
    order_manager,
    market_maker,
    incoming_order_book_updates_queue,
    incoming_order_entry_res_queue
  );

  trading_engine.start();

  while (true) {
    std::this_thread::sleep_for(10ms);
  }

  return 0;
}
