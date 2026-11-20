# feedhandler

Market data feed handler and real-time order book in C++23.
Exchange depth stream (WebSocket) -> sequence-validated book -> consumers,
featuring an interactive FTXUI terminal user interface, asynchronous CSV recording,
and fixed-point 128-bit arithmetic.

## Overview

- **Real-time feed:** Connects to Binance WebSocket market stream (`stream.binance.com:9443`) for `btcusdt` 100ms depth updates.
- **Order book lifecycle:** Fetches depth snapshot via HTTPS REST, buffers incoming stream deltas during synchronization, validates sequence continuity (`first_update_id == last_update_id + 1`), and automatically triggers resynchronization on gaps.
- **Fixed-point arithmetic:** 128-bit integer fixed-point representation (`__int128_t`) with 8 decimal places scale factor ($10^8$), avoiding floating-point rounding errors.
- **Concurrent architecture:** Dedicated background I/O network thread (Boost.Beast / Boost.Asio), asynchronous CSV logging thread, background worker thread for book maintenance, asynchronous Quill logging to `orderbook.log`, and atomic views (`std::atomic<std::shared_ptr<const BookView>>`) for lock-free UI consumption.
- **Terminal UI:** Live depth ladder rendered in the terminal using FTXUI.

## Binance numeric representation

Binance sends depth prices and quantities as decimal strings. They are retained
as strings only in the JSON wire DTO, then parsed strictly into signed 128-bit
fixed-point values with eight decimal places (`value * 100,000,000`) before
reaching consumers. This supports values beyond `int64` without floating-point
rounding; malformed values, values outside the `int128` range, and non-zero
precision beyond eight decimal places are rejected. Update IDs and sequence numbers
use `SeqId` (`uint64_t`), and timestamps remain `uint64_t` milliseconds.

## Data model and source map

### DTO vs. domain types

- `ob::dto` contains wire-format types: their fields mirror Binance JSON
and preserve exchange encodings. For example, a depth level is two strings
(`price` and `quantity`) because Binance sends decimal numbers as JSON strings.
DTOs are for decoding and encoding messages; application logic should not do
book arithmetic on them.
- `ob::domain` contains exchange-independent application types. Prices and
quantities are validated fixed-point integers (`Price`, `Qty`), and updates/snapshots use the
domain `Level`, `DepthUpdate`, and `DepthSnapshot` types. This gives the order book exact arithmetic and keeps
Binance's JSON representation out of core logic.
- `protocol/binance_convert.hpp` is the boundary between the two: it validates
wire values and converts DTO updates/snapshots into domain values, returning
parse errors instead of silently accepting malformed numbers or sequence IDs.

The normal data flow is:

```text
Binance REST/WebSocket JSON
	-> protocol/binance_dto.hpp (wire DTO)
	-> protocol/binance_convert.hpp (validate and convert)
	-> domain/types.hpp (application domain model)
	-> observer (OrderBook / AsyncDepthUpdateCsvWriter) -> TUI View
```

### Source files

- `src/main.cpp`: application composition root; initializes the writer, order book, listener service, and connects the FTXUI terminal application with the background network thread.
- `src/utils/ui/`: terminal user interface (FTXUI):
  - `tui.hpp`: `OrderBookApp` (interactive rendering loop, background ticker, and graceful shutdown on `q` / `Esc`) and `BackgroundTask` (thread management for the network service).
  - `orderbook_view.hpp`: renders the visual order book depth ladder (asks, bids, spread, and formatted levels).
