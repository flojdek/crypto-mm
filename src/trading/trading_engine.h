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

class TradingEngine {
public:
  TradingEngine() {}

private:
  util::SpscQueue<client::WsOrderBookMsg>& incoming_ws_order_book_msg_;
  util::SpscQueue<client::WsTradesMsg>& incoming_ws_trade_msg_;
  util::SpscQueue<client::WsOrderMsg>& incoming_ws_order_msg_;
  util::SpscQueue<client::WsExecutionMsg>& incoming_ws_execution_msg_;
  util::SpscQueue<client::WsOrderEntryReqMsg>& outgoing_ws_order_entry_req_msg_;
  util::SpscQueue<client::WsOrderEntryResMsg>& incoming_ws_order_entry_res_msg_;
  trading::FeatureEngine& feature_engine_;
  trading::OrderManager& order_manager_;
  trading::MarketMaker& market_maker_;
};
