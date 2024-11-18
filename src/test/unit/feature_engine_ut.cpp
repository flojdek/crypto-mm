#include "gtest/gtest.h"
#include "../../trading/feature_engine.h"

namespace trading {
    class FeatureEngineTest : public ::testing::Test {
    protected:
        FeatureEngine featureEngine;
    };

    TEST_F(FeatureEngineTest, FairPriceCalculationWithValidData) {
        double best_bid = 100.0;
        double best_bid_quantity = 10.0;
        double best_ask = 102.0;
        double best_ask_quantity = 15.0;

        featureEngine.onBestBidBestAskUpdate(best_bid, best_bid_quantity, best_ask, best_ask_quantity);
        double fair_price = featureEngine.getFairPrice();

        ASSERT_DOUBLE_EQ(fair_price, 101.2);
    }

    TEST_F(FeatureEngineTest, FairPriceCalculationWithZeroBidAndAsk) {
        double best_bid = 0.0;
        double best_bid_quantity = 0.0;
        double best_ask = 0.0;
        double best_ask_quantity = 0.0;

        featureEngine.onBestBidBestAskUpdate(best_bid, best_bid_quantity, best_ask, best_ask_quantity);
        double fair_price = featureEngine.getFairPrice();

        ASSERT_DOUBLE_EQ(fair_price, 0.0);
    }

    TEST_F(FeatureEngineTest, FairPriceCalculationWithOnlyValidBid) {
        double best_bid = 100.0;
        double best_bid_quantity = 10.0;
        double best_ask = 0.0;
        double best_ask_quantity = 0.0;

        featureEngine.onBestBidBestAskUpdate(best_bid, best_bid_quantity, best_ask, best_ask_quantity);
        double fair_price = featureEngine.getFairPrice();

        ASSERT_DOUBLE_EQ(fair_price, 0.0);
    }

    TEST_F(FeatureEngineTest, FairPriceCalculationWithOnlyValidAsk) {
        double best_bid = 0.0;
        double best_bid_quantity = 0.0;
        double best_ask = 102.0;
        double best_ask_quantity = 15.0;

        featureEngine.onBestBidBestAskUpdate(best_bid, best_bid_quantity, best_ask, best_ask_quantity);
        double fair_price = featureEngine.getFairPrice();

        ASSERT_DOUBLE_EQ(fair_price, 0.0);
    }
} // namespace trading
