#pragma once

#include "feature_engine.h"
#include "market_maker.h"
#include "order_manager.h"
#include "../client/ws_execution_client.h"
#include "../client/ws_order_book_client.h"
#include "../client/ws_order_client.h"
#include "../client/ws_order_entry_client.h"
#include "../client/ws_trades_client.h"
#include "../util/spsc_queue.h"

namespace trading {
  class TradingEngine {
  public:
    TradingEngine(FeatureEngine &feature_engine, OrderManager &order_manager, MarketMaker &market_maker,
                  util::SpscQueue<client::WsOrderBookClient::WsBestBidBestAskMsg> &incoming_order_book_updates_queue,
                  util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryResMsg> &incoming_order_entry_res_queue)
      : feature_engine_(feature_engine)
        , order_manager_(order_manager)
        , market_maker_(market_maker)
        , incoming_order_book_updates_queue_(incoming_order_book_updates_queue)
        , incoming_order_entry_res_queue_(incoming_order_entry_res_queue) {
    }

    void start() {
      run_ = true;
      processing_thread_ = std::thread(&TradingEngine::process, this);
    }

    void stop() {
      run_ = false;
      if (processing_thread_.joinable()) {
        processing_thread_.join();
      }
    }

    void process() {
      while (run_) {
        client::WsOrderBookClient::WsBestBidBestAskMsg best_bid_best_ask_msg;
        if (incoming_order_book_updates_queue_.pop(best_bid_best_ask_msg)) {
          feature_engine_.onBestBidBestAskUpdate(best_bid_best_ask_msg.best_bid,
                                                 best_bid_best_ask_msg.best_bid_quantity,
                                                 best_bid_best_ask_msg.best_ask,
                                                 best_bid_best_ask_msg.best_ask_quantity);
          market_maker_.onBestBidBestAskUpdate(best_bid_best_ask_msg.best_bid, best_bid_best_ask_msg.best_bid_quantity,
                                               best_bid_best_ask_msg.best_ask,
                                               best_bid_best_ask_msg.best_ask_quantity);
        }
        client::WsOrderEntryClient::WsOrderEntryResMsg order_entry_res_msg;
        if (incoming_order_entry_res_queue_.pop(order_entry_res_msg)) {
          market_maker_.onOrderUpdate(order_entry_res_msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
    }

  private:
    bool run_{false};
    std::thread processing_thread_;
    trading::FeatureEngine &feature_engine_;
    trading::OrderManager &order_manager_;
    trading::MarketMaker &market_maker_;
    util::SpscQueue<client::WsOrderBookClient::WsBestBidBestAskMsg> &incoming_order_book_updates_queue_;
    util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryResMsg> &incoming_order_entry_res_queue_;
  };
}
