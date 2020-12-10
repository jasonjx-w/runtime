#include "thread_pool.h"

ThreadPool::ThreadPool(size_t thread_count) {
  ctx_ = std::make_shared<Context>();

  auto work = [ctx = ctx_] {
    std::unique_lock<std::mutex> lk(ctx->mtx);
    for (;;) {
      if (!ctx->tasks.empty()) {
        auto task = std::move(ctx->tasks.front());
        ctx->tasks.pop();
        lk.unlock();
        task();
        lk.lock();
      } else if (ctx->is_shutdown) {
        break;
      } else {
        ctx->cond.wait(lk);
      }
    }
  };

  for (size_t i = 0; i < thread_count; ++i) {
    ctx_->workers.emplace_back(work);
  }
}

ThreadPool::~ThreadPool() {
  if (nullptr != ctx_) {
    {
      std::lock_guard<std::mutex> lk(ctx_->mtx);
      ctx_->is_shutdown = true;
    }
    ctx_->cond.notify_all();
  }
  for (std::thread &worker : ctx_->workers) {
    worker.join();
  }
}
