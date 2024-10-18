cmake_minimum_required(VERSION 3.29)
project(crypto_mm)

set(CMAKE_CXX_STANDARD 20)

find_package(Boost)
find_package(nlohmann_json)
find_package(OpenSSL)

add_executable(crypto_mm main.cpp
        src/trading/feature_engine.h
        src/trading/order_manager.h
        src/trading/market_maker.h
        src/client/ws_order_book_client.h
        src/client/ws_trades_client.h
        src/client/ws_order_client.h
        src/client/ws_execution_client.h
        src/client/ws_order_entry_client.h
        src/util/spsc_queue.h
        src/util/padded_value.h
        src/trading/trading_engine.h)

target_link_libraries(crypto_mm boost::boost nlohmann_json::nlohmann_json openssl::openssl)