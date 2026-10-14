/**
 * @file types.hpp
 * @brief Domain types used by the feed handler.
 */

#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace ob::domain {

/**
 * @brief Signed fixed-point integer used for prices and quantities.
 */
using FixedInt = __int128_t;

/**
 * @brief Unsigned fixed-point integer used during calculations.
 */
using UnsignedFixedInt = __uint128_t;

/**
 * @brief Number of decimal places used by fixed-point values.
 */
inline constexpr int kScale = 8;

/**
 * @brief Scale factor for converting fixed-point tick values to decimal values.
 */
inline constexpr FixedInt kScaleFactor = 100'000'000;

/**
 * @brief Represents a price in fixed-point tick units.
 *
 * @details The stored value in @c ticks is interpreted as a decimal price by
 * dividing it by @c kScaleFactor.
 */
struct Price {
    FixedInt ticks{};
    auto operator<=>(const Price&) const = default;
};

/**
 * @brief Represents a quantity in fixed-point tick units.
 */
struct Qty {
    FixedInt ticks{};
    auto operator<=>(const Qty&) const = default;

    /**
     * @return true if the quantity is exactly zero.
     */
    [[nodiscard]] constexpr bool is_zero() const noexcept { return ticks == 0; }
};

/**
 * @brief Sequence identifier used for ordering and validation.
 */
struct SeqId {
    std::int64_t value{};
    auto operator<=>(const SeqId&) const = default;
};

/**
 * @brief A single price/quantity level in the order book.
 */
struct Level {
    Price price;
    Qty qty;
};

/**
 * @brief Incremental order book update for a symbol.
 *
 * @details Contains the symbol, event time, sequence range, and the bid/ask
 * changes contained in a single update message.
 */
struct DepthDelta {
    std::string symbol;
    std::int64_t event_time_ms{};
    SeqId first_update_id;  ///< First sequence identifier in the delta.
    SeqId last_update_id;   ///< Last sequence identifier in the delta.
    std::vector<Level> bids;
    std::vector<Level> asks;
};

/**
 * @brief Full order book snapshot for a symbol.
 */
struct DepthSnapshot {
    SeqId last_update_id;  ///< Last applied sequence identifier.
    std::vector<Level> bids;
    std::vector<Level> asks;
};

/**
 * @brief Error reasons generated while parsing feed messages.
 */
enum class ParseError : std::uint8_t {
    BadFormat,
    Overflow,
    PrecisionLoss,
    NonPositivePrice,
    BadSeq,
    WrongEvent,
};

/**
 * @brief Converts a @c ParseError to a short string label.
 *
 * @param e Error code to describe.
 * @return String representation of the error.
 */
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
    }
    return "Unknown";
}

}  // namespace ob::domain