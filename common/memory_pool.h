#include "logging.h"
#include <iostream>

namespace runtime {

class MemoryPool {
 public:
  void *Allocate(size_t unused_alignment, size_t num_bytes);
  void Deallocate(void *ptr, size_t num_bytes = 0);
};

}  // namespace runtime
