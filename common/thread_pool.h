#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <queue>
#include <thread>

class ThreadPool {
 public:
  ThreadPool() = default;
  ThreadPool(ThreadPool&&) = default;
  explicit ThreadPool(size_t thread_count);
  ~ThreadPool();

  template<typename F, typename... Args>
  void enqueue(F&& f, Args&&... args) {
    auto task = std::make_shared<std::packaged_task<void()>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    {
      std::lock_guard<std::mutex> lk(ctx_->mtx);
      ctx_->tasks.emplace([task]() {(*task)();});
    }
    ctx_->cond.notify_all();
  }

 private:
  struct Context {
    std::mutex mtx;
    std::condition_variable cond;
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> workers;
    // for join(), if not, caller need care about
    // when all these tasks finish, then dtor this threadpool. 
    // but this is not necessary.
    bool is_shutdown = false;
  };
  std::shared_ptr<Context> ctx_;
};
