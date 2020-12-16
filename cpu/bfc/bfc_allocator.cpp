#include "bfc_allocator.h"

namespace runtime {

BFCMemoryPool::BFCMemoryPool(std::shared_ptr<DeviceAllocator> allocator,
                             size_t num_bytes,
                             bool growth,
                             bool garbage) {
  allocator_ = allocator;
  garbage_collection_ = garbage;
  if (growth) {
    curr_region_alloc_bytes_ = std::min(num_bytes, MIN_REGION_SIZE);
  } else {
    curr_region_alloc_bytes_ = Round(num_bytes);
  }

  regions_total_max_bytes_ = num_bytes;

  for (size_t i = 0; i < MAX_BIN_NUM; i++) {
    bins_.emplace_back(Bin(std::pow(2, i) * MIN_ALLOCATION_SIZE));
  }
}

size_t BFCMemoryPool::Round(size_t bytes) {
  return (bytes + MIN_ALLOCATION_SIZE - 1) * MIN_ALLOCATION_SIZE / MIN_ALLOCATION_SIZE;
}

size_t BFCMemoryPool::GetBinId(size_t bytes) {
  auto log2 = [](size_t n) -> size_t {
    int64_t r = 0;
    while(n > 0) {
      r ++;
      n = n >> 1;
    }
    return r - 1;
  };
  // bin id:
  //      0  |  256 bytes 
  //      1  |  512 bytes 
  //      ..
  //      22 | 2048 MB
  uint64_t v = std::max<size_t>(bytes, MIN_ALLOCATION_SIZE) >> MIN_ALLOCATION_BITS;
  int64_t b = std::min(MAX_BIN_NUM -1, log2(v));
  return b;
}

void BFCMemoryPool::SplitChunk(std::shared_ptr<Chunk> chunk,
                               size_t reserve_bytes) {
  auto new_chunk = std::make_shared<Chunk>();
  new_chunk->ptr = (void *)((char *)chunk->ptr + reserve_bytes);
  
  new_chunk->allocated_size = chunk->allocated_size - reserve_bytes;
  chunk->allocated_size = reserve_bytes;

  new_chunk->freed_at_count = chunk->freed_at_count;

  auto temp = chunk->next;
  new_chunk->prev = chunk;
  new_chunk->next = chunk->next;
  chunk->next = temp;

  if (temp != nullptr) {
    temp->prev = new_chunk;
  }

  // insest to bin
  auto bin_id = GetBinId(new_chunk->allocated_size);
  new_chunk->bin_id = bin_id;
  auto bin = &bins_.at(bin_id);
  bin->free_chunks.insert(new_chunk);
}

void *BFCMemoryPool::Allocate(size_t unused_alignment, size_t num_bytes, uint64_t freed_before) {
  if (0 == num_bytes) {
    return nullptr;
  }

  size_t rounded_bytes = Round(num_bytes);
  int64_t bin_id = GetBinId(rounded_bytes);

  // 1. find free chunk.
  auto chunk = ChooseChunk(bin_id, rounded_bytes, num_bytes, freed_before);
  if (chunk != nullptr) {
    // got ptr
    return chunk->ptr;
  }

  // didn't got free chunk
  // extend space
  if (Extend(unused_alignment, rounded_bytes)) {
    auto chunk = ChooseChunk(bin_id, rounded_bytes, num_bytes, freed_before);
    if (chunk != nullptr) {
      return chunk->ptr;
    }
  }

  // didn't got free chunk
  // and extend space failed.
  //
  //
  return nullptr;
}

bool BFCMemoryPool::Extend(size_t unused_alignment, size_t rounded_bytes) {
  size_t available_bytes = regions_total_max_bytes_ - regions_total_alloc_bytes_;
  available_bytes = available_bytes / MIN_ALLOCATION_SIZE * MIN_ALLOCATION_SIZE;

  if (rounded_bytes > available_bytes) {
    // available space in not enough.
    return false;
  }

  bool extended = false;
  while (rounded_bytes > curr_region_alloc_bytes_) {
    curr_region_alloc_bytes_ *= 2;
    extended = true;
  }

  size_t bytes = std::min(curr_region_alloc_bytes_, available_bytes);
  void* ptr = allocator_->Allocate(unused_alignment, bytes);
  if (ptr == nullptr) {
    return false;
  }
  regions_total_alloc_bytes_ += bytes;

  region_manager_->AddRegion(ptr, bytes);
  auto bin = bins_.at(GetBinId(bytes));
  bin.free_chunks.insert(std::make_shared<Chunk>(ptr, bytes));

  if (extended == false) {
    // for next extend, allocate a bigger space.
    curr_region_alloc_bytes_ *= 2;
  }
  return true;
}

std::shared_ptr<BFCMemoryPool::Chunk>
BFCMemoryPool::ChooseChunk(int64_t bin_id, size_t rounded_bytes, size_t num_bytes, uint64_t freed_before) {
  for (;bin_id < MAX_BIN_NUM; bin_id++) {
    // begin at smallest bin
    // if didn't find free chunk then find in next bigger bin, repeat until find it.
    // if after all bins still didn't find it,
    // return nullptr, this means allocator will allocate new space.
    auto bin = &(bins_.at(bin_id)); 
    for (auto it = bin->free_chunks.begin(); it != bin->free_chunks.end(); ++it) {
      if (freed_before > 0 && freed_before < (*it)->freed_at_count) {
        continue;
      }

      if ((*it)->allocated_size > rounded_bytes) {
        // check size, split chunk if need
        if ((*it)->allocated_size >= rounded_bytes * 2 ||
            (*it)->allocated_size - rounded_bytes >= MAX_FRAGMENTATION_SIZE) {
          SplitChunk((*it), rounded_bytes);
        } 
        (*it)->requested_size = num_bytes;
        // 3. update allocator record
        //
        // remove chunk from free_chunks
        bin->free_chunks.erase(it);
        return (*it);
      } 
      // next free chunk
    }
    // next bin
  }

  // don't find chunk
  return nullptr;
}

}  // namespace runtime
