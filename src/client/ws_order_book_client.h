#pragma once

#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl/ssl_stream.hpp>
#include <iostream>
#include <string>
#include <functional>
#include <nlohmann/json.hpp> // For parsing JSON, assuming you use this library
#include <map>
#include <algorithm>
#include <mutex>
#include <thread>
#include <chrono>

namespace client {
    using json = nlohmann::json;
    namespace beast = boost::beast; // from <boost/beast.hpp>
    namespace http = beast::http; // from <boost/beast/http.hpp>
    namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
    namespace net = boost::asio; // from <boost/asio.hpp>
    using tcp = boost::asio::ip::tcp; // from <boost/asio/ip/tcp.hpp>

    class WsOrderBookClient {
    public:
        struct WsBestBidBestAskMsg {
            double best_bid{};
            double best_bid_quantity{};
            double best_ask{};
            double best_ask_quantity{};
        };

        WsOrderBookClient(const std::string &uri, util::SpscQueue<WsBestBidBestAskMsg> &ob_updates_queue)
            : m_uri(uri)
              , m_is_connected(false)
              , m_ioc()
              , m_ssl_ctx(net::ssl::context::tlsv12_client)
              , m_resolver(m_ioc)
              , m_ws(beast::ssl_stream<beast::tcp_stream>(m_ioc, m_ssl_ctx))
              , m_ob_updates_queue(ob_updates_queue) {
            std::cout << "Initializing WebSocket client with URI: " << m_uri << std::endl;
        }

        void start() {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::cout << "Starting WebSocket client" << std::endl;

            // Parse the URI to extract the host, port, and endpoint
            std::string host = m_uri.substr(m_uri.find("//") + 2);
            std::string endpoint = "/";
            std::string port = "443";
            if (host.find("/") != std::string::npos) {
                endpoint = host.substr(host.find("/"));
                host = host.substr(0, host.find("/"));
            }
            if (host.find(":") != std::string::npos) {
                port = host.substr(host.find(":") + 1);
                host = host.substr(0, host.find(":"));
            }

            // Resolve the host
            std::cout << "host: " << host << std::endl;
            auto const results = m_resolver.resolve(host, port);

            // Make the connection on the IP address we get from a lookup
            beast::get_lowest_layer(m_ws).connect(results);

            // Perform the SSL handshake
            m_ws.next_layer().handshake(net::ssl::stream_base::client);

            // Set a decorator to change the User-Agent of the handshake
            m_ws.set_option(websocket::stream_base::decorator(
                [](websocket::request_type &req) {
                    req.set(http::field::user_agent,
                            std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-coro");
                }));

            // Perform the WebSocket handshake with the correct endpoint
            m_ws.handshake(host, endpoint);

            m_is_connected = true;
            subscribe_order_book();

            // Read messages in a separate thread
            m_read_thread = std::thread([this] { read_messages(); });
        }

        void stop() {
            std::lock_guard<std::mutex> lock(m_mutex);
            std::cout << "Stopping WebSocket client" << std::endl;
            m_ws.close(websocket::close_code::normal);
            if (m_read_thread.joinable()) {
                m_read_thread.join();
            }
            m_is_connected = false;
        }

    private:
        void read_messages() {
            try {
                for (;;) {
                    beast::flat_buffer buffer;
                    m_ws.read(buffer);
                    std::lock_guard<std::mutex> lock(m_mutex);
                    std::string message = beast::buffers_to_string(buffer.data());
                    auto start_time = std::chrono::high_resolution_clock::now();
                    on_message(message);
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).
                            count();
                    std::cout << "Processed message in " << duration << " microseconds" << std::endl;
                }
            } catch (const std::exception &e) {
                std::cerr << "Read error: " << e.what() << std::endl;
            }
        }

        void print_order_book(const std::map<double, double> &orders, const std::string &side) {
            std::cout << "Printing " << side << " orders:" << std::endl;
            for (const auto &order: orders) {
                std::cout << "Price: " << order.first << ", Quantity: " << order.second << std::endl;
            }
        }

        void on_open() {
            std::cout << "WebSocket connection opened" << std::endl;
            m_is_connected = true;
            subscribe_order_book();
        }

        void on_fail(const std::string &reason) {
            std::cerr << "Connection failed: " << reason << std::endl;
        }

        void on_close() {
            m_is_connected = false;
            std::cerr << "WebSocket connection closed" << std::endl;
        }

