# feedhandler

Market data feed handler + order book in C++20.
Target: exchange depth stream (WebSocket) -> sequence-checked book -> consumer,
with a naive (`std::map`) and an optimized book, SPSC hand-off, replay mode, benchmarks.

**Status:** skeleton. Only `fh::SpscQueue` exists.

## Binance numeric representation

Binance sends depth prices and quantities as decimal strings. They are retained
as strings only in the JSON wire DTO, then parsed strictly into signed 128-bit
fixed-point values with eight decimal places (`value * 100,000,000`) before
reaching consumers. This supports values beyond `int64` without floating-point
rounding; malformed values, values outside the `int128` range, and non-zero
precision beyond eight decimal places are rejected. Update IDs and timestamps
remain `int64`.

## Data model and source map

### DTO vs. domain types

`ob::dto` contains wire-format types: their fields mirror Binance JSON
and preserve exchange encodings. For example, a depth level is two strings
(`price` and `quantity`) because Binance sends decimal numbers as JSON strings.
DTOs are for decoding and encoding messages; application logic should not do
book arithmetic on them.

`ob::domain` contains exchange-independent application types. Prices and
quantities are validated fixed-point integers, and updates/snapshots use the
domain `Level` type. This gives the order book exact arithmetic and keeps
Binance's JSON representation out of core logic.

`protocol/binance_convert.hpp` is the boundary between the two: it validates
wire values and converts DTO updates/snapshots into domain values, returning
parse errors instead of silently accepting malformed numbers or sequence IDs.

The normal data flow is:

```text
Binance REST/WebSocket JSON
	-> protocol/binance_dto.hpp (wire DTO)
	-> protocol/binance_convert.hpp (validate and convert)
	-> domain/types.hpp (application model)
	-> observer / order book / writer
```

### Source files

- `src/main.cpp`: application composition root; configures clients, writer, and
	observer subscriptions, then starts the feed.
- `src/domain/types.hpp`: exchange-independent types such as `Price`, `Qty`,
	`Level`, `DepthDelta`, `DepthSnapshot`, and `ParseError`.
- `src/protocol/binance_dto.hpp`: Binance JSON DTOs and Glaze field mappings.
- `src/protocol/binance_convert.hpp`: strict DTO-to-domain conversion and
	fixed-point parse/format helpers.
- `src/rest/binance_rest_depth_client.hpp`: HTTPS client that requests a Binance depth
	snapshot, decodes its DTO, and returns a validated domain snapshot.
- `src/listener/binance_listener_service.hpp` and
	`src/listener/binance_listener_service.cpp`: Binance WebSocket
	connection, depth-message parsing/conversion, and observer notifications.
- `src/orderbook/order_book.hpp`: order-book observer; currently a skeleton where
	snapshot and delta application logic belongs.
- `src/writer/async_depth_update_csv_writer.hpp`: asynchronous CSV writer observing domain depth
	deltas and flushing queued rows to disk.

The REST snapshot fetcher and WebSocket listener are separate today. The
snapshot is fetched in `main.cpp`, but it is not yet applied to `OrderBook`;
sequence synchronization and reconnect/resnapshot handling are also future
work.

## Build

```bash
export VCPKG_ROOT=/path/to/vcpkg
export CC=clang CXX=clang++
cmake --preset asan-ubsan && cmake --build --preset asan-ubsan && ctest --preset asan-ubsan
cmake --preset tsan       && cmake --build --preset tsan       && ctest --preset tsan
cmake --preset release    && cmake --build --preset release
```

## Benchmark methodology (rules for every number in this repo)

- Report p50 / p99 / p99.9 from HdrHistogram. No bare means.
- Warm-up discarded; threads pinned; hardware + kernel + governor recorded next to results.
- Network latency and internal processing are measured separately.
- Not run in CI (shared runners are noise).

### Hardware used for published numbers
_TODO: CPU model, cores used, kernel, governor, isolcpus, compiler version._

## Что пробовал и не сработало
_Пока пусто. Заполнять по ходу: гипотеза -> замер -> результат._
