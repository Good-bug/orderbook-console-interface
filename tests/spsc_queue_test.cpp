#include <gtest/gtest.h>

#include <cstdint>
#include <thread>

#include "fh/spsc_queue.hpp"

TEST(SpscQueue, PopFromEmptyFails) {
    fh::SpscQueue<int, 4> q;
    int v = -1;
    EXPECT_FALSE(q.try_pop(v));
    EXPECT_EQ(v, -1);
}

TEST(SpscQueue, PushToFullFailsThenRecovers) {
    fh::SpscQueue<int, 4> q;
    for (int i = 0; i < 4; ++i) {
        ASSERT_TRUE(q.try_push(i));
    }
    EXPECT_FALSE(q.try_push(99));
    int v = 0;
    ASSERT_TRUE(q.try_pop(v));
    EXPECT_EQ(v, 0);
    EXPECT_TRUE(q.try_push(99));
}

TEST(SpscQueue, WrapAroundKeepsFifo) {
    fh::SpscQueue<int, 4> q;
    int next_in = 0;
    int next_out = 0;
    for (int round = 0; round < 1000; ++round) {
        for (int i = 0; i < 3; ++i) {
            ASSERT_TRUE(q.try_push(next_in++));
        }
        for (int i = 0; i < 3; ++i) {
            int v = -1;
            ASSERT_TRUE(q.try_pop(v));
            ASSERT_EQ(v, next_out++);
        }
    }
}

// The one that matters under TSan.
TEST(SpscQueue, TwoThreadsPreserveOrder) {
    constexpr std::uint64_t kCount = 1'000'000;
    fh::SpscQueue<std::uint64_t, 1024> q;

    std::thread producer([&q] {
        for (std::uint64_t i = 0; i < kCount; ++i) {
            while (!q.try_push(i)) {
                std::this_thread::yield();
            }
        }
    });

    std::uint64_t expected = 0;
    while (expected < kCount) {
        std::uint64_t v = 0;
        if (q.try_pop(v)) {
            ASSERT_EQ(v, expected);
            ++expected;
        } else {
            std::this_thread::yield();
        }
    }
    producer.join();
}
