#include "common/allocator.h"

namespace runtime {

// class BFCAllocator : public Allocator {
//  private:
//   // chunk
//   struct Chunk {
//     Chunk *prev = nullptr;
//     Chunk *next = nullptr;
//     void *ptr = nullptr;
//     int64_t bin_id = 0;
//     int64_t allocation_id = -1;
//     size_t total_size = 0;
//     size_t requested_size = 0;
//     bool in_use() const { return allocation_id == -1; }
//   };
//   // bin
//   struct Bin {
//     size_t bin_size = 0;  // chunk in this bin is [bin_size, next_bin_size)
//   };
//   size_t Round(size_t bytes);
// };

}  // namespace runtime
