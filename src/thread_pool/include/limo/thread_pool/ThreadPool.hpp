#pragma once

#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <vector>

namespace limo::thread_pool {

/**
 * @brief A simple thread pool for executing tasks concurrently.
 *
 * Allows submitting tasks that return futures, managing a pool of worker threads
 * to execute them. Supports graceful shutdown to finish queued tasks before stopping.
 *
 * @author Volodymyr Shpyrka
 */
class ThreadPool {
public:
    using Task = std::function<void()>;

    explicit ThreadPool(std::size_t threadCount = std::thread::hardware_concurrency());
    ~ThreadPool();

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
    ThreadPool(ThreadPool&&) = delete;
    ThreadPool& operator=(ThreadPool&&) = delete;

    std::size_t size() const;
    void shutdown();

    /**
     * @brief Submit a callable for execution on the thread pool - variadic template.
     *
     * Accepts any callable object (function, lambda, functor, member function via std::bind)
     * and its arguments, schedules it on the pool, and returns a future to retrieve the
     * result when the task finishes.
     *
     * std::packaged_task<ReturnType()> wraps a zero-argument callable that returns ReturnType;
     * it stores the result so the associated std::future<ReturnType> can retrieve it.
     * 
     * @tparam F Callable type. May be a function pointer, lambda, or functor.
     * @tparam Args Argument types forwarded to the callable.
     *
     * The return type of the callable is deduced as:
     * - ReturnType = std::invoke_result_t<F, Args...>
     * - The returned future is std::future<ReturnType>
     *
     * @param f Callable to execute.
     * @param args Arguments passed to the callable.
     * @return std::future<ReturnType> Future representing the result of the task.
     * @throws std::runtime_error if the pool is shutting down and cannot accept tasks.
     */
    template <typename F, typename... Args>
    std::future<std::invoke_result_t<F, Args...>> submit(F&& f, Args&&... args) {
        using ReturnType = std::invoke_result_t<F, Args...>;
        std::shared_ptr<std::packaged_task<ReturnType()>>
        task = std::make_shared<std::packaged_task<ReturnType()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );
        std::future<ReturnType> result = task->get_future();

        enqueue([task]() { (*task)(); });
        return result;
    }

private:
    void enqueue(Task task);
    void workerLoop();

    std::vector<std::thread> workers;
    std::queue<Task> tasks;
    mutable std::mutex mutex;
    std::condition_variable cv;
    bool stopping{false};
};

} // namespace limo::thread_pool
