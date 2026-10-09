#include <version>

#if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L
#error \
    "std::expected недоступен: нужен GCC 12+/Clang 16+ (с подходящей stdlib)/MSVC 19.33+ и -std=c++23"
#endif

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "listener/service.h"
#include "protocol/binance.dto.hpp"
#include "writer/filewriter.h"

int main() {
    const std::string host = "stream.binance.com";
    const std::string port = "9443";
    std::unordered_map<std::string, unsigned int> target{{"btcusdt", 100}};

    ob::binance::writer::AsyncDepthUpdateCsvWriter csv_writer{
        "depth_updates.csv", [](const ob::binance::dto::DepthUpdate& update) {
            // Format the fields needed from update as a CSV row.
            std::string bids;
            for (const auto& [price, qty] : update.bids) {
                if (!bids.empty()) {
                    bids += ';';
                }
                bids += std::format("{},{}", price, qty);
            }

            std::string asks;
            for (const auto& [price, qty] : update.asks) {
                if (!asks.empty()) {
                    asks += ';';
                }
                asks += std::format("{},{}", price, qty);
            }

            auto row =
                std::format("{},{},{},{},{},{},{}", update.symbol,
                            update.first_update_id, update.last_update_id,
                            update.event_time, update.symbol, bids, asks);
            return row;
        }};

    try {
        ob::binance::observer::BinanceListenerService service{
            host, port, std::move(target)};

        service.subscribe(&csv_writer);

        service.init();
        service.snifer();
    } catch (const boost::beast::system_error& se) {
        if (se.code() == boost::beast::websocket::error::closed) {
            std::cout << "connection closed cleanly\n";
            return 0;
        }
        std::cerr << "system error: " << se.code().message() << '\n';
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << '\n';
        return 1;
    }
}