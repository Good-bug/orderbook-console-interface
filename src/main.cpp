#include <version>
#include "protocol/binance_convert.hpp"

#if !defined(__cpp_lib_expected) || __cpp_lib_expected < 202202L
#error \
    "std::expected is unavailable: GCC 12+, Clang 16+ (with a compatible standard library), or MSVC 19.33+ and -std=c++23 are required"
#endif

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <format>
#include <ftxui/component/component.hpp>
#include <ftxui/component/event.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <iostream>
#include <string>
#include <unordered_map>

#include "gateway/binance_listener_service.hpp"
#include "orderbook/books/naive_book.hpp"
#include "orderbook/order_book.hpp"
#include "utils/ui/tui.hpp"
#include "writer/filewriter.hpp"

using namespace ob;

namespace {
std::string formatDepthUpdateCsvRow(const domain::DepthUpdate& update) {
    std::string bids;
    for (const auto& level : update.bids) {
        if (!bids.empty()) {
            bids += ';';
        }
        bids += std::format("{},{}",
                            ob::convert::format_fixed_int124(level.price.value),
                            ob::convert::format_fixed_int124(level.qty.value));
    }

    std::string asks;
    for (const auto& level : update.asks) {
        if (!asks.empty()) {
            asks += ';';
        }
        asks += std::format("{},{}",
                            ob::convert::format_fixed_int124(level.price.value),
                            ob::convert::format_fixed_int124(level.qty.value));
    }

    return std::format(
        "{},{},{},{},{},{}", update.symbol, update.first_update_id.value,
        update.last_update_id.value, update.event_time_ms, bids, asks);
}
}  // namespace


int main() {
    ob::writer::AsyncDepthUpdateCsvWriter csv{"depth_updates.csv", formatDepthUpdateCsvRow};
    ob::orderbook::OrderBook<ob::orderbook::NaiveBook> book;

    ob::observer::BinanceListenerService service{
        "stream.binance.com", "9443", {{"btcusdt", 100}}};
    service.subscribe(&csv);
    service.subscribe(&book);
    book.start();

    utils::ui::OrderBookApp app([&] { return book.view(); }, /*depth=*/10);

    utils::ui::BackgroundTask net(
        [&] { service.run(); },     // Network thread
        [&] { service.stop(); },    // Stop the service
        [&] { app.quit(); });       // Close the UI when the connection ends

    app.run();                      // Blocks until q/Esc is pressed or the connection closes
    net.stop_and_join();

    if (net.error()) {
        std::cerr << *net.error() << '\n';
        return 1;
    }
}