#ifndef RUNTIME_CPU_STD_ALLOCATOR_H_
#define RUNTIME_CPU_STD_ALLOCATOR_H_

#include "common/allocator.h"
#include <stdlib.h>

namespace runtime {

class STDAllocator : public DeviceAllocator {
 public:
  void *Allocate(size_t alignment, size_t num_bytes) {
    return malloc(num_bytes);
  }

  void Deallocate(void *ptr, size_t num_bytes) {
    free(ptr);
  }
};

}  // namespace runtime

#endif  // RUNTIME_CPU_STD_ALLOCATOR_H_
