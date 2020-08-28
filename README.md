# Runtime方法
C++运行时资源管理模块


# 关于
用于管理运行时的资源, 线程池和内存池.

实现时考虑profiling和debug, 提供对应的方法.


# 简述
## 1.文件结构
```
|-- runtime
    |-- allocator.h   # 提供对外统一的抽象接口
    |-- threadpool.h  # 提供对外统一的抽象接口
    |-- cpu/          # cpu资源管理, 不同设备的运行时资源管理实现
    |    |-- xxx.h
    |    |-- xxx.cpp
    |  
    |-- gpu/       # gpu资源管理
    |-- tests/     # 测试代码
    |-- tools/     # 工具和脚本

```

# 原理和算法
## 1.内存池
内存池借用bfc算法(Best-Fit with Coalescing). 该算法是dlmalloc的一个简单版本. 

### 核心数据结构
#### 1.Chunk
bfc算法使用Chunk为最小单位保存数据, 根据需要一个Chunk可以分裂(split)或合并(merge). 多个Chunk组成一个双向链表(逻辑上的). 
```
struct Chunk {
    Chunk *prev = NULL;  // 指向前驱
    Chunk *next = NULL;  // 指向后继

    int64_t bin_id = 0;         // 该chunk对应的bin的id.
    int64_t allocation_id = 0;  // 默认为0; 被分配后, 则为对应的id.
    
    size_t malloced_size = 0;   // 当前分配的空间(chunk的总尺寸)
    size_t requested_size = 0;  // 用户使用的空间(<=chunk的总尺寸)
};
```
#### 2.bin
上述的Chunk自身大小不定, 使用bin的概念将Chunk按尺寸分组, 即每个bin下有一个或多个(尺寸在一定范围内的)的Chunk.
|  bin0  |  bin1  |  bin2  |
|  ----  |  ----  |  ----  |
|   256  |   512  |  1024  |
| chunk0 | chunkn |   ...  |
| chunk1 | chunkm |   ...  |

### 执行流程

### 分支管理备忘
#### 原则(GitHub Flow)
1. 一个master, 保证随时可部署.
2. feature和bugfix, 均拉独立分支进行修改.
3. 积极提交代码, 并说清思路和修改理由.
4. 开发分支进master时, 使用pull request机制. 
5. 稳定commit上打好tag
 
参考: http://scottchacon.com/2011/08/31/github-flow.html

#### 操作
1. 拉分支
```
git checkout -b develop master  // 从master拉出develop分支
```

2. 合分支
```
git checkout master
git merge --no-ff develop   // 将develop合到当前master分支
```

3. 打tag
```
git tag -a 1.2
```

4. 删除本地无用分支(同步至远端状态)
```
git remote prune origin
```
