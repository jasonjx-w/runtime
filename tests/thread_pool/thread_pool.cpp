#include "common/thread_pool.h"
#include "gtest/gtest.h"
#include <vector>
#include <thread>

TEST(THREAD_POOL, run) {
  ThreadPool pool(4);

  auto sum = [](int i, int j) {
    auto temp = i + j;
    // std::cout << i + j << " = " << i << " + " << j << std::endl;
  };

  auto sub = [](int i, int j) {
    auto temp = i - j;
    // std::cout << i + j << " = " << i << " + " << j << std::endl;
  };

  pool.enqueue(sum, 1, 2);
  pool.enqueue(sum, 2, 3);
  pool.enqueue(sum, 3, 4);
  pool.enqueue(sum, 5, 6);
  pool.enqueue(sum, 6, 7);
  pool.enqueue(sum, 7, 8);
  pool.enqueue(sub, 1, 2);
  pool.enqueue(sub, 2, 3);
  pool.enqueue(sub, 3, 4);
  pool.enqueue(sub, 5, 6);
  pool.enqueue(sub, 6, 7);
  pool.enqueue(sub, 7, 8);
}

