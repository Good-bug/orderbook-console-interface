// Cross-core one-way latency through SpscQueue at a fixed offered rate.
//
// Why paced: if the producer runs flat out the queue fills and you measure
// queueing delay, not the cost of the hand-off.
//
// Reported number = t_pop - t_push, i.e. includes ~2 clock reads. The clock
// calibration histogram (back-to-back now()) is printed so you can see that floor.
//
// usage: spsc_latency [msgs=2000000] [warmup=200000] [gap_ns=1000]
// cores: FH_PROD_CORE (default 0), FH_CONS_CORE (default 1)
#include <hdr/hdr_histogram.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <thread>

#include "bench_util.hpp"
#include "fh/spsc_queue.hpp"

namespace {

struct Msg {
    std::uint64_t ts_ns{0};
    std::uint64_t seq{0};
};

hdr_histogram* make_hist() {
    hdr_histogram* h = nullptr;
    // 1 ns .. 10 s, 3 significant digits
    if (hdr_init(1, 10'000'000'000LL, 3, &h) != 0) {
        std::fprintf(stderr, "hdr_init failed\n");
        std::exit(1);
    }
    return h;
}

void print(const char* title, hdr_histogram* h) {
    std::printf("%s\n  p50=%lld  p99=%lld  p99.9=%lld  p99.99=%lld  max=%lld  (ns, n=%lld)\n",
                title, static_cast<long long>(hdr_value_at_percentile(h, 50.0)),
                static_cast<long long>(hdr_value_at_percentile(h, 99.0)),
                static_cast<long long>(hdr_value_at_percentile(h, 99.9)),
                static_cast<long long>(hdr_value_at_percentile(h, 99.99)),
                static_cast<long long>(hdr_max(h)), static_cast<long long>(h->total_count));
}

}  // namespace

int main(int argc, char** argv) {
    const std::uint64_t total = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 2'000'000;
    const std::uint64_t warmup = argc > 2 ? std::strtoull(argv[2], nullptr, 10) : 200'000;
    const std::uint64_t gap_ns = argc > 3 ? std::strtoull(argv[3], nullptr, 10) : 1000;

    // --- clock floor ---
    hdr_histogram* clk = make_hist();
    fhbench::pin_this_thread(fhbench::env_int("FH_PROD_CORE", 0));
    for (int i = 0; i < 1'000'000; ++i) {
        const auto a = fhbench::now_ns();
        const auto b = fhbench::now_ns();
        hdr_record_value(clk, static_cast<std::int64_t>(b - a) + 1);
    }

    // --- queue ---
    auto q = std::make_unique<fh::SpscQueue<Msg, 4096>>();
    hdr_histogram* lat = make_hist();
    std::atomic<std::uint64_t> full_stalls{0};

    std::thread consumer([&] {
        fhbench::pin_this_thread(fhbench::env_int("FH_CONS_CORE", 1));
        Msg m;
        for (std::uint64_t got = 0; got < total;) {
            if (q->try_pop(m)) {
                const auto d = fhbench::now_ns() - m.ts_ns;
                if (m.seq >= warmup) {
                    hdr_record_value(lat, static_cast<std::int64_t>(d) + 1);
                }
                ++got;
            }
        }
    });

    const std::uint64_t t0 = fhbench::now_ns();
    for (std::uint64_t i = 0; i < total; ++i) {
        const std::uint64_t due = t0 + i * gap_ns;
        while (fhbench::now_ns() < due) {
        }
        Msg m{fhbench::now_ns(), i};
        while (!q->try_push(m)) {
            full_stalls.fetch_add(1, std::memory_order_relaxed);
        }
    }
    consumer.join();

    print("clock floor (back-to-back now())", clk);
    print("SPSC one-way latency", lat);
    std::printf("gap=%llu ns, warmup=%llu msgs discarded, producer full-stalls=%llu\n",
                static_cast<unsigned long long>(gap_ns), static_cast<unsigned long long>(warmup),
                static_cast<unsigned long long>(full_stalls.load()));
    std::printf("NOTE: record CPU model, kernel, isolcpus/governor in the README with this output.\n");
    hdr_close(clk);
    hdr_close(lat);
}
