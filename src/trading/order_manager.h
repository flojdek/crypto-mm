#pragma once

#include "../client/ws_order_entry_client.h"

namespace trading {
  class OrderManager {
  public:
    static constexpr double clip = 0.1;

    explicit OrderManager(
      util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryReqMsg> &
      outgoing_order_entry_req_queue) : outgoing_order_entry_req_queue_(outgoing_order_entry_req_queue) {
    }

    struct OMOrder {
      enum State {
        NONE,
        PENDING_OPEN,
        OPEN,
        PENDING_CLOSE,
        CLOSED
      };

      [[nodiscard]] std::string toString() const {
        std::ostringstream oss;
        oss << "OMOrder { "
            << "price: " << price << ", "
            << "quantity: " << quantity << ", "
            << "side: " << side << ", "
            << "order_id: " << order_id << ", "
            << "client_order_id: " << client_order_id << ", "
            << "state: " << state
            << " }";
        return oss.str();
      }

      State state{NONE};
      double price{};
      double quantity{};
      std::string side{};
      std::string order_id{};
      std::string client_order_id{};
    };

    void moveOrders(double bid_price, double ask_price) {
      std::cout << "OrderManager::moveOrders(bid_price=" << bid_price << ", ask_price=" << ask_price << ")" <<
          std::endl;
      moveOrder(bid_order_, bid_price, "bid", clip);
      moveOrder(ask_order_, ask_price, "ask", clip);
    }

    void moveOrder(OMOrder &order, double price, const std::string& side, double quantity) {
      switch (order.state) {
        case OMOrder::OPEN: {
          if (order.price != price) {
            cancelOrder(order);
          }
        }
        break;
        case OMOrder::NONE:
        case OMOrder::CLOSED: {
          if (price != 0.0) [[likely]] {
            // TODO: Check pre-trade risk.
            newOrder(order, price, side, quantity);
          }
        }
        break;
        case OMOrder::PENDING_OPEN:
        case OMOrder::PENDING_CLOSE: {
          std::cout << order.toString() << " already pending new/cancel..." << std::endl;
        }
        break;
      }
    }

    void newOrder(OMOrder &order, double price, const std::string& side, double quantity) {
      client::WsOrderEntryClient::WsOrderEntryReqMsg msg;
      msg.type = client::WsOrderEntryClient::WsOrderEntryReqMsg::RequestType::NEW_ORDER;
      msg.price = price;
      msg.quantity = quantity;
      msg.side = side;
      msg.client_order_id = order.client_order_id;
      outgoing_order_entry_req_queue_.push(msg);
      order.state = OMOrder::PENDING_OPEN;
      order.price = price;
      order.side = side;
      order.quantity = quantity;
      std::cout << "newOrder: " << order.toString() << std::endl;
    }

    void cancelOrder(OMOrder &order) {
      std::cout << "cancelOrder: " << order.toString() << std::endl;
    }

    void onOrderUpdate(const client::WsOrderEntryClient::WsOrderEntryResMsg& msg) {
      auto order = msg.side == "bid" ? bid_order_ : ask_order_;
      switch (msg.status) {
        case client::WsOrderEntryClient::WsOrderEntryResMsg::OrdStatus::NEW: {
          order.state = OMOrder::OPEN;
        }
        break;
        case client::WsOrderEntryClient::WsOrderEntryResMsg::OrdStatus::CANCELED: {
          order.state = OMOrder::CLOSED;
        }
        break;
        case client::WsOrderEntryClient::WsOrderEntryResMsg::OrdStatus::PARTIALLY_FILLED:
        case client::WsOrderEntryClient::WsOrderEntryResMsg::OrdStatus::FILLED: {
          order.quantity = msg.leaves_qty;
          if(order.quantity == 0.0)
            order.state = OMOrder::CLOSED;
        }
        break;
        case client::WsOrderEntryClient::WsOrderEntryResMsg::OrdStatus::REJECTED:
        case client::WsOrderEntryClient::WsOrderEntryResMsg::OrdStatus::NONE: {
        }
        break;
      };
    }

  private:
    util::SpscQueue<client::WsOrderEntryClient::WsOrderEntryReqMsg> &outgoing_order_entry_req_queue_;

    OMOrder ask_order_{};
    OMOrder bid_order_{};
  };
}
