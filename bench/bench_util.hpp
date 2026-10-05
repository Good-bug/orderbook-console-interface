#pragma once

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#ifdef __linux__
#include <pthread.h>
#include <sched.h>
#endif

namespace fhbench {

inline std::uint64_t now_ns() noexcept {
    return static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch())
            .count());
}

inline int env_int(const char* name, int fallback) {
    const char* s = std::getenv(name);
    return s != nullptr ? std::atoi(s) : fallback;
}

// Pins the calling thread. Returns false (and warns) if it could not.
// A benchmark that silently runs unpinned is a benchmark you can't trust.
inline bool pin_this_thread(int core) {
#ifdef __linux__
    cpu_set_t set;
    CPU_ZERO(&set);
    CPU_SET(static_cast<unsigned>(core), &set);
    if (pthread_setaffinity_np(pthread_self(), sizeof(set), &set) == 0) {
        return true;
    }
#endif
    std::fprintf(stderr, "WARNING: could not pin thread to core %d\n", core);
    return false;
}

}  // namespace fhbench
