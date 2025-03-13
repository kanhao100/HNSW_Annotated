# HNSW算法详细分析

## 1. 算法概述

HNSW (Hierarchical Navigable Small World) 是一种用于高效近似最近邻 (AKNN) 搜索的算法，基于小世界图的概念。该算法由Malkov和Yashunin在2016年提出，论文标题为《Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs》。

HNSW 算法的核心思想是构建一个多层次的图结构，其中：
- 底层包含所有数据点
- 上层是下层的稀疏子集
- 通过这种层次结构，可以实现对数级别的搜索复杂度

## 2. 数据结构

从代码实现来看，HNSW 算法主要使用以下数据结构：

### 2.1 Item 结构体

```cpp
struct Item {
    // 构造函数，初始化向量值
    Item(vector<double> _values):values(_values) {}
    
    // 向量数据
    vector<double> values;
    
    // 计算与另一个数据点的距离（L2欧氏距离的平方）
    double dist(Item& other) {
        double result = 0.0;
        for (int i = 0; i < values.size(); i++) 
            result += (values[i] - other.values[i]) * (values[i] - other.values[i]);
        return result;
    }
};
```

`Item` 结构体表示一个数据点，包含：
- 向量数据 `values`
- 距离计算函数 `dist`（使用L2欧氏距离的平方）

### 2.2 HNSWGraph 结构体

```cpp
struct HNSWGraph {
    // 邻居数量参数
    int M;
    // 高层（>=1）最大邻居数量
    int MMax;
    // 底层（0层）最大邻居数量
    int MMax0;
    // 构建时的搜索范围
    int efConstruction;
    // 最大层数
    int ml;

    // 数据项数量
    int itemNum;
    // 存储所有数据项的向量
    vector<Item> items;
    // 每层的邻接表，存储图的连接关系
    vector<unordered_map<int, vector<int>>> layerEdgeLists;
    // 入口节点ID
    int enterNode;

    // 随机数生成器
    default_random_engine generator;
    
    // 方法声明...
};
```

`HNSWGraph` 结构体是算法的核心，包含：
- 算法参数：`M`, `MMax`, `MMax0`, `efConstruction`, `ml`
- 数据存储：`items`（所有数据点）
- 图结构：`layerEdgeLists`（多层图的邻接表）
- 入口点：`enterNode`（搜索的起始点）

## 3. 关键参数解释

HNSW 算法的性能很大程度上取决于以下参数：

- **M**：每个节点添加的邻居数量，影响图的连接度
- **MMax**：高层（>=1）最大邻居数量，控制高层图的稀疏程度
- **MMax0**：底层（0层）最大邻居数量，通常大于MMax以提高精度
- **efConstruction**：构建时的搜索范围，影响构建质量
- **ml**：最大层数，影响层次结构的高度

## 4. 核心算法流程

### 4.1 图的构建过程

HNSW 的构建过程是逐点插入的，每个新点的插入步骤如下：

1. **随机确定层级**：
```cpp
int l = 0;
uniform_real_distribution<double> distribution(0.0,1.0);
while(l < ml && (1.0 / ml <= distribution(generator))) {
    l++;
    if (layerEdgeLists.size() <= l) 
        layerEdgeLists.push_back(unordered_map<int, vector<int>>());
}
```
这段代码使用概率分布确定新节点所在的最高层级，层级分布遵循指数衰减规律。

2. **搜索连接点**：
```cpp
int ep = enterNode;
for (int i = maxLyer; i > l; i--) 
    ep = searchLayer(q, ep, 1, i)[0];
```
从最高层开始，逐层向下搜索，找到合适的连接点。

3. **建立连接**：
```cpp
for (int i = min(l, maxLyer); i >= 0; i--) {
    // 确定当前层的最大邻居数
    int MM = l == 0 ? MMax0 : MMax;
    
    // 在当前层搜索efConstruction个最近邻
    vector<int> neighbors = searchLayer(q, ep, efConstruction, i);
    
    // 选择最近的M个点作为邻居
    vector<int> selectedNeighbors = vector<int>(neighbors.begin(), 
                                   neighbors.begin()+min(int(neighbors.size()), M));
    
    // 为新节点添加边连接到选定的邻居
    for (int n: selectedNeighbors) addEdge(n, nid, i);
    
    // 检查并调整邻居的连接...
}
```
从节点所在的最高层开始，逐层向下建立连接。

4. **邻居调整**：
```cpp
for (int n: selectedNeighbors) {
    if (layerEdgeLists[i][n].size() > MM) {
        // 计算邻居节点与其所有邻居的距离
        vector<pair<double, int>> distPairs;
        for (int nn: layerEdgeLists[i][n]) 
            distPairs.emplace_back(items[n].dist(items[nn]), nn);
        
        // 按距离排序
        sort(distPairs.begin(), distPairs.end());
        
        // 只保留最近的MM个邻居
        layerEdgeLists[i][n].clear();
        for (int d = 0; d < min(int(distPairs.size()), MM); d++) 
            layerEdgeLists[i][n].push_back(distPairs[d].second);
    }
}
```
如果某个节点的邻居数量超过限制，则保留距离最近的邻居。

### 4.2 搜索过程

HNSW 的搜索过程分为两个阶段：

