#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ob::domain {

// Единый scale: 8 знаков после запятой.
// Допущение: для Binance Spot хватает, проверь по exchangeInfo
// (tickSize/stepSize).
inline constexpr int kScale = 8;
inline constexpr std::int64_t kScaleFactor = 100'000'000;

struct Price {
    std::int64_t ticks{};
    auto operator<=>(const Price&) const = default;
};

struct Qty {
    std::int64_t ticks{};
    auto operator<=>(const Qty&) const = default;
    [[nodiscard]] constexpr bool is_zero() const noexcept {
        return ticks == 0;
    }  // qty==0 -> удалить уровень
};

struct SeqId {
    std::int64_t value{};
    auto operator<=>(const SeqId&) const = default;
};

struct Level {
    Price price;
    Qty qty;
};

struct DepthDelta {
    std::string symbol;
    std::int64_t event_time_ms{};
    SeqId first_id;  // U
    SeqId last_id;   // u
    std::vector<Level> bids;
    std::vector<Level> asks;
};

enum class ParseError : std::uint8_t {
    BadFormat,  // пусто, мусор, знак, экспонента, "1.", ".5"
    Overflow,  // не влезает в int64 при scale 1e-8
    PrecisionLoss,  // больше kScale значащих знаков после запятой
    NonPositivePrice,
    BadSeq,      // U/u <= 0 или U > u
    WrongEvent,  // e != "depthUpdate"
    WrongSymbol,
};

constexpr std::string_view to_string(ParseError e) noexcept {
    switch (e) {
        case ParseError::BadFormat:
            return "BadFormat";
        case ParseError::Overflow:
            return "Overflow";
        case ParseError::PrecisionLoss:
            return "PrecisionLoss";
        case ParseError::NonPositivePrice:
            return "NonPositivePrice";
        case ParseError::BadSeq:
            return "BadSeq";
        case ParseError::WrongEvent:
            return "WrongEvent";
        case ParseError::WrongSymbol:
            return "WrongSymbol";
    }
    return "Unknown";
}

}  // namespace ob::domain