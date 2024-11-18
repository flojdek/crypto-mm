#pragma once

#include "feature_engine.h"
#include "order_manager.h"

#include "../client/ws_order_entry_client.h"

namespace trading {
  class MarketMaker {
  public:
    explicit MarketMaker(OrderManager &om, FeatureEngine &fe) : feature_engine_(fe), order_manager_(om) {
    }

    MarketMaker(const MarketMaker &) = delete;

    MarketMaker(const MarketMaker &&) = delete;

    MarketMaker &operator=(const MarketMaker &) = delete;

    MarketMaker &operator=(const MarketMaker &&) = delete;

    void onBestBidBestAskUpdate(double best_bid, double best_bid_quantity, double best_ask, double best_ask_quantity) {
      const auto fair_price = feature_engine_.getFairPrice();
      const auto one_pct_below_fair_price = fair_price * (1 - 0.01);
      const auto one_pct_above_fair_price = fair_price * (1 + 0.01);

      if (best_bid != 0.0 && best_bid_quantity != 0.0 && best_ask != 0.0 && best_ask_quantity != 0.0 && fair_price !=
          0.0) [[likely]] {
        // On any side if the market is very close (1%) to our fair price we quote a bit wider.
        // On any side if the market is further away from our fair price we join the best quotes.
        const auto bid_price = one_pct_below_fair_price <= best_bid ? (best_bid - 1.0) : best_bid;
        const auto ask_price = best_ask <= one_pct_above_fair_price ? (best_ask - 1.0) : best_ask;

        order_manager_.moveOrders(bid_price, ask_price);
      }
    }

    void onOrderUpdate(client::WsOrderEntryClient::WsOrderEntryResMsg msg) {
      order_manager_.onOrderUpdate(msg);
    }

  private:
    double m_fair_price{};
    const FeatureEngine &feature_engine_;
    OrderManager &order_manager_;
  };
}
