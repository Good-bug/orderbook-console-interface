#pragma once
#include <algorithm>
#include <cstdio>
#include <ftxui/dom/elements.hpp>
#include <string>

#include "domain/types.hpp"
#include "orderbook/order_book.hpp"
using namespace ob;

// ---- Adapt to your domain::Level if needed ----
inline double to_price(const domain::Level& l) {
    return static_cast<double>(l.price.value) / 100.0;
}
inline double to_qty(const domain::Level& l) {
    return static_cast<double>(l.qty.value) / 1000.0;
}
// ---------------------------------------------

namespace ob::utils::ui {
using namespace ftxui;

inline std::string fmt(double v, int prec) {
    char buf[32];
    std::snprintf(buf, sizeof buf, "%.*f", prec, v);
    return buf;
}

inline Element row(const domain::Level& l, double max_qty, Color color) {
    const double q = to_qty(l);
    return hbox({
        text(fmt(to_price(l), 2)) | align_right | size(WIDTH, EQUAL, 12) |
            ::ftxui::color(color),
        text(fmt(q, 3)) | align_right | size(WIDTH, EQUAL, 12),
        text(" "),
        gauge(max_qty > 0 ? static_cast<float>(q / max_qty) : 0.f) |
            ::ftxui::color(color) | flex,
    });
}

inline Element render_book(const orderbook::BookView& v, std::size_t depth) {
    const std::size_t nb = std::min(depth, v.bids.size());
    const std::size_t na = std::min(depth, v.asks.size());

    double max_qty = 0;
    for (std::size_t i = 0; i < nb; ++i) {
        max_qty = std::max(max_qty, to_qty(v.bids[i]));
    }
    for (std::size_t i = 0; i < na; ++i) {
        max_qty = std::max(max_qty, to_qty(v.asks[i]));
    }

    Elements rows;
    rows.push_back(hbox({
                       text("PRICE") | align_right | size(WIDTH, EQUAL, 12),
                       text("QTY") | align_right | size(WIDTH, EQUAL, 12),
                       filler(),
                   }) |
                   bold);
    rows.push_back(separator());

    // Asks: worst price at the top, best price next to the spread.
    for (std::size_t i = na; i-- > 0;) {
        rows.push_back(row(v.asks[i], max_qty, Color::Red));
    }

    // Spread
    std::string spread = "—";
    if (nb && na) {
        spread = fmt(to_price(v.asks[0]) - to_price(v.bids[0]), 2);
    }
    rows.push_back(separator());
    rows.push_back(text("spread " + spread) | hcenter | dim);
    rows.push_back(separator());

    for (std::size_t i = 0; i < nb; ++i) {
        rows.push_back(row(v.bids[i], max_qty, Color::Green));
    }

    auto title =
        text(" Order Book  #" + std::to_string(v.last_update_id.value) + " ") |
        bold;
    return window(title, vbox(std::move(rows))) | size(WIDTH, GREATER_THAN, 50);
}

}  // namespace ui