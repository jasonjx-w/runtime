#ifndef RUNTIME_COMMON_ALLOCATOR_H_
#define RUNTIME_COMMON_ALLOCATOR_H_

#include "logging.h"
#include <iostream>

namespace runtime {

class DeviceAllocator {
 public:
  virtual void *Allocate(size_t unused_alignment, size_t num_bytes) = 0;
  virtual void Deallocate(void *ptr, size_t num_bytes = 0) = 0;
};

}  // namespace runtime

#endif  // RUNTIME_COMMON_ALLOCATOR_H_
