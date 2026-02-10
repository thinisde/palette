#pragma once
#include <cstddef>
#include <functional>

namespace palette::services {

size_t resolve_worker_thread_count(const char *env_name, size_t fallback);

class thread_pool {
  public:
    explicit thread_pool(size_t thread_count);
    ~thread_pool();

    thread_pool(const thread_pool &) = delete;
    thread_pool &operator=(const thread_pool &) = delete;

    void enqueue(std::function<void()> task);
    size_t size() const;

  private:
    struct impl;
    impl *state_;
};

} // namespace palette::services
