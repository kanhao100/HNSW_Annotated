# HNSW算法的Vitis HLS实现

这是一个为Vitis HLS优化的HNSW (Hierarchical Navigable Small World) 算法实现。该实现避免了动态内存分配和STL容器的使用，使其适合在FPGA上进行硬件加速。

## 文件说明

- `hnsw_hls.h`: 头文件，包含所有数据结构和函数声明
- `hnsw_hls.cpp`: 实现文件，包含所有函数的实现
- `hnsw_hls_test.cpp`: 测试文件，用于验证算法的正确性
- `Makefile_hls`: 用于编译测试程序的Makefile

## 主要修改

相比原始的HNSW实现，本版本做了以下修改以适应HLS的要求：

1. **替换动态容器**:
   - 使用固定大小的数组替代 `vector`
   - 使用自定义的 `NeighborList` 结构替代 `vector<int>`
   - 使用自定义的 `CandidateSet` 结构替代 `set<pair<double, int>>`
   - 使用布尔数组替代 `unordered_set<int>`

2. **避免动态内存分配**:
   - 所有数据结构都使用预定义的最大大小
   - 通过宏定义控制最大容量

3. **简化随机数生成**:
   - 使用简单的线性同余法生成随机数，避免使用 `<random>` 库

## 参数配置

可以通过修改 `hnsw_hls.h` 中的宏定义来调整算法参数：

```cpp
#define MAX_DIM 16          // 最大向量维度
#define MAX_ITEMS 10000     // 最大数据项数量
#define MAX_LAYERS 10       // 最大层数
#define MAX_NEIGHBORS 50    // 每个节点的最大邻居数
#define MAX_CANDIDATES 100  // 搜索时的最大候选点数量
```

## 在Vitis HLS中使用

1. **创建新项目**:
   - 在Vitis HLS中创建新项目
   - 添加 `hnsw_hls.h` 和 `hnsw_hls.cpp` 文件

2. **指定顶层函数**:
   - 使用 `hnsw_search` 函数作为顶层函数
   - 函数原型: `void hnsw_search(double query[MAX_DIM], int dim, int K, int results[MAX_NEIGHBORS])`

3. **添加指令**:
   - 根据需要添加HLS指令（如流水线、展开循环等）
   - 例如，可以在距离计算循环中添加 `#pragma HLS PIPELINE` 以提高性能

4. **综合与实现**:
   - 运行C仿真确保功能正确
   - 运行C综合生成RTL
   - 导出RTL并集成到您的设计中

## 编译和测试

使用提供的Makefile编译测试程序：

```bash
make -f Makefile_hls
./hnsw_hls_test
```

测试程序会生成随机数据，构建HNSW图，然后执行查询测试，并与暴力搜索结果进行比较。

## 性能注意事项

1. **内存访问**:
   - 多维数组的访问可能导致性能瓶颈
   - 考虑使用HLS指令优化内存访问模式

2. **循环优化**:
   - 搜索算法中的循环可以通过流水线和展开来优化
   - 但要注意资源使用和时序约束

3. **参数调整**:
   - 根据具体应用调整MAX_*参数
   - 较大的值会增加资源使用，但可能提高准确性

## 限制

1. 固定大小的数据结构限制了可处理的数据规模
2. 简化的随机数生成可能影响图的质量
3. 没有实现删除操作

## 可能的优化方向

1. 使用HLS指令进一步优化性能
2. 实现更高效的内存布局
3. 添加更多参数控制，以便在资源使用和性能之间取得平衡 