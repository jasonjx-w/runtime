#include "common/allocator.h"
#include "common/memory_pool.h"
#include <set>

namespace runtime {

class BFCMemoryPool : public MemoryPool {
 public:
   BFCMemoryPool(std::shared_ptr<DeviceAllocator> allocator,
                 size_t num_bytes,
                 bool growth = true,
                 bool garbage = false);

   virtual ~BFCMemoryPool() {}

   void *Allocate(size_t unused_alignment,
                  size_t num_bytes,
                  uint64_t freed_before);

 private:
  const size_t MIN_REGION_SIZE = 1 << 10;
  const size_t MIN_ALLOCATION_BITS = 8;
  const size_t MIN_ALLOCATION_SIZE = 1 << MIN_ALLOCATION_BITS; // 256 bytes
  const size_t MAX_BIN_NUM = 22; // 2 ^ (22 + 8) = 2048MB
  const size_t MAX_ALLOCATION_SIZE = 1 << 31; // 2024MB 

  // if allocated size in chunk - we needed size > MAX_FRAGMENTATION_SIZE
  // or allocated size in chunk > 2 * we needed size
  // split it into 2 chunks.
  const size_t MAX_FRAGMENTATION_SIZE = 1 << 27; // 128MB

  struct Context {
    std::mutex mtx;
  };

  class RegionManager {
   public:
    RegionManager() {}
    virtual ~RegionManager() {}
  
    // contiguous memory space
    struct Region {
      Region(void* p, size_t s):
        start_at(p), size_in_bytes(s), end_at((void *)((char *)start_at + size_in_bytes)) {}
      size_t size_in_bytes = 0;
      void* start_at = nullptr;
      void* end_at   = nullptr; 
    };
  
    void AddRegion(void *ptr, size_t size);
    void RemoveRegion();
  
   private:
    std::vector<Region> regions_;
  };

  // chunk
  struct Chunk {
    Chunk() {}
    Chunk(void * p, size_t bytes): ptr(p), allocated_size(bytes) {}
    std::shared_ptr<Chunk> prev = nullptr;
    std::shared_ptr<Chunk> next = nullptr;

    size_t allocated_size = 0;
    size_t requested_size = 0;
    void *ptr = nullptr;

    size_t bin_id = 0;
    int64_t allocation_id = -1;
    bool in_use() const { return allocation_id == -1; }
    size_t freed_at_count = 0;
  };

  // bin
  struct Bin {
    Bin() {}
    explicit Bin(size_t bs): bin_size(bs) {}
    size_t bin_size = 0;  // chunk in this bin is [bin_size, next_bin_size)

    struct cmp {
      bool operator() (std::shared_ptr<Chunk> a, std::shared_ptr<Chunk> b) {
        if (a->allocated_size != b->allocated_size) {
          return a->allocated_size < b->allocated_size;
        }
        return a->ptr < b->ptr;
      }
    };
    std::set<std::shared_ptr<Chunk>, cmp> free_chunks;
  };

  // all regions total size
  size_t regions_total_alloc_bytes_ = 0;
  size_t regions_total_max_bytes_   = 0;
  // current region's size
  size_t curr_region_alloc_bytes_ = 0;
  
  // flags
  bool garbage_collection_ = false;

  // important members
  std::shared_ptr<DeviceAllocator> allocator_;
  std::unique_ptr<RegionManager> region_manager_;
  std::vector<Bin> bins_;

  std::shared_ptr<Chunk> ChooseChunk(int64_t bin_id,
                                     size_t rounded_bytes,
                                     size_t num_bytes,
                                     uint64_t freed_before);
  bool Extend(size_t unused_alignment, size_t rounded_bytes);
  void SplitChunk(std::shared_ptr<Chunk> chunk, size_t bytes);
  size_t Round(size_t bytes);
  size_t GetBinId(size_t bytes);
};

}  // namespace runtime
