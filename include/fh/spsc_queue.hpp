#pragma once

#include <array>
#include <atomic>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <utility>

namespace fh {

// Hard-coded on purpose: std::hardware_destructive_interference_size triggers
// -Winterference-size on GCC and is not ABI-stable. 64 is right for x86-64;
// revisit for Apple Silicon (128).
inline constexpr std::size_t kCacheLine = 64;

// Bounded wait-free single-producer / single-consumer ring buffer.
//
//  * Capacity is a power of two; indices are monotonic uint64_t and masked on
//    access, so full/empty need no "one slot wasted" trick.
//  * Producer-owned and consumer-owned state sit on separate cache lines.
//  * Each side caches the other side's index and re-reads the shared atomic
//    only when the cached value says full/empty -> fewer cache-line transfers.
//  * Storage is inline. For large Capacity allocate the queue on the heap
//    (std::make_unique), not on the stack.
//
// Contract: exactly one thread calls try_push, exactly one thread calls try_pop.
// Nothing here allocates, locks or throws.
template <typename T, std::size_t Capacity>
class SpscQueue {
    static_assert(Capacity >= 2 && std::has_single_bit(Capacity),
                  "Capacity must be a power of two >= 2");
    static_assert(std::is_default_constructible_v<T>,
                  "T must be default constructible (slots are pre-constructed)");
    static_assert(std::is_nothrow_move_assignable_v<T>);

public:
    SpscQueue() = default;
    SpscQueue(const SpscQueue&) = delete;
    SpscQueue& operator=(const SpscQueue&) = delete;

    // Producer side.
    template <typename U>
    [[nodiscard]] bool try_push(U&& value) noexcept(std::is_nothrow_assignable_v<T&, U&&>) {
        const std::uint64_t head = head_.load(std::memory_order_relaxed);
        if (head - cached_tail_ == Capacity) {
            cached_tail_ = tail_.load(std::memory_order_acquire);
            if (head - cached_tail_ == Capacity) {
                return false;
            }
        }
        buf_[head & kMask] = std::forward<U>(value);
        head_.store(head + 1, std::memory_order_release);
        return true;
    }

    // Consumer side.
    [[nodiscard]] bool try_pop(T& out) noexcept {
        const std::uint64_t tail = tail_.load(std::memory_order_relaxed);
        if (tail == cached_head_) {
            cached_head_ = head_.load(std::memory_order_acquire);
            if (tail == cached_head_) {
                return false;
            }
        }
        out = std::move(buf_[tail & kMask]);
        tail_.store(tail + 1, std::memory_order_release);
        return true;
    }

    static constexpr std::size_t capacity() noexcept { return Capacity; }

private:
    static constexpr std::uint64_t kMask = Capacity - 1;

    // --- producer cache line ---
    alignas(kCacheLine) std::atomic<std::uint64_t> head_{0};
    std::uint64_t cached_tail_{0};

    // --- consumer cache line ---
    alignas(kCacheLine) std::atomic<std::uint64_t> tail_{0};
    std::uint64_t cached_head_{0};

    // --- data ---
    alignas(kCacheLine) std::array<T, Capacity> buf_{};
};

}  // namespace fh