1. **层间导航**：
```cpp
int maxLyer = layerEdgeLists.size() - 1;
int ep = enterNode;
for (int l = maxLyer; l >= 1; l--) 
    ep = searchLayer(q, ep, 1, l)[0];
```
从最高层开始，逐层向下找到最近的点作为下一层的入口点。

2. **底层精确搜索**：
```cpp
return searchLayer(q, ep, K, 0);
```
在底层（包含所有点）进行精确搜索，找到K个最近邻。

### 4.3 层内搜索算法

层内搜索是HNSW的核心，实现了贪婪搜索与局部搜索的结合：

```cpp
vector<int> HNSWGraph::searchLayer(Item& q, int ep, int ef, int lc) {
    // 候选集合，按距离排序
    set<pair<double, int>> candidates;
    // 当前找到的最近邻集合
    set<pair<double, int>> nearestNeighbors;
    // 已访问节点集合
    unordered_set<int> isVisited;

    // 计算查询点与入口点的距离
    double td = q.dist(items[ep]);
    candidates.insert(make_pair(td, ep));
    nearestNeighbors.insert(make_pair(td, ep));
    isVisited.insert(ep);
    
    // 主搜索循环
    while (!candidates.empty()) {
        // 取出距离最近的候选点
        auto ci = candidates.begin(); candidates.erase(candidates.begin());
        int nid = ci->second;
        
        // 获取当前最近邻集合中距离最远的点
        auto fi = nearestNeighbors.end(); fi--;
        
        // 如果当前候选点比最远的最近邻还远，则停止搜索
        if (ci->first > fi->first) break;
        
        // 遍历当前节点的所有邻居
        for (int ed: layerEdgeLists[lc][nid]) {
            // 跳过已访问节点
            if (isVisited.find(ed) != isVisited.end()) continue;
            
            fi = nearestNeighbors.end(); fi--;
            isVisited.insert(ed);
            
            // 计算查询点与邻居的距离
            td = q.dist(items[ed]);
            
            // 如果邻居比当前最远的最近邻更近，或者最近邻集合未满，则加入候选集和最近邻集合
            if ((td < fi->first) || nearestNeighbors.size() < ef) {
                candidates.insert(make_pair(td, ed));
                nearestNeighbors.insert(make_pair(td, ed));
                
                // 如果最近邻集合超过ef，则移除最远的点
                if (nearestNeighbors.size() > ef) nearestNeighbors.erase(fi);
            }
        }
    }
    
    // 将最近邻集合转换为ID列表返回
    vector<int> results;
    for(auto &p: nearestNeighbors) results.push_back(p.second);
    return results;
}
```

这个搜索算法的关键点：
- 使用两个集合：`candidates`（候选点）和`nearestNeighbors`（当前最近邻）
- 每次从候选集中取出最近的点，探索其邻居
- 如果找到更近的点，更新两个集合
- 当候选集为空或当前候选点比最远的最近邻还远时，停止搜索

## 5. 算法复杂度分析

### 5.1 时间复杂度

- **构建复杂度**：O(n log n)，其中n是数据点数量
  - 每个点的插入需要O(log n)的搜索时间
  - 总共插入n个点

- **查询复杂度**：O(log n)
  - 层间导航需要O(log n)时间
  - 每层搜索的复杂度受M和ef参数控制

### 5.2 空间复杂度

- **存储复杂度**：O(n M)
  - 每个点平均有M个邻居
  - 总共n个点

## 6. 算法优势与局限性

### 6.1 优势

1. **高效搜索**：对数级别的搜索复杂度，远优于线性扫描
2. **高质量近似**：通过调整参数，可以在速度和精度之间取得良好平衡
3. **增量构建**：支持动态添加新数据点
4. **适应性强**：适用于各种距离度量和数据分布

### 6.2 局限性

1. **参数敏感**：性能很大程度上依赖于参数选择
2. **内存消耗**：比一些其他ANN算法（如LSH）需要更多内存
3. **构建时间**：构建高质量图需要较长时间

## 7. 实验结果分析

根据README中的实验结果：

| 数据量   | Top-1 召回率 | 暴力搜索 (ms/q) | HNSW (ms/q) | 
| -------- | ----------- | -------------- | ----------- |
| 10k      | 100%        | 2.72           | 0.15        |
| 100k     | 100%        | 35.32          | 0.26        |
| 1M       | 100%        | 483.74         | 0.42        |

可以看出：

1. **精度**：HNSW在所有测试中都达到了100%的Top-1召回率
2. **速度**：随着数据量增加，HNSW的速度优势越来越明显
   - 10k数据：加速约18倍
   - 100k数据：加速约136倍
   - 1M数据：加速约1152倍
3. **扩展性**：HNSW的查询时间随数据量增长非常缓慢，展现出优秀的扩展性

## 8. 代码实现特点

本实现是HNSW算法的一个简化版本，具有以下特点：

1. **简洁实现**：使用STL容器和算法，代码结构清晰
2. **无优化**：未包含论文中提到的一些优化技巧
3. **简单邻居选择**：仅实现了基本的邻居选择策略（SELECT-NEIGHBORS-SIMPLE）



## 9. 总结

HNSW算法是一种高效的近似最近邻搜索方法，通过构建多层次的小世界图结构，实现了对数级别的搜索复杂度。本实现虽然简化，但仍然展示了HNSW算法的核心思想和优秀性能。通过调整参数，HNSW可以在不同的应用场景中取得速度和精度的良好平衡。 