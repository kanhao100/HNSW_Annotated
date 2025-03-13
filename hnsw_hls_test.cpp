#include "hnsw_hls.h"
#include <stdio.h>
#include <math.h>
#include <time.h>  // 添加time.h头文件，提供clock()和CLOCKS_PER_SEC

// 定义仿真参数
#define N_VECTORS 3344
#define DIM 384
#define NUM_NEIGHBORS 1  // 改名为NUM_NEIGHBORS避免与函数参数冲突
#define N_QUERIES 1600

// 声明为全局变量，这样两个函数可以共享同一个图实例
HNSWGraph graph;

// HLS接口函数 - 用于Vitis HLS综合
void hnsw_search(double query[MAX_DIM], int dim, int k, int results[MAX_NEIGHBORS]) {
    // 移除静态变量，使用全局的graph
    Item q;
    for (int i = 0; i < dim; i++) {
        q.values[i] = query[i];
    }
    
    // 执行搜索
    graph.KNNSearch(q, k, results);
}

// 测试函数 - 随机生成数据并测试HNSW算法
void testHNSW() {
    // 初始化参数
    int M = 32;              // 邻居数量
    int MMax = 32;           // 高层最大邻居数
    int MMax0 = 64;          // 底层最大邻居数
    int efConstruction = 32; // 构建时搜索范围
    int ml = 4;              // 最大层数
    
    int dim = DIM;           // 向量维度
    int numItems = N_VECTORS; // 测试数据量
    int numQueries = N_QUERIES; // 查询次数
    int K = NUM_NEIGHBORS;   // 查询K个近邻
    
    // 使用全局graph变量，而不是创建新的实例
    graph.init(M, MMax, MMax0, efConstruction, ml, dim, 12345);
    
    // 生成随机数据
    Item items[MAX_ITEMS];
    
    printf("生成随机数据...\n");
    for (int i = 0; i < numItems; i++) {
        for (int d = 0; d < dim; d++) {
            items[i].values[d] = graph.randomFloat(); // 从[0,1)均匀分布生成
        }
        
        // 插入数据到图中
        graph.Insert(items[i]);
        
        if (i % 500 == 0) {
            printf("已插入 %d 个数据点\n", i);
        }
    }
    
    printf("开始查询测试...\n");
    int numHits = 0;
    double total_hnsw_time = 0.0;
    double total_bruteforce_time = 0.0;
    
    for (int i = 0; i < numQueries; i++) {
        // 生成随机查询
        double query[MAX_DIM];  // 改用double数组以匹配hnsw_search接口
        for (int d = 0; d < dim; d++) {
            query[d] = graph.randomFloat();
        }
        
        // 使用hnsw_search接口函数进行搜索
        int hnswResults[MAX_NEIGHBORS];
        clock_t begin_time = clock();
        hnsw_search(query, dim, K, hnswResults);
        total_hnsw_time += double(clock() - begin_time) / CLOCKS_PER_SEC;
        
        // 为了暴力搜索，需要将query转换为Item格式
        Item queryItem;
        for (int d = 0; d < dim; d++) {
            queryItem.values[d] = query[d];
        }
        
        // 暴力搜索（线性扫描）
        DistIdPair bruteForceResults[numItems];
        int bruteForceSize = 0;
        
        // 记录暴力搜索时间
        begin_time = clock();
        for (int j = 0; j < numItems; j++) {
            double distance = queryItem.dist(items[j], dim);
            
            // 插入排序
            int pos = bruteForceSize;
            while (pos > 0 && bruteForceResults[pos-1].dist > distance) {
                bruteForceResults[pos] = bruteForceResults[pos-1];
                pos--;
            }
            
            bruteForceResults[pos].dist = distance;
            bruteForceResults[pos].id = j;
            
            if (bruteForceSize < numItems) {
                bruteForceSize++;
            }
        }
        total_bruteforce_time += double(clock() - begin_time) / CLOCKS_PER_SEC;
        
        // 检查结果是否匹配
        if (hnswResults[0] == bruteForceResults[0].id) {
            numHits++;
        }
        
        if (i % 100 == 0) {
            printf("已完成 %d 次查询\n", i);
        }
    }
    
    printf("Top-1 召回率: %d/%d (%.2f%%)\n", 
           numHits, numQueries, (float)numHits / numQueries * 100);
    
    // 输出平均查询时间（毫秒）
    printf("HNSW平均查询时间: %.4f ms\n", 
           (total_hnsw_time / numQueries) * 1000);
    printf("暴力搜索平均查询时间: %.4f ms\n", 
           (total_bruteforce_time / numQueries) * 1000);
    printf("加速比: %.2fx\n", 
            total_bruteforce_time / total_hnsw_time);
}

// 主函数
int main() {
    printf("开始HNSW算法测试...\n");
    printf("数据规模: %d个%d维向量, 查询%d次, K=%d\n", 
            N_VECTORS, DIM, N_QUERIES, NUM_NEIGHBORS);
    testHNSW();
    printf("测试完成\n");
    
    return 0;
} 