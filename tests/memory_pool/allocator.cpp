#include "cpu/std_allocator.h"
#include "cpu/bfc/bfc_allocator.h"
#include "gtest/gtest.h"

TEST(BFC_ALLOCATOR, run) {
  auto allocator = std::make_shared<runtime::STDAllocator>();
  auto pool = std::make_shared<runtime::BFCMemoryPool>(allocator, 1024 * 3);

  // expect status: 1 region; 22 bins; no free chunks in each bin
  // check bins
  ASSERT_EQ(pool->Peep().bin_num, 22);
  // check regions
  ASSERT_EQ(pool->Peep().region_num, 0);
  // check free chunks
  auto free_chunks = pool->Peep().free_chunks;
  ASSERT_EQ(pool->Peep().bin_num, free_chunks.size());
  for (size_t i = 0; i < free_chunks.size(); ++i) {
    auto chunks_of_bin = free_chunks[i];
    for (size_t c = 0; c < chunks_of_bin.size(); ++c) {
      ASSERT_EQ(chunks_of_bin[c].first, 0);
      ASSERT_EQ(chunks_of_bin[c].second, 0);
    }
  }

  auto ptr1 = pool->Allocate(32, 100, 0);
  // expect status: 
  // space has been extended -> 1 region; 22 bins; 1 free chunks in bins[2] (1024)
  // but only 100 requested, after round, 256 requested, so the 1024 chunk splited
  // reserve 256, and erase from free chunks, the chunk of 1024 turn to bin[1](512-1024)
  ASSERT_EQ(pool->Peep().bin_num, 22);
  ASSERT_EQ(pool->Peep().region_num, 1);
  free_chunks = pool->Peep().free_chunks;
  for (size_t i = 0; i < free_chunks.size(); ++i) {
    auto chunks_of_bin = free_chunks[i];
    if (i == 1) {
      ASSERT_EQ(chunks_of_bin.size(), 1);
      ASSERT_EQ(chunks_of_bin[0].first, 768);  // 1024 - 256 = 768
      ASSERT_EQ(chunks_of_bin[0].second, 0);
    } else {
      ASSERT_EQ(chunks_of_bin.size(), 0);
    }
  }

  auto ptr2 = pool->Allocate(32, 300, 0);
  free_chunks = pool->Peep().free_chunks;
  // allocate 300(512 rounded)
  // 768 - 512 < 512 -> so return bin[1] and 768 erased. 
  for (size_t i = 0; i < free_chunks.size(); ++i) {
    auto chunks_of_bin = free_chunks[i];
    ASSERT_EQ(chunks_of_bin.size(), 0);
  }
  // now only 2 chunks: 1 * 256 and 1 * 768

  auto ptr3 = pool->Allocate(32, 1030, 0);
  // region + 1
  ASSERT_EQ(pool->Peep().region_num, 2);
  free_chunks = pool->Peep().free_chunks;
  for (size_t i = 0; i < free_chunks.size(); ++i) {
    auto chunks_of_bin = free_chunks[i];
    ASSERT_EQ(chunks_of_bin.size(), 0);
  }



}

