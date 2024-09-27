cmake_minimum_required(VERSION 3.29)
project(bybitmm)

set(CMAKE_CXX_STANDARD 20)

add_executable(bybitmm main.cpp
        src/strategy/feature_engine.h
        src/strategy/order_manager.h
        src/strategy/market_maker.h
        src/client/ws_order_book_client.h
        src/client/ws_trades_client.h
        src/client/ws_order_client.h
        src/client/ws_execution_client.h
        src/client/ws_order_entry_client.h
        src/spsc_queue/spsc_queue_v1.h
        src/spsc_queue/spsc_padded_queue.h)
