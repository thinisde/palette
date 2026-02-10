#include "palette/services/thread_pool.hpp"
#include <algorithm>
#include <condition_variable>
#include <cstdlib>
#include <exception>
#include <mutex>
#include <queue>
#include <thread>
#include <utility>
#include <vector>

namespace palette::services {

size_t resolve_worker_thread_count(const char *env_name, size_t fallback) {
    const size_t default_fallback = fallback == 0 ? 4 : fallback;
    const char *raw = std::getenv(env_name);
    if (!raw || *raw == '\0') {
        return default_fallback;
    }

    char *end = nullptr;
    const long parsed = std::strtol(raw, &end, 10);
    if (!end || *end != '\0' || parsed <= 0) {
        return default_fallback;
    }

    const size_t max_threads = 64;
    return std::clamp(static_cast<size_t>(parsed), static_cast<size_t>(1),
                      max_threads);
}

struct thread_pool::impl {
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mutex;
    std::condition_variable cv;
    bool stopping = false;
};

thread_pool::thread_pool(size_t thread_count) : state_(new impl{}) {
    const size_t resolved = std::max<size_t>(1, thread_count);
    state_->workers.reserve(resolved);

    for (size_t i = 0; i < resolved; ++i) {
        state_->workers.emplace_back([this]() {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(state_->mutex);
                    state_->cv.wait(lock, [this]() {
                        return state_->stopping || !state_->tasks.empty();
                    });

                    if (state_->stopping && state_->tasks.empty()) {
                        return;
                    }

                    task = std::move(state_->tasks.front());
                    state_->tasks.pop();
                }

                try {
                    task();
                } catch (...) {
                    // Keep the worker alive even if a handler throws.
                }
            }
        });
    }
}

thread_pool::~thread_pool() {
    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        state_->stopping = true;
    }
    state_->cv.notify_all();

    for (auto &worker : state_->workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    delete state_;
}

void thread_pool::enqueue(std::function<void()> task) {
    if (!task) {
        return;
    }

    {
        std::lock_guard<std::mutex> lock(state_->mutex);
        if (state_->stopping) {
            return;
        }
        state_->tasks.emplace(std::move(task));
    }
    state_->cv.notify_one();
}

size_t thread_pool::size() const { return state_->workers.size(); }

} // namespace palette::services
