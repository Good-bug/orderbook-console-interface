// THROUGHPUT ONLY. This says nothing about latency distribution -- see
// spsc_latency.cpp for p50/p99/p99.9. Cores: FH_PROD_CORE / FH_CONS_CORE.
#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <thread>

#include "bench_util.hpp"
#include "fh/spsc_queue.hpp"

static void BM_SpscThroughput(benchmark::State& state) {
    using Queue = fh::SpscQueue<std::uint64_t, (1U << 16)>;
    auto q = std::make_unique<Queue>();
    std::atomic<bool> stop{false};

    std::thread consumer([&] {
        fhbench::pin_this_thread(fhbench::env_int("FH_CONS_CORE", 1));
        std::uint64_t v = 0;
        while (!stop.load(std::memory_order_relaxed)) {
            while (q->try_pop(v)) {
                benchmark::DoNotOptimize(v);
            }
        }
    });

    fhbench::pin_this_thread(fhbench::env_int("FH_PROD_CORE", 0));
    std::uint64_t i = 0;
    for (auto _ : state) {
        while (!q->try_push(i)) {
        }
        ++i;
    }
    stop.store(true, std::memory_order_relaxed);
    consumer.join();
    state.SetItemsProcessed(static_cast<std::int64_t>(state.iterations()));
}
BENCHMARK(BM_SpscThroughput)->UseRealTime();

BENCHMARK_MAIN();
