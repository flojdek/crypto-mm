#include <thread>

#include "gtest/gtest.h"
#include "../../trading/feature_engine.h"
#include "../../trading/order_manager.h"
#include "../../trading/market_maker.h"
#include "../../trading/trading_engine.h"
#include "../../client/ws_order_entry_client.h"
#include "../../util/spsc_queue.h"

namespace trading {
    class TradingEngineTest : public ::testing::Test {
    protected:
        util::SpscQueue<client::WsOrderBookClient::WsBestBidBestAskMsg> incoming_order_book_updates_queue{1000};
        util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryReqMsg> outgoing_order_entry_req_queue{1000};
        util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryResMsg> incoming_order_entry_res_queue{1000};

        FeatureEngine featureEngine;
        OrderManager orderManager{outgoing_order_entry_req_queue};
        MarketMaker marketMaker{orderManager, featureEngine};
        TradingEngine tradingEngine{
            featureEngine, orderManager, marketMaker, incoming_order_book_updates_queue,
            incoming_order_entry_res_queue
        };

        void SetUp() override {
            tradingEngine.start();
        }

        void TearDown() override {
            tradingEngine.stop();
        }
    };

    TEST_F(TradingEngineTest, ProcessOrderBookUpdate) {
        client::WsOrderBookClient::WsBestBidBestAskMsg best_bid_best_ask_msg;
        best_bid_best_ask_msg.best_bid = 100.0;
        best_bid_best_ask_msg.best_bid_quantity = 10.0;
        best_bid_best_ask_msg.best_ask = 102.0;
        best_bid_best_ask_msg.best_ask_quantity = 15.0;

        incoming_order_book_updates_queue.push(best_bid_best_ask_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Allow some time for processing

        double fair_price = featureEngine.getFairPrice();
        ASSERT_DOUBLE_EQ(fair_price, 101.2);
    }

    TEST_F(TradingEngineTest, ProcessOrderEntryResponse) {
        // Arrange
        client::WsOrderEntryClient::WsOrderEntryResMsg order_entry_res_msg{
            client::WsOrderEntryClient::WsOrderEntryResMsg::OrdStatus::NEW, "Order executed", "order123"
        };

        // Act
        incoming_order_entry_res_queue.push(order_entry_res_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Allow some time for processing

        // Assert
        // Here we would ideally verify that the order manager has processed the order update correctly.
        // This would involve adding methods to OrderManager to query the state of orders for verification.
        // For now, we just check that no exceptions occurred.
        SUCCEED();
    }

    TEST_F(TradingEngineTest, SubmitOrderOnBestBidBestAskUpdate) {
        client::WsOrderBookClient::WsBestBidBestAskMsg best_bid_best_ask_msg;
        best_bid_best_ask_msg.best_bid = 100.0;
        best_bid_best_ask_msg.best_bid_quantity = 10.0;
        best_bid_best_ask_msg.best_ask = 102.0;
        best_bid_best_ask_msg.best_ask_quantity = 15.0;

        incoming_order_book_updates_queue.push(best_bid_best_ask_msg);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Allow some time for processing

        std::array<client::WsOrderEntryClient::WsOrderEntryReqMsg, 2> orders;
        ASSERT_TRUE(outgoing_order_entry_req_queue.pop(orders[0]));
        ASSERT_TRUE(outgoing_order_entry_req_queue.pop(orders[1]));

        client::WsOrderEntryClient::WsOrderEntryReqMsg *bid_order = nullptr;
        client::WsOrderEntryClient::WsOrderEntryReqMsg *ask_order = nullptr;

        for (auto &order : orders) {
            if (order.side == "bid") {
                bid_order = &order;
            } else if (order.side == "ask") {
                ask_order = &order;
            }
        }

        ASSERT_NE(bid_order, nullptr);
        ASSERT_NE(ask_order, nullptr);

        // Assertions for bid order
        ASSERT_EQ(bid_order->side, "bid");
        ASSERT_EQ(bid_order->price, 100.0);
        ASSERT_EQ(bid_order->quantity, 0.1);

        // Assertions for ask order
        ASSERT_EQ(ask_order->side, "ask");
        ASSERT_EQ(ask_order->price, 101.0);
        ASSERT_EQ(ask_order->quantity, 0.1);

        std::cout << "Bid Order: " << bid_order->to_string() << std::endl;
        std::cout << "Ask Order: " << ask_order->to_string() << std::endl;
    }
} // namespace trading