        void on_message(const std::string &message) {
            std::cout << "Processing WebSocket message: " << message << std::endl;
            try {
                json data = json::parse(message);
                std::cout << "Parsed JSON: " << data.dump() << std::endl;
                if (data.contains("type") && data["type"] == "l2_updates" && data.contains("symbol") && data["symbol"]
                    == "BTCGUSDPERP") {
                    if (data.contains("changes")) {
                        for (const auto &change: data["changes"]) {
                            if (change.size() == 3) {
                                std::string side = change[0].get<std::string>();
                                double price = std::stod(change[1].get<std::string>());
                                double quantity = std::stod(change[2].get<std::string>());

                                //std::cout << "Processing change - Side: " << side << ", Price: " << price << ", Quantity: " << quantity << std::endl;

                                if (side == "buy") {
                                    if (quantity > 0.0) {
                                        //std::cout << "Inserting/updating bid: " << price << " with quantity: " << quantity << std::endl;
                                        m_bid_orders[price] = quantity;
                                    } else {
                                        //std::cout << "Removing bid: " << price << std::endl;
                                        m_bid_orders.erase(price);
                                    }
                                } else if (side == "sell") {
                                    if (quantity > 0.0) {
                                        //std::cout << "Inserting/updating ask: " << price << " with quantity: " << quantity << std::endl;
                                        m_ask_orders[price] = quantity;
                                    } else {
                                        //std::cout << "Removing ask: " << price << std::endl;
                                        m_ask_orders.erase(price);
                                    }
                                }
                            }
                        }
                    }
                    trim_and_update_best_levels();
                    std::cout << "push with best bid: " << m_best_bid << ", best bid quantity: " << m_best_bid_quantity
                              << ", best ask: " << m_best_ask << ", best ask quantity: " << m_best_ask_quantity << std::endl;
                    m_ob_updates_queue.push(WsBestBidBestAskMsg{
                        .best_bid = m_best_bid,
                        .best_bid_quantity = m_best_bid_quantity,
                        .best_ask = m_best_ask,
                        .best_ask_quantity = m_best_ask_quantity
                    });
                }
            } catch (const std::exception &e) {
                std::cerr << "Error parsing message: " << e.what() << std::endl;
            }
        }

        void trim_and_update_best_levels() {
            //std::cout << "Trimming order book to top 3 levels and updating best levels" << std::endl;
            // Keep only the top 3 levels for bids
            while (m_bid_orders.size() > 3) {
                //std::cout << "Removing lowest bid level: " << m_bid_orders.begin()->first << std::endl;
                m_bid_orders.erase(m_bid_orders.begin());
            }

            // Keep only the top 3 levels for asks
            while (m_ask_orders.size() > 3) {
                //std::cout << "Removing highest ask level: " << std::prev(m_ask_orders.end())->first << std::endl;
                m_ask_orders.erase(std::prev(m_ask_orders.end()));
            }

            // Update best bid and ask levels
            if (!m_bid_orders.empty()) {
                auto best_bid_it = std::prev(m_bid_orders.end());
                m_best_bid = best_bid_it->first;
                m_best_bid_quantity = best_bid_it->second;
                //std::cout << "Updated best bid: " << m_best_bid << " with quantity: " << m_best_bid_quantity << std::endl;
            } else {
                m_best_bid = 0.0;
                m_best_bid_quantity = 0.0;
                //std::cout << "No bids available, setting best bid to -infinity" << std::endl;
            }

            if (!m_ask_orders.empty()) {
                auto best_ask_it = m_ask_orders.begin();
                m_best_ask = best_ask_it->first;
                m_best_ask_quantity = best_ask_it->second;
                //std::cout << "Updated best ask: " << m_best_ask << " with quantity: " << m_best_ask_quantity << std::endl;
            } else {
                m_best_ask = 0.0;
                m_best_ask_quantity = 0.0;
                //std::cout << "No asks available, setting best ask to infinity" << std::endl;
            }
        }

        void subscribe_order_book() {
            if (!m_is_connected) {
                std::cout << "Not connected, cannot subscribe to order book" << std::endl;
                return;
            }

            json payload = {
                {"type", "subscribe"},
                {"subscriptions", {{{"name", "l2"}, {"symbols", {"BTCGUSDPERP"}}}}}
            };
            std::cout << "Subscribing to order book with payload: " << payload.dump() << std::endl;
            m_ws.write(net::buffer(payload.dump()));
        }

        std::string m_uri;
        bool m_is_connected;
        net::io_context m_ioc;
        net::ssl::context m_ssl_ctx;
        tcp::resolver m_resolver;
        websocket::stream<beast::ssl_stream<beast::tcp_stream> > m_ws;
        std::mutex m_mutex;
        std::thread m_read_thread;
        util::SpscQueue<WsBestBidBestAskMsg> &m_ob_updates_queue;

        double m_best_bid;
        double m_best_bid_quantity;
        double m_best_ask;
        double m_best_ask_quantity;

        std::map<double, double> m_bid_orders;
        std::map<double, double> m_ask_orders;
    };
}
