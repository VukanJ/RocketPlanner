#include "ThreadPool.h"

ThreadPool::ThreadPool(std::size_t numThreads) {
    threads.reserve(numThreads);
    for (std::size_t i = 0; i < numThreads; ++i) {
        threads.emplace_back([this] () {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(queue_mutex);
                    condition.wait(lock, [this] {return stop_all_threads || !tasks.empty(); });

                    if (stop_all_threads && tasks.empty()) {
                        return;
                    }

                    ++runningThreads;
                    task = std::move(tasks.front());
                    tasks.pop();
                }

                task();

                --runningThreads;
            }
        });
    }
}

ThreadPool::~ThreadPool() {
    shutdown();
}

void ThreadPool::send(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        tasks.emplace(std::move(task));
    }
    condition.notify_one();
}

void ThreadPool::shutdown() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        stop_all_threads = true;
    }
    condition.notify_all();
    for (auto& worker : threads) {
        worker.join();
    }
}

std::size_t ThreadPool::numRunningThreads() const {
    return runningThreads;
}

