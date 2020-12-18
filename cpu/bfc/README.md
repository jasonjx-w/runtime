# 原理和算法

## 1.内存池
内存池借用bfc算法(Best-Fit with Coalescing). 该算法是dlmalloc的一个简单版本. 

### 核心数据结构

#### 1.Region
region指代一块连续的内存(即一次malloc操作).

每个region的空间, 将会被切成多个不同的chunk, 根据实际情况这些chunk的尺寸也会动态变化(split/merge).

这些chunk将会挂到对应的bin上, 方便索引.

当找不到合适的chunk后, 将会申请一块新的region, 一般新申请的region将会是之前region的两倍大.

多个region将通过RegionManager统一管理.

#### 2.chunk
本实现使用chunk管理指针. 为减少浪费, 一个chunk可以分裂(split)或合并(merge). 

多个同尺寸的chunk可以组成一个双向链表(逻辑上的), 以方便迅速遍历. 

```
struct Chunk {
    Chunk *prev = NULL;  // 指向前驱
    Chunk *next = NULL;  // 指向后继

    size_t bin_id = 0;         // 该chunk对应的bin的id.
    int64_t allocation_id = 0;  // 默认为0; 被分配后, 则为对应的id.
    
    size_t allocated_size = 0;   // 当前分配的空间(chunk的总尺寸)
    size_t requested_size = 0;   // 用户使用的空间(<=chunk的总尺寸)
};
```

#### 3.Bin
为快速寻找所需的chunk, 使用bin的概念将chunk按尺寸分组, 即每个bin下有一个或多个尺寸一致的chunk.

|  bin0  |  bin1  |   ...  |  binx  |
|  ----  |  ----  |  ----  |  ----  |
|   256  |   512  |   ...  | 1024MB |
| chunk0 | chunkn |   ...  |   ...  |
| chunk1 | chunkm |   ...  |   ...  |

### 执行流程
