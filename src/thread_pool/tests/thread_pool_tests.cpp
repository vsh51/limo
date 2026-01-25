#include "limo/thread_pool/ThreadPool.hpp"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>

using limo::thread_pool::ThreadPool;

TEST(ThreadPoolTests, ExecutesSubmittedTasks) {
    ThreadPool pool(2);
    std::future<int> future = pool.submit([]() { return 42; });
    EXPECT_EQ(future.get(), 42);
}

TEST(ThreadPoolTests, RunsMultipleTasks) {
    ThreadPool pool(3);
    std::atomic<int> counter{0};

    std::future<void> f1 = pool.submit([&counter]() { counter.fetch_add(1); });
    std::future<void> f2 = pool.submit([&counter]() { counter.fetch_add(1); });
    std::future<void> f3 = pool.submit([&counter]() { counter.fetch_add(1); });

    f1.get();
    f2.get();
    f3.get();

    EXPECT_EQ(counter.load(), 3);
}

TEST(ThreadPoolTests, ShutdownPreventsNewTasks) {
    ThreadPool pool(1);
    pool.shutdown();

    EXPECT_THROW(pool.submit([]() { return 1; }), std::runtime_error);
}

TEST(ThreadPoolTests, ShutdownIsIdempotent) {
    ThreadPool pool(2);
    pool.shutdown();
    EXPECT_NO_THROW(pool.shutdown());
}

TEST(ThreadPoolTests, ShutdownProcessesQueuedTasks) {
    ThreadPool pool(1);
    std::atomic<int> counter{0};

    std::future<void> future = pool.submit([&counter]() { counter.fetch_add(1); });
    pool.shutdown();

    EXPECT_NO_THROW(future.get());
    EXPECT_EQ(counter.load(), 1);
}

TEST(ThreadPoolTests, WorkersWaitUntilTasksArrive) {
    ThreadPool pool(1);
    std::atomic<int> value{0};

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    std::future<int> future = pool.submit([&value]() { value.store(7); return 7; });

    EXPECT_EQ(future.get(), 7);
    EXPECT_EQ(value.load(), 7);
}

TEST(ThreadPoolTests, UsesAtLeastOneWorker) {
    ThreadPool pool(0);
    EXPECT_GE(pool.size(), 1u);
}

TEST(ThreadPoolTests, SubmitsCallableWithArguments) {
    ThreadPool pool(2);
    std::function<int(int, int)> add = [](int a, int b) { return a + b; };

    std::future<int> future = pool.submit(add, 3, 4);

    EXPECT_EQ(future.get(), 7);
}

TEST(ThreadPoolTests, SubmitsSharedPtrArgument) {
    ThreadPool pool(1);
    std::shared_ptr<int> payload = std::make_shared<int>(5);

    std::future<int> future = pool.submit(
        [](const std::shared_ptr<int>& value) { return *value * 2; }, payload);

    EXPECT_EQ(future.get(), 10);
    EXPECT_EQ(*payload, 5);
}
