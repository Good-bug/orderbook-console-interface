#pragma once

#include <array>
#include <cstdint>
#include <glaze/glaze.hpp>
#include <string>
#include <vector>

namespace ob::binance::dto {

// Binance шлёт цены и количества строками: ["price", "qty"]
using Level = std::array<std::string, 2>;

// ---------- Запросы ----------
struct SubscribeRequest {
    std::string method{"SUBSCRIBE"};
    std::vector<std::string> params;
    std::int64_t id{1};
};

// ---------- Ответы ----------
// Ack на SUBSCRIBE: {"result":null,"id":1}
struct SubscribeAck {
    std::optional<std::string> result;  // null -> nullopt
    std::int64_t id{};
};

// Diff depth stream
struct DepthUpdate {
    std::string event_type;          // "e"
    std::int64_t event_time{};       // "E", мс
    std::string symbol;              // "s"
    std::int64_t first_update_id{};  // "U"
    std::int64_t last_update_id{};   // "u"
    std::vector<Level> bids;         // "b"
    std::vector<Level> asks;         // "a"
};

// REST-снапшот (понадобится на следующем шаге)
struct DepthSnapshot {
    std::int64_t last_update_id{};  // "lastUpdateId"
    std::vector<Level> bids;
    std::vector<Level> asks;
};

}  // namespace ob::binance::dto

// Маппинг читаемых имён полей на имена в JSON
template <>
struct glz::meta<ob::binance::dto::DepthUpdate> {
    using T = ob::binance::dto::DepthUpdate;
    static constexpr auto value =
        object("e", &T::event_type, "E", &T::event_time, "s", &T::symbol, "U",
               &T::first_update_id, "u", &T::last_update_id, "b", &T::bids, "a",
               &T::asks);
};

template <>
struct glz::meta<ob::binance::dto::DepthSnapshot> {
    using T = ob::binance::dto::DepthSnapshot;
    static constexpr auto value = object("lastUpdateId", &T::last_update_id,
                                         "bids", &T::bids, "asks", &T::asks);
};