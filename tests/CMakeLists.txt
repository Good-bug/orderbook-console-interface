add_executable(spsc_queue_test spsc_queue_test.cpp)
target_link_libraries(spsc_queue_test PRIVATE fh::fh GTest::gtest_main Threads::Threads)
add_test(NAME spsc_queue_test COMMAND spsc_queue_test)
