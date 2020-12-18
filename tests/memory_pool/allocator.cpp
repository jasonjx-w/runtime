#include "cpu/std_allocator.h"
#include "cpu/bfc/bfc_allocator.h"
#include "gtest/gtest.h"

TEST(BFC_ALLOCATOR, run) {
  auto allocator = std::make_shared<runtime::STDAllocator>();
  runtime::BFCMemoryPool pool(allocator, 536870912); // 512MB

  char* ptr = (char *)pool.Allocate(32, 10485760, 0);
  for (int i = 0; i < 256; ++i) {
    ptr[i] = 'a';
  }
  pool.Deallocate(ptr);
  for (int i = 0; i < 256; ++i) {
    ptr[i] += 1;
  }
}

