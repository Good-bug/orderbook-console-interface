# feedhandler

Market data feed handler + order book in C++20.
Target: exchange depth stream (WebSocket) -> sequence-checked book -> consumer,
with a naive (`std::map`) and an optimized book, SPSC hand-off, replay mode, benchmarks.

**Status:** skeleton. Only `fh::SpscQueue` exists.

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
