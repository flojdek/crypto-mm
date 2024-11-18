#pragma once

namespace trading {
  class FeatureEngine {
  public:
    explicit FeatureEngine() {
    }

    FeatureEngine(const FeatureEngine &) = delete;

    FeatureEngine(const FeatureEngine &&) = delete;

    FeatureEngine &operator=(const FeatureEngine &) = delete;

    FeatureEngine &operator=(const FeatureEngine &&) = delete;

    void onBestBidBestAskUpdate(double best_bid, double best_bid_quantity, double best_ask, double best_ask_quantity) {
      std::cout << "best_bid: " << best_bid << std::endl;
      std::cout << "best_bid_quantity: " << best_bid_quantity << std::endl;
      std::cout << "best_ask: " << best_ask << std::endl;
      std::cout << "best_ask_quantity: " << best_ask_quantity << std::endl;
      if (best_bid != 0.0 && best_bid_quantity != 0.0 && best_ask != 0.0 && best_ask_quantity != 0.0) {
        [[likely]]
            m_fair_price = (best_bid * best_bid_quantity + best_ask * best_ask_quantity) / (
                             best_bid_quantity + best_ask_quantity);
      } else {
        m_fair_price = 0.0;
      }
      std::cout << "m_fair_price: " << m_fair_price << std::endl;
    }

    double getFairPrice() const {
      return m_fair_price;
    }

  private:
    double m_fair_price{0.0};
  };
}
