#include "bfc_allocator.h"

namespace runtime {

constexpr size_t BFCMemoryPool::MIN_REGION_SIZE;
constexpr size_t BFCMemoryPool::MIN_ALLOCATION_BITS;
constexpr size_t BFCMemoryPool::MIN_ALLOCATION_SIZE;
constexpr size_t BFCMemoryPool::MAX_BIN_NUM;
constexpr size_t BFCMemoryPool::MAX_ALLOCATION_SIZE;
constexpr size_t BFCMemoryPool::MAX_FRAGMENTATION_SIZE;

#ifndef RUNTIME_JUST_LIBRARY
BFCMemoryPool::PeepInfo BFCMemoryPool::Peep() {
  PeepInfo res;
  // bins info
  res.bin_num = bins_.size();
  // free chunks info
  for (auto it : bins_) {
    std::vector<std::pair<size_t, size_t>> temp;
    for (auto c : it.free_chunks) {
      temp.emplace_back(std::make_pair(c->allocated_size, c->requested_size));
    }
    res.free_chunks.emplace_back(temp);
  }
  // regions info
  res.region_num = region_manager_->Regions().size();
  return res;
}
#endif

BFCMemoryPool::BFCMemoryPool(std::shared_ptr<DeviceAllocator> allocator,
                             size_t num_bytes, bool growth, bool garbage) {
  allocator_ = allocator;
  regions_total_max_bytes_ = num_bytes;
  if (growth) {
    curr_region_alloc_bytes_ = std::min(num_bytes, MIN_REGION_SIZE);
  } else {
    curr_region_alloc_bytes_ = Round(num_bytes);
  }

  // init bins
  for (size_t i = 0; i < MAX_BIN_NUM; i++) {
    bins_.emplace_back(Bin(std::pow(2, i) * MIN_ALLOCATION_SIZE));
  }
  // init region manager
  region_manager_ = std::make_shared<RegionManager>();
}

BFCMemoryPool::~BFCMemoryPool() {
  for (auto &item : region_manager_->Regions()) {
    allocator_->Deallocate(item.start_at, item.size_in_bytes);
  }
}

void *BFCMemoryPool::Allocate(size_t unused_alignment, size_t num_bytes,
                              uint64_t freed_before) {
  if (0 == num_bytes) {
    return nullptr;
  }

  size_t rounded_bytes = Round(num_bytes);
  BinId bin_id = GetBinId(rounded_bytes);

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
  return nullptr;
}

void BFCMemoryPool::Deallocate(void *ptr) {
  if (nullptr == ptr) {
    return;
  }

  Handle h = region_manager_->GetHandle(ptr);

  // mark free
  h->mark_free();

  // init region manager
  // merge next/prev into current chunk if possible.
  ExpandChunk(h, false);
}

// allocate new memory space
bool BFCMemoryPool::Extend(size_t unused_alignment, size_t rounded_bytes) {
  size_t available_bytes = regions_total_max_bytes_ - regions_total_alloc_bytes_;
  available_bytes = available_bytes / MIN_ALLOCATION_SIZE * MIN_ALLOCATION_SIZE;

  if (rounded_bytes > available_bytes) {
    // available space in not enough, return.
    // this allocation is not allowed.
    return false;
  }

  // allocation space
  // a bigger space which is 2^x
  bool extended = false;
  while (rounded_bytes > curr_region_alloc_bytes_) {
    curr_region_alloc_bytes_ *= 2;
    extended = true;
  }

  size_t bytes = std::min(curr_region_alloc_bytes_, available_bytes);
  void *ptr = allocator_->Allocate(unused_alignment, bytes);
  if (ptr == nullptr) {
    // allocated failed.
    return false;
  }

  // allocate success
  regions_total_alloc_bytes_ += bytes;

  // insert into region:
  // create new region
  // and insert into region vector
  region_manager_->InsertRegion(ptr, bytes);
  Handle chunk = std::make_shared<Chunk>(ptr, bytes);

  // bind chunk to region
  region_manager_->SetHandle(ptr, chunk);

  // insert this new chunk into bin
  auto bin = &(bins_.at(GetBinId(bytes)));
  bin->free_chunks.insert(chunk);

  if (extended == false) {
    // for next extend, allocate a bigger space.
    curr_region_alloc_bytes_ *= 2;
  }
  
  return true;
}

BFCMemoryPool::Handle BFCMemoryPool::ChooseChunk(BFCMemoryPool::BinId bin_id, size_t rounded_bytes,
                                                 size_t num_bytes, uint64_t freed_before) {
  for (; bin_id < MAX_BIN_NUM; bin_id++) {
    // begin at smallest bin
    // if didn't find free chunk then find in next bigger bin, repeat until find it.
    // if after all bins still didn't find it,
    // return nullptr, this means allocator will allocate new space.
    auto bin = &(bins_.at(bin_id));
    for (auto it = bin->free_chunks.begin(); it != bin->free_chunks.end(); ++it) {
      if (freed_before > 0 && freed_before < (*it)->freed_at_count) {
        continue;
      }

      if ((*it)->allocated_size >= rounded_bytes) {
        // check size, split chunk if need
        if ((*it)->allocated_size >= rounded_bytes * 2 ||
            (*it)->allocated_size - rounded_bytes >= MAX_FRAGMENTATION_SIZE) {
          ShrinkChunk((*it), rounded_bytes);
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

// shrink current chunk, split into 2 chunks.
// current chunk size is reserve_bytes
// and surplus space is belong to the new chunk
void BFCMemoryPool::ShrinkChunk(Handle chunk, size_t reserve_bytes) {
  auto new_chunk = std::make_shared<Chunk>();
  new_chunk->ptr = (void *)((char *)chunk->ptr + reserve_bytes);
  new_chunk->allocated_size = chunk->allocated_size - reserve_bytes;
  
  // resize origin chunk.
  chunk->allocated_size = reserve_bytes;

  // link new chunk and origin chunk.
  // cuz they are adjacent.
  auto temp = chunk->next;
  new_chunk->prev = chunk;
  new_chunk->next = temp;
  chunk->next = temp;

  if (temp != nullptr) {
    temp->prev = new_chunk;
  }

  // bind chunk and region
  region_manager_->SetHandle(new_chunk->ptr, new_chunk);

  // insert new chunk into bin
  BinId bin_id = GetBinId(new_chunk->allocated_size);
  new_chunk->belong_to = bin_id;
  auto bin = &bins_.at(bin_id);
  bin->free_chunks.insert(new_chunk);
}

// expand current chunk
// merge h2 into current h1
void BFCMemoryPool::ExpandChunk(Handle h1, Handle h2) {
  auto h3 = h2->next;
  h1->next = h3;
  if (h3 != nullptr) {
    h3->prev = h1;
  }

  // resize current chunk
  h1->allocated_size += h2->allocated_size;
  h1->freed_at_count = std::max(h1->freed_at_count, h2->freed_at_count);

  // clear handle
  region_manager_->SetHandle(h2->ptr, nullptr);

  // clear h2
  h2->clear();
}

// expand current chunk.
// merge the prev and next into current (if possiable).
void BFCMemoryPool::ExpandChunk(Handle h, bool ignore_freed_at) {
  auto next = h->next;
  auto prev = h->prev;
  if (next != nullptr && !next->in_use()) {
    // remove "next" from bins
    auto bin = &bins_.at(next->belong_to);
    bin->free_chunks.erase(next);
    // merge
    ExpandChunk(h, next);
  }

  if (prev != nullptr && !prev->in_use()) {
    // remove "prev" from bins
    auto bin = &bins_.at(prev->belong_to);
    bin->free_chunks.erase(prev);

    // merge
    ExpandChunk(prev, h);
    
    // insert new chunk into bins
    bin = &(bins_.at(GetBinId(prev->allocated_size)));
    bin->free_chunks.insert(prev);
    h = prev;
  }
}


size_t BFCMemoryPool::Round(size_t bytes) {
  return (bytes + MIN_ALLOCATION_SIZE - 1) / MIN_ALLOCATION_SIZE * MIN_ALLOCATION_SIZE;
}

size_t BFCMemoryPool::GetBinId(size_t bytes) {
  auto log2 = [](size_t n) -> size_t {
    int64_t r = 0;
    while (n > 0) {
      r++;
      n = n >> 1;
    }
    return r - 1;
  };
  // bin id:
  //      0  |  256 bytes
  //      1  |  512 bytes
  //      ..
  //      22 | 2048 MB
  size_t v = std::max<size_t>(bytes, MIN_ALLOCATION_SIZE) >> MIN_ALLOCATION_BITS;
  BinId b = std::min(MAX_BIN_NUM - 1, log2(v));
  return b;
}

// add region to regions(vector)
// and sort region by ptr
void BFCMemoryPool::RegionManager::InsertRegion(void *ptr, size_t size) {
  auto it = std::upper_bound(regions_.begin(), regions_.end(), ptr, [](void *p, Region r) { return p < r.end_at; });
  regions_.insert(it, Region(ptr, size));
}

// by iterator, easy to use
std::vector<BFCMemoryPool::RegionManager::Region>::iterator
BFCMemoryPool::RegionManager::RemoveRegion(std::vector<BFCMemoryPool::RegionManager::Region>::iterator it) {
  return regions_.erase(it);
}

// return the region contains p
// if not found, return nullptr
BFCMemoryPool::RegionManager::Region *BFCMemoryPool::RegionManager::GetRegion(const void *ptr) {
  auto it = std::upper_bound(regions_.begin(), regions_.end(), ptr,
                             [](const void *p, Region r) { return p < r.end_at; });
  if (it != regions_.end()) {
    return &(*it);
  }
  return nullptr;
}

void BFCMemoryPool::RegionManager::SetHandle(const void *p, Handle h) {
  auto region = GetRegion(p);
  region->handles.at(region->index(p)) = h;
}

BFCMemoryPool::Handle BFCMemoryPool::RegionManager::GetHandle(const void *p) {
  auto region = GetRegion(p);
  return region->handles.at(region->index(p));
}

} // namespace runtime