- `lib/` (core library `feedhandler_lib`, aliased as `feedhandler::lib`):
  - `lib/include/domain/types.hpp`: exchange-independent domain types (`Price`, `Qty`, `Level`, `DepthUpdate`, `DepthSnapshot`, `SeqId`, `ParseError`).
  - `lib/include/protocol/binance_dto.hpp`: Binance JSON DTOs and Glaze field mappings.
  - `lib/include/protocol/binance_convert.hpp`: strict DTO-to-domain conversion and fixed-point parse/format helpers.
  - `lib/include/rest/binance_rest_depth_client.hpp`: HTTPS client that requests a Binance depth snapshot, decodes its DTO, and returns a validated domain snapshot.
  - `lib/include/gateway/binance_listener_service.hpp` and `lib/src/gateway/binance_listener_service.cpp`: Binance WebSocket connection, depth message parsing/conversion, and observer notifications.
  - `lib/include/orderbook/order_book.hpp` and `lib/src/orderbook/order_book.cpp`: order-book observer managing depth state; handles buffering during initial snapshot fetch, sequence validation, automatic resync on gap detection, and thread-safe publishing of `BookView` snapshots.
  - `lib/include/writer/filewriter.hpp`: `AsyncDepthUpdateCsvWriter` observing depth updates and queuing them to disk asynchronously in CSV format (`depth_updates.csv`).
  - `lib/tests/binance_convert_test.cpp`: unit tests verifying strict fixed-point arithmetic and DTO conversion edge cases with Catch2.

### Order Book synchronization logic

1. **Startup / Connection:** When `BinanceListenerService` enters the running state, `OrderBook` sets status to `Initiating` and asynchronously requests a depth snapshot from the Binance REST API (`BinanceRestDepthClient`).
2. **Buffering:** Live WebSocket depth updates received while waiting for the snapshot are queued into an internal buffer.
3. **Snapshot Application:** Once the REST snapshot arrives, book bids and asks are populated from the snapshot, and buffered events older than or matching `snapshot.last_update_id` are discarded.
4. **Sequence Continuity:** The remaining buffered updates and subsequent live updates are validated to ensure sequence continuity (`update.first_update_id == last_update_id + 1`).
5. **Resynchronization on Gap:** If a sequence gap is detected or continuity is broken, `OrderBook` emits a `ResetEvent`, clears its state and buffer, and requests a fresh snapshot.
6. **Thread-Safe Publishing:** The book updates its internal price levels and publishes immutable `BookView` snapshots via `std::atomic<std::shared_ptr<const BookView>>`, providing atomic lock-free reads for the UI rendering loop.

## Build and Run

### Prerequisites

- C++23 compatible compiler (GCC 14+ recommended or Clang 18+)
- CMake 3.25+
- Ninja build system
- [vcpkg](https://github.com/microsoft/vcpkg) with dependencies: `boost-asio`, `boost-beast`, `openssl`, `glaze`, `catch2`, `quill`, `ftxui`

### CMake Presets

Presets are defined in `CMakePresets.json` (with user overrides in `CMakeUserPresets.json`):

```bash
export VCPKG_ROOT=/path/to/vcpkg

# Debug build
cmake --preset debug && cmake --build --preset debug && ctest --preset debug

# Address & Undefined Behavior Sanitizer (ASan + UBSan)
cmake --preset asan && cmake --build --preset asan && ctest --preset asan

# ThreadSanitizer (TSan)
cmake --preset tsan && cmake --build --preset tsan && ctest --preset tsan

# Release build
cmake --preset release && cmake --build --preset release && ctest --preset release
```

### Running

Run the built executable:

```bash
./build/debug/orderbook
# or
./build/release/orderbook
```

Controls:
- Press `q` or `Esc` to quit the application gracefully.
- Depth updates are logged asynchronously to `depth_updates.csv`.
- Order book lifecycle, listener status, depth updates, sequence validation, snapshot buffering/replay, and resynchronization details are logged to `orderbook.log`.

## Benchmark methodology (rules for every number in this repo)

- Report p50 / p99 / p99.9 from HdrHistogram. No bare means.
- Warm-up discarded; threads pinned; hardware + kernel + governor recorded next to results.
- Network latency and internal processing are measured separately.
- Not run in CI (shared runners are noise).

### Hardware used for published numbers
_TODO: CPU model, cores used, kernel, governor, isolcpus, compiler version._

## Experiments that did not work
_No entries yet. Add them as you go: hypothesis -> measurement -> result._
