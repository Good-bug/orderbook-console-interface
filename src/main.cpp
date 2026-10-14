#include <version>

#include "rest/binance_rest_depth_client.hpp"


#if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L
#error \
    "std::expected недоступен: нужен GCC 12+/Clang 16+ (с подходящей stdlib)/MSVC 19.33+ и -std=c++23"
#endif

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <format>
#include <iostream>
#include <string>
#include <unordered_map>

#include "gateway/binance_listener_service.hpp"
#include "protocol/binance_dto.hpp"
#include "protocol/binance_convert.hpp"
#include "writer/filewriter.hpp"


using namespace ob;

namespace {
std::string formatDepthUpdateCsvRow(const dto::DepthUpdateDto& update) {
    std::string bids;
    for (const auto& level : update.bids) {
        if (!bids.empty()) {
            bids += ';';
        }
        bids +=
            std::format("{},{}", level[0],
                        level[1]);
    }

    std::string asks;
    for (const auto& level : update.asks) {
        if (!asks.empty()) {
            asks += ';';
        }
        asks +=
            std::format("{},{}", level[0],
                        level[1]);
    }

    return std::format("{},{},{},{},{},{}", update.symbol,
                       update.first_update_id, update.last_update_id,
                       update.event_time, bids, asks);
}
}  // namespace

int main() {
    const std::string host = "stream.binance.com";
    const std::string port = "9443";
    std::unordered_map<std::string, unsigned int> target{{"btcusdt", 100}};

    ob::writer::AsyncDepthUpdateCsvWriter csv_writer{"depth_updates.csv",
                                                     formatDepthUpdateCsvRow};

    ob::rest::BinanceRestDepthClient rest_client{};
    [[maybe_unused]] const auto snapshot =
        rest_client.fetch_snapshot("btcusdt", 100);
    // std::cout << "Fetched depth snapshot for symbol: " <<
    // glz::write_json(snapshot)->c_str() << '\n';

    try {
        ob::observer::BinanceListenerService service{host, port,
                                                     std::move(target)};

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