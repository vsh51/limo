#include "limo/thread_pool/ThreadPool.hpp"

namespace limo::thread_pool {

ThreadPool::ThreadPool(std::size_t threadCount) {
    if (threadCount == 0) {
        threadCount = 1;
    }

    workers.reserve(threadCount);
    for (std::size_t i = 0; i < threadCount; ++i) {
        workers.emplace_back([this]() { workerLoop(); });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

std::size_t ThreadPool::size() const {
    return workers.size();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (stopping) {
            return;
        }
        stopping = true;
    }
    cv.notify_all();

    for (auto& worker : workers) {
        worker.join();
    }
}

void ThreadPool::enqueue(Task task) {
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (stopping) {
            throw std::runtime_error("ThreadPool is stopping");
        }
        tasks.push(std::move(task));
    }
    cv.notify_one();
}

void ThreadPool::workerLoop() {
    while (true) {
        Task task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]() { return stopping || !tasks.empty(); });
            if (stopping && tasks.empty()) {
                return;
            }
            task = std::move(tasks.front());
            tasks.pop();
        }
        task();
    }
}

} // namespace limo::thread_pool
