#ifndef THREADPOOL_H
#define THREADPOOL_H

#include <vector>
#include <thread>
#include <queue>
#include <functional>
#include <condition_variable>


class ThreadPool final {
public:
    ThreadPool(std::size_t numThreads=std::thread::hardware_concurrency());
    ~ThreadPool();

    void send(std::function<void()> task);
    std::size_t numRunningThreads() const;

    void shutdown();

private:
    std::vector<std::thread> threads;

    std::mutex queue_mutex;
    std::queue<std::function<void(void)>> tasks;

    std::condition_variable condition;
    std::atomic<bool> stop_all_threads = false;
    std::atomic<int> runningThreads;
};

#endif // THREADPOOL_H
