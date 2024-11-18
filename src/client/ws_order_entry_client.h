#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/connect.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <string>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <thread>
#include <atomic>
#include "../util/spsc_queue.h"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace client {
    class WsOrderEntryClient {
    public:
        struct WsOrderEntryReqMsg {
            enum class RequestType {
                NONE,
                NEW_ORDER,
                CANCEL_ORDER
            };

            WsOrderEntryReqMsg() = default;

            std::string to_string() const {
                std::ostringstream oss;
                if (type == RequestType::NEW_ORDER) {
                    oss << "amount=" << quantity << "&price=" << price << "&side=" << side << "&type=exchange limit";
                } else if (type == RequestType::CANCEL_ORDER) {
                    if (!order_id.empty()) {
                        oss << "order_id=" << order_id;
                    }
                    if (!client_order_id.empty()) {
                        if (!oss.str().empty()) {
                            oss << "&";
                        }
                        oss << "client_order_id=" << client_order_id;
                    }
                }
                return oss.str();
            }

            RequestType type{RequestType::NONE};
            double price{};
            double quantity{};
            std::string side{};
            std::string order_id{};
            std::string client_order_id{};
        };

        struct WsOrderEntryResMsg {
            // Follows https://btobits.com/fixopaedia/fixdic44/tag_39_OrdStatus_.html loosely.
            enum class OrdStatus {
                NONE,
                NEW,
                CANCELED,
                PARTIALLY_FILLED,
                FILLED,
                REJECTED
            };

            WsOrderEntryResMsg() = default;

            WsOrderEntryResMsg(OrdStatus st, const std::string& msg)
                : status(st), message(msg) {
            }

            WsOrderEntryResMsg(OrdStatus st, const std::string& msg, const std::string& oid)
                : status(st), message(msg), order_id(oid) {
            }

            OrdStatus status{OrdStatus::NONE};
            std::string message{};
            std::string side;
            std::string order_id{};
            std::string client_order_id{};
            double leaves_qty{};
        };

        WsOrderEntryClient(const std::string &api_base_url, util::SpscQueue<WsOrderEntryReqMsg> &request_queue,
                           util::SpscQueue<WsOrderEntryResMsg> &response_queue, const std::string &api_key,
                           const std::string &api_secret)
            : api_base_url_(api_base_url), request_queue_(request_queue), response_queue_(response_queue),
              api_key_(api_key), api_secret_(api_secret), ioc_(), resolver_(ioc_), stream_(ioc_),
              stop_flag_(false) {
        }

        ~WsOrderEntryClient() {
            stop();
        }

        void start() {
            stop_flag_ = false;
            processing_thread_ = std::thread(&WsOrderEntryClient::process_orders, this);
        }

        void stop() {
            stop_flag_ = true;
            if (processing_thread_.joinable()) {
                processing_thread_.join();
            }
        }

    private:
        void process_orders() {
            WsOrderEntryReqMsg order_request;
            while (!stop_flag_) {
                if (request_queue_.pop(order_request)) {
                    if (order_request.type == WsOrderEntryReqMsg::RequestType::NEW_ORDER) {
                        send_order(order_request);
                    } else if (order_request.type == WsOrderEntryReqMsg::RequestType::CANCEL_ORDER) {
                        cancel_order(order_request);
                    }
                } else {
                    std::cout << "No orders to process" << std::endl;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }

        void send_order(const WsOrderEntryReqMsg &order_request) {
            execute_request(order_request, "/v1/order/new", "Order submitted successfully");
        }

        void cancel_order(const WsOrderEntryReqMsg &order_request) {
            execute_request(order_request, "/v1/order/cancel", "Order cancelled successfully");
        }

        void execute_request(const WsOrderEntryReqMsg &order_request, const std::string &target,
                             const std::string &success_message) {
            try {
                auto const host = "api.gemini.com";
                auto const port = "443";
                int version = 11;

                // Generate nonce
                auto nonce = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();

                // Create payload
                std::ostringstream payload_stream;
                payload_stream << "nonce=" << nonce << "&request=" << target;
                if (!order_request.to_string().empty()) {
                    payload_stream << "&" << order_request.to_string();
                }
                std::string payload = payload_stream.str();
                std::string encoded_payload = base64_encode(reinterpret_cast<const unsigned char *>(payload.c_str()),
                                                            payload.length());

                // Generate signature
                unsigned char *digest = HMAC(EVP_sha384(), api_secret_.c_str(), api_secret_.length(),
                                             reinterpret_cast<const unsigned char *>(encoded_payload.c_str()),
                                             encoded_payload.length(), nullptr, nullptr);
                std::ostringstream signature_stream;
                for (int i = 0; i < SHA384_DIGEST_LENGTH; ++i) {
                    signature_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(digest[i]);
                }
                std::string signature = signature_stream.str();

                // Resolve the host
                auto const results = resolver_.resolve(host, port);

                // Make the connection on the IP address we get from a lookup
                beast::get_lowest_layer(stream_).connect(results);

                // Set up an HTTP POST request message
                http::request<http::string_body> req{http::verb::post, target, version};
                req.set(http::field::host, host);
                req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
                req.set("X-GEMINI-APIKEY", api_key_);
                req.set("X-GEMINI-PAYLOAD", encoded_payload);
                req.set("X-GEMINI-SIGNATURE", signature);
                req.set(http::field::content_type, "application/x-www-form-urlencoded");
                req.prepare_payload();

                // Send the HTTP request to the remote host
                http::write(stream_, req);

                // This buffer is used for reading and must be persisted
                beast::flat_buffer buffer;

                // Declare a container to hold the response
                http::response<http::string_body> res;

                // Receive the HTTP response
                http::read(stream_, buffer, res);

                // Handle the response
                if (res.result() == http::status::ok) {
                    // Parse order ID from response
                    std::string order_id;
                    auto body = res.body();
                    auto pos = body.find("order_id");
                    if (pos != std::string::npos) {
                        auto start = body.find(':', pos) + 1;
                        auto end = body.find(',', start);
                        order_id = body.substr(start, end - start);
                    }
                    response_queue_.push(WsOrderEntryResMsg(WsOrderEntryResMsg::OrdStatus::NEW, success_message, order_id));
                } else {
                    response_queue_.push(WsOrderEntryResMsg(WsOrderEntryResMsg::OrdStatus::REJECTED, res.reason()));
                }

                // Gracefully close the stream
                beast::get_lowest_layer(stream_).socket().shutdown(tcp::socket::shutdown_both);
            } catch (const std::exception &e) {
                std::cerr << "Error: " << e.what() << std::endl;
                response_queue_.push(WsOrderEntryResMsg(WsOrderEntryResMsg::OrdStatus::REJECTED, e.what()));
            }
        }

        std::string api_base_url_;
        util::SpscQueue<WsOrderEntryReqMsg> &request_queue_;
        util::SpscQueue<WsOrderEntryResMsg> &response_queue_;
        std::string api_key_;
        std::string api_secret_;

        net::io_context ioc_;
        tcp::resolver resolver_;
        beast::tcp_stream stream_;
        std::thread processing_thread_;
        std::atomic<bool> stop_flag_;

        std::string base64_encode(const unsigned char *input, size_t length) {
            static const char encode_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
            std::string encoded;
            encoded.reserve(((length + 2) / 3) * 4);
            for (size_t i = 0; i < length; i += 3) {
                unsigned int value = 0;
                for (size_t j = i; j < i + 3; ++j) {
                    value <<= 8;
                    if (j < length) {
                        value |= input[j];
                    }
                }
                encoded.push_back(encode_table[(value >> 18) & 0x3F]);
                encoded.push_back(encode_table[(value >> 12) & 0x3F]);
                encoded.push_back((i + 1 < length) ? encode_table[(value >> 6) & 0x3F] : '=');
                encoded.push_back((i + 2 < length) ? encode_table[value & 0x3F] : '=');
            }
            return encoded;
        }
    };
} // namespace client
