#include "common/memory_pool.h"
#include "cpu/std_allocator.h"
#include <cmath>
#include <memory>
#include <mutex>
#include <set>
#include <vector>
#include <algorithm>

namespace runtime {

class BFCMemoryPool : public MemoryPool {
public:
  BFCMemoryPool(std::shared_ptr<DeviceAllocator> allocator, size_t max_bytes = MAX_ALLOCATION_SIZE,
                bool growth = true, bool garbage = false);

  virtual ~BFCMemoryPool();

  void *Allocate(size_t unused_alignment, size_t num_bytes,
                 uint64_t freed_before);
  void Deallocate(void *p);

  static constexpr size_t MIN_REGION_SIZE = 1 << 10;
  static constexpr size_t MIN_ALLOCATION_BITS = 8;
  static constexpr size_t MIN_ALLOCATION_SIZE = 1 << MIN_ALLOCATION_BITS; // 256 bytes
  static constexpr size_t MAX_BIN_NUM = 22;              // 2 ^ (22 + 8) = 2048MB

  // the limit size(max) of allocation 
  // if exceed this limitation may cause overflow or system oom. 
  static constexpr size_t MAX_ALLOCATION_SIZE = 1 << 31; // 2024MB

  // if allocated size in chunk - we needed size > MAX_FRAGMENTATION_SIZE
  // or allocated size in chunk > 2 * we needed size
  // split it into 2 chunks.
  static constexpr size_t MAX_FRAGMENTATION_SIZE = (1 << 27); // 128MB

#ifndef RUNTIME_JUST_LIBRARY
  // only for debug and test
  struct PeepInfo {
    // the num of bin 
    size_t bin_num = 0;
    // free chunks for each bin
    // vector outside is for each bin
    // vector inside is for bin's chunks
    // 1 is allocated_size; 2 is requested_size
    std::vector<std::vector<std::pair<size_t, size_t>>> free_chunks;
    // the num of region
    size_t region_num = 0;
  };
  PeepInfo Peep();
#endif

private:
  typedef short BinId;  // Max_BIN_NUM is 22, so short is enough
  static constexpr short INVALID_BIN = -1;

  // by chunk to split or merge memory space
  // and grouping chunks into bins, so we can get memory space by bins quickly.
  // but management of memory space(alloc/dealloc) is under thre charge of region(region_manager).
  struct Chunk {
    Chunk() = default;
    Chunk(void *p, size_t bytes): ptr(p), allocated_size(bytes) {}
    // all chunks saved in vector,
    // use this handle point to next(prev) memory adjacent chunk
    // so we can split or merge chunks
    std::shared_ptr<Chunk> prev = nullptr;
    std::shared_ptr<Chunk> next = nullptr;
    BinId belong_to = INVALID_BIN;

    size_t allocated_size = 0;
    size_t requested_size = 0;
    void *ptr = nullptr;

    int64_t allocation_count = -1;
    size_t freed_at_count = 0;

    bool in_use() const {
      return allocation_count == -1;
    }

    void mark_free() {
      allocation_count = -1;
    }

    void clear() {
      next = nullptr;
      prev = nullptr;
      belong_to = INVALID_BIN;
      allocation_count = -1;
    }
  };
  typedef std::shared_ptr<Chunk> Handle; // chunk handle

  // by bin to grouping free chunks
  struct Bin {
    Bin() = default;
    explicit Bin(size_t bs) :size_in_bytes(bs) {}

    struct cmp {
      bool operator()(Handle a, Handle b) {
        if (a->allocated_size != b->allocated_size) {
          return a->allocated_size < b->allocated_size;
        }
        return a->ptr < b->ptr;
      }
    };
    // chunk in this bin is [bin_size, next_bin_size)
    size_t size_in_bytes = 0;
    std::set<Handle, cmp> free_chunks;
  };

  class RegionManager {
   public:
    RegionManager() {}
    virtual ~RegionManager() {}

    // continuous memory space
    // every region sliced by MIN_ALLOCATION_SIZE
    // each slice is one handle (one chunk) in the begining.
    // and each handle corresponding to MIN_ALLOCATION_SIZE memory.
    // so adjacent handle can be merge or split dynamically.
    struct Region {
      Region(void *p, size_t s): start_at(p), size_in_bytes(s) {
        end_at = (void *)((char *)start_at + size_in_bytes);
        // assert size_in_bytes % MIN_ALLOCATION_SIZE == 0;
        auto n = (size_in_bytes + MIN_ALLOCATION_SIZE - 1) / MIN_ALLOCATION_SIZE;
        handles.resize(n);
      }
      size_t size_in_bytes = 0;
      void *start_at = nullptr;
      void *end_at = nullptr;
      std::vector<Handle> handles;

      size_t index(const void *p) {
        // space of region divided into index * MIN_ALLOCATION_SIZE
        return ((char *)p - (char *)start_at) >> MIN_ALLOCATION_BITS;
      }
    };

    // insert and remove region to(from) regions
    void InsertRegion(void *ptr, size_t size);
    std::vector<Region>::iterator RemoveRegion(std::vector<Region>::iterator it);

    // set(get) the handle into(from) one of regions
    void SetHandle(const void* p, Handle h);
    Handle GetHandle(const void *p);

    std::vector<Region>& Regions() { return regions_; }
  private:
    // return the region contains p
    Region *GetRegion(const void *p);
    std::vector<Region> regions_;
  };

private:
  // max limit size
  // decided by ctor. (this limit < MAX_ALLOCATION_SIZE) 
  size_t regions_total_max_bytes_ = 0;
  // all regions total allocated size
  size_t regions_total_alloc_bytes_ = 0;
  // current region's size
  size_t curr_region_alloc_bytes_ = 0;

  // flags
  // bool garbage_collection_ = false;
  // bool backpedal_ = false;

  // important members
  std::shared_ptr<DeviceAllocator> allocator_;
  std::shared_ptr<RegionManager> region_manager_;
  std::vector<Bin> bins_;

private:
  bool Extend(size_t unused_alignment, size_t rounded_bytes);
  Handle ChooseChunk(BinId bin_id, size_t rounded_bytes, size_t num_bytes, uint64_t freed_before);

  // shrink current chunk, split into 2 chunks.
  // current chunk size is reserve_bytes
  // and surplus space is belong to the new chunk
  void ShrinkChunk(Handle h, size_t reserve_bytes);

  // expand current chunk
  // merge h2 into current h
  void ExpandChunk(Handle h, Handle h2);

  // expand current chunk.
  // merge the prev and next into current (if possiable).
  void ExpandChunk(Handle h, bool ignore_freed_at);

  size_t GetBinId(size_t bytes);
  size_t Round(size_t bytes);
};

} // namespace runtime
