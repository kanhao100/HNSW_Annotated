#include "hnsw.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <random>
#include <vector>
using namespace std;

/**
 * 随机测试函数 - 用于测试HNSW算法的性能
 * @param numItems 测试数据集大小
 * @param dim 向量维度
 * @param numQueries 查询次数
 * @param K 查询的近邻数量
 */
void randomTest(int numItems, int dim, int numQueries, int K) {
	// 初始化随机数生成器
	default_random_engine generator;
	uniform_real_distribution<double> distribution(0.0,1.0);
	
	// 生成随机测试数据集
	vector<Item> randomItems;
	for (int i = 0; i < numItems; i++) {
		vector<double> temp(dim);
		for (int d = 0; d < dim; d++) {
			temp[d] = distribution(generator);  // 每个维度的值从[0,1]均匀分布中采样
		}
		randomItems.emplace_back(temp);
	}
	// 随机打乱数据集
	random_shuffle(randomItems.begin(), randomItems.end());

	// 构建HNSW图
	// 参数说明: M=10(邻居数量), MMax=30(高层最大邻居数), MMax0=30(底层最大邻居数), efConstruction=10(构建时搜索数量), ml=2(最大层数)
	HNSWGraph myHNSWGraph(10, 30, 30, 10, 2);
	for (int i = 0; i < numItems; i++) {
		if (i % 10000 == 0) cout << i << endl;  // 每处理10000个数据点输出一次进度
		myHNSWGraph.Insert(randomItems[i]);  // 将数据点插入HNSW图
	}
	
	// 性能测试变量
	double total_brute_force_time = 0.0;  // 暴力搜索总时间
	double total_hnsw_time = 0.0;  // HNSW搜索总时间

	cout << "START QUERY" << endl;
	int numHits = 0;  // 记录HNSW算法与暴力搜索结果一致的次数
	for (int i = 0; i < numQueries; i++) {
		// 生成随机查询向量
		vector<double> temp(dim);
		for (int d = 0; d < dim; d++) {
			temp[d] = distribution(generator);
		}
		Item query(temp);

		// 暴力搜索 - 计算查询点与所有数据点的距离并排序
		clock_t begin_time = clock();
		vector<pair<double, int>> distPairs;
		for (int j = 0; j < numItems; j++) {
			if (j == i) continue;  // 跳过自身
			distPairs.emplace_back(query.dist(randomItems[j]), j);  // 计算距离并记录索引
		}
		sort(distPairs.begin(), distPairs.end());  // 按距离排序
		total_brute_force_time += double( clock () - begin_time ) /  CLOCKS_PER_SEC;  // 计算耗时

		// HNSW搜索
		begin_time = clock();
		vector<int> knns = myHNSWGraph.KNNSearch(query, K);  // 使用HNSW算法查找K个最近邻
		total_hnsw_time += double( clock () - begin_time ) /  CLOCKS_PER_SEC;  // 计算耗时

		// 检查HNSW算法的第一个结果是否与暴力搜索一致
		if (knns[0] == distPairs[0].second) numHits++;
	}
	// 输出测试结果: 命中次数, 暴力搜索平均时间, HNSW平均时间
	cout << numHits << " " << total_brute_force_time / numQueries  << " " << total_hnsw_time / numQueries << endl;
}

/**
 * 主函数
 */
int main() {
	// 运行随机测试: 10000个数据点, 4维向量, 100次查询, 查找5个近邻
	randomTest(10000, 4, 100, 5);
	return 0;
}
