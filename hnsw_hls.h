#ifndef HNSW_HLS_H
#define HNSW_HLS_H

// 定义常量参数
#define MAX_DIM 384          // 最大向量维度
#define MAX_ITEMS 4000       // 最大数据项数量，增加以支持3344个向量
#define MAX_LAYERS 10        // 最大层数
#define MAX_NEIGHBORS 64     // 每个节点的最大邻居数，增加以支持更大的MMax0
#define MAX_CANDIDATES 200   // 搜索时的最大候选点数量，增加以支持更复杂的搜索

/**
 * 数据项结构体 - 表示一个向量数据点
 */
struct Item {
    // 向量数据，使用固定大小数组
    double values[MAX_DIM];
    
    // 计算与另一个数据点的距离（L2欧氏距离的平方）
    double dist(const Item& other, int dim) const {
        double result = 0.0;
        for (int i = 0; i < dim; i++) {
            double diff = values[i] - other.values[i];
            result += diff * diff;
        }
        return result;
    }
};

/**
 * 邻居列表结构体 - 替代vector<int>
 */
struct NeighborList {
    int data[MAX_NEIGHBORS]; // 邻居ID数组
    int size;                // 当前邻居数量
    
    // 初始化
    void init() {
        size = 0;
    }
    
    // 添加邻居
    void push_back(int neighbor) {
        if (size < MAX_NEIGHBORS) {
            data[size++] = neighbor;
        }
    }
    
    // 清空邻居列表
    void clear() {
        size = 0;
    }
    
    // 获取指定位置的邻居
    int& operator[](int index) {
        return data[index];
    }
    
    // 获取指定位置的邻居（常量版本）
    const int& operator[](int index) const {
        return data[index];
    }
};

/**
 * 距离-ID对结构体 - 替代pair<double, int>
 */
struct DistIdPair {
    double dist;  // 距离
    int id;       // 节点ID
    
    // 比较运算符，用于排序
    bool operator<(const DistIdPair& other) const {
        return dist < other.dist;
    }
};

/**
 * 候选集合结构体 - 替代set<pair<double, int>>
 * 实现为有序数组
 */
struct CandidateSet {
    DistIdPair data[MAX_CANDIDATES];
    int size;
    
    // 初始化
    void init() {
        size = 0;
    }
    
    // 插入新元素并保持有序
    void insert(double dist, int id) {
        if (size >= MAX_CANDIDATES) return;
        
        // 找到插入位置
        int pos = size;
        while (pos > 0 && data[pos-1].dist > dist) {
            data[pos] = data[pos-1];
            pos--;
        }
        
        // 插入新元素
        data[pos].dist = dist;
        data[pos].id = id;
        size++;
    }
    
    // 移除最小元素（第一个元素）
    void pop_front() {
        if (size > 0) {
            for (int i = 0; i < size - 1; i++) {
                data[i] = data[i+1];
            }
            size--;
        }
    }
    
    // 移除最大元素（最后一个元素）
    void pop_back() {
        if (size > 0) {
            size--;
        }
    }
    
    // 检查是否为空
    bool empty() const {
        return size == 0;
    }
    
    // 获取最小元素（第一个元素）
    const DistIdPair& front() const {
        return data[0];
    }
    
    // 获取最大元素（最后一个元素）
    const DistIdPair& back() const {
        return data[size-1];
    }
};

/**
 * 访问集合结构体 - 替代unordered_set<int>
 * 实现为简单的标记数组
 */
struct VisitedSet {
    bool visited[MAX_ITEMS];
    
    // 初始化
    void init() {
        for (int i = 0; i < MAX_ITEMS; i++) {
            visited[i] = false;
        }
    }
    
    // 插入元素
    void insert(int id) {
        if (id < MAX_ITEMS) {
            visited[id] = true;
        }
    }
    
    // 检查元素是否存在
    bool contains(int id) const {
        if (id < MAX_ITEMS) {
            return visited[id];
        }
        return false;
    }
};

/**
 * HNSW图结构体 - 实现层次化可导航小世界图
 */
struct HNSWGraph {
    // 参数
    int M;              // 邻居数量参数
    int MMax;           // 高层（>=1）最大邻居数量
    int MMax0;          // 底层（0层）最大邻居数量
    int efConstruction; // 构建时的搜索范围
    int ml;             // 最大层数
    int dim;            // 向量维度
    
    // 数据项数量
    int itemNum;
    
    // 存储所有数据项的向量
    Item items[MAX_ITEMS];
    
    // 每层的邻接表，存储图的连接关系
    // 替代vector<unordered_map<int, vector<int>>>
    NeighborList layerEdgeLists[MAX_LAYERS][MAX_ITEMS];
    
    // 每层的节点数量
    int layerNodeCount[MAX_LAYERS];
    
    // 入口节点ID
    int enterNode;
    
    // 随机数种子
    unsigned int randomSeed;
    
    /**
     * 构造函数
     */
    void init(int _M, int _MMax, int _MMax0, int _efConstruction, int _ml, int _dim, unsigned int seed) {
        M = _M;
        MMax = _MMax;
        MMax0 = _MMax0;
        efConstruction = _efConstruction;
        ml = _ml;
        dim = _dim;
        itemNum = 0;
        randomSeed = seed;
        
        // 初始化层节点计数
        for (int i = 0; i < MAX_LAYERS; i++) {
            layerNodeCount[i] = 0;
        }
        
        // 初始化第0层
        layerNodeCount[0] = 0;
    }
    
    /**
     * 添加边 - 在指定层连接两个节点
     */
    void addEdge(int st, int ed, int lc);
    
    /**
     * 在指定层搜索最近邻
     */
    void searchLayer(const Item& q, int ep, int ef, int lc, int* results, int& resultSize);
    
    /**
     * 插入新数据项到图中
     */
    void Insert(const Item& q);
    
    /**
     * K近邻搜索 - 查找K个最近邻
     */
    void KNNSearch(const Item& q, int K, int* results);
    
    /**
     * 生成随机数 [0, 1)
     */
    double randomFloat() {
        randomSeed = (randomSeed * 1103515245 + 12345) & 0x7fffffff;
        return ((double)randomSeed / (double)0x7fffffff);
    }
};

#endif 