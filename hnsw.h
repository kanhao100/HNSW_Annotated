#ifndef HNSW_H
#define HNSW_H

#include <random>
#include <vector>
#include <unordered_map>
#include <iostream>
using namespace std;

/**
 * 数据项结构体 - 表示一个向量数据点
 */
struct Item {
	// 构造函数，初始化向量值
	Item(vector<double> _values):values(_values) {}
	
	// 向量数据
	vector<double> values;
	
	// 计算与另一个数据点的距离（L2欧氏距离的平方）
	double dist(Item& other) {
		double result = 0.0;
		for (int i = 0; i < values.size(); i++) result += (values[i] - other.values[i]) * (values[i] - other.values[i]);
		return result;
	}
};

/**
 * HNSW图结构体 - 实现层次化可导航小世界图
 */
struct HNSWGraph {
	/**
	 * 构造函数
	 * @param _M 每个节点添加的邻居数量
	 * @param _MMax 高层（>=1）最大邻居数量
	 * @param _MMax0 底层（0层）最大邻居数量
	 * @param _efConstruction 构建时的搜索范围
	 * @param _ml 最大层数
	 */
	HNSWGraph(int _M, int _MMax, int _MMax0, int _efConstruction, int _ml):M(_M),MMax(_MMax),MMax0(_MMax0),efConstruction(_efConstruction),ml(_ml){
		layerEdgeLists.push_back(unordered_map<int, vector<int>>());
	}
	
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

	// 方法声明
	/**
	 * 添加边 - 在指定层连接两个节点
	 * @param st 起始节点ID
	 * @param ed 目标节点ID
	 * @param lc 层级
	 */
	void addEdge(int st, int ed, int lc);
	
	/**
	 * 在指定层搜索最近邻
	 * @param q 查询数据项
	 * @param ep 入口点
	 * @param ef 返回的邻居数量
	 * @param lc 搜索的层级
	 * @return 最近邻节点ID列表
	 */
	vector<int> searchLayer(Item& q, int ep, int ef, int lc);
	
	/**
	 * 插入新数据项到图中
	 * @param q 要插入的数据项
	 */
	void Insert(Item& q);
	
	/**
	 * K近邻搜索 - 查找K个最近邻
	 * @param q 查询数据项
	 * @param K 返回的近邻数量
	 * @return K个最近邻节点ID列表
	 */
	vector<int> KNNSearch(Item& q, int K);

	/**
	 * 打印图结构 - 用于调试
	 */
	void printGraph() {
		for (int l = 0; l < layerEdgeLists.size(); l++) {
			cout << "Layer:" << l << endl;
			for (auto it = layerEdgeLists[l].begin(); it != layerEdgeLists[l].end(); ++it) {
				cout << it->first << ":";
				for (auto ed: it->second) cout << ed << " ";
				cout << endl;
			}
		}
	}
};

#endif