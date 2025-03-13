#include "hnsw.h"

#include <algorithm>
#include <iostream>
#include <queue>
#include <random>
#include <set>
#include <unordered_set>
#include <vector>
using namespace std;

/**
 * 在指定层搜索最近邻
 * 实现了HNSW论文中的搜索算法
 */
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

/**
 * K近邻搜索 - 查找K个最近邻
 * 实现了HNSW的多层搜索策略
 */
vector<int> HNSWGraph::KNNSearch(Item& q, int K) {
	// 获取最高层
	int maxLyer = layerEdgeLists.size() - 1;
	// 从入口节点开始
	int ep = enterNode;
	
	// 从最高层开始，逐层向下搜索，每层只找最近的1个点作为下一层的入口点
	for (int l = maxLyer; l >= 1; l--) ep = searchLayer(q, ep, 1, l)[0];
	
	// 在底层（0层）搜索K个最近邻并返回
	return searchLayer(q, ep, K, 0);
}

/**
 * 添加边 - 在指定层连接两个节点
 * 实现了无向图的边添加
 */
void HNSWGraph::addEdge(int st, int ed, int lc) {
	// 避免自环
	if (st == ed) return;
	
	// 添加双向边
	layerEdgeLists[lc][st].push_back(ed);
	layerEdgeLists[lc][ed].push_back(st);
}

/**
 * 插入新数据项到图中
 * 实现了HNSW的分层插入策略
 */
void HNSWGraph::Insert(Item& q) {
	// 获取新节点ID
	int nid = items.size();
	itemNum++; items.push_back(q);
	
	// 随机采样节点所在的层级
	int maxLyer = layerEdgeLists.size() - 1;
	int l = 0;
	uniform_real_distribution<double> distribution(0.0,1.0);
	while(l < ml && (1.0 / ml <= distribution(generator))) {
		l++;
		// 如果需要，创建新层
		if (layerEdgeLists.size() <= l) layerEdgeLists.push_back(unordered_map<int, vector<int>>());
	}
	
	// 如果是第一个节点，直接设为入口节点并返回
	if (nid == 0) {
		enterNode = nid;
		return;
	}
	
	// 搜索上层入口点
	int ep = enterNode;
	for (int i = maxLyer; i > l; i--) ep = searchLayer(q, ep, 1, i)[0];
	
	// 从节点所在层开始，逐层向下插入
	for (int i = min(l, maxLyer); i >= 0; i--) {
		// 确定当前层的最大邻居数
		int MM = l == 0 ? MMax0 : MMax;
		
		// 在当前层搜索efConstruction个最近邻
		vector<int> neighbors = searchLayer(q, ep, efConstruction, i);
		
		// 选择最近的M个点作为邻居
		vector<int> selectedNeighbors = vector<int>(neighbors.begin(), neighbors.begin()+min(int(neighbors.size()), M));
		
		// 为新节点添加边连接到选定的邻居
		for (int n: selectedNeighbors) addEdge(n, nid, i);
		
		// 检查并调整邻居的连接，确保每个节点的邻居数不超过限制
		for (int n: selectedNeighbors) {
			if (layerEdgeLists[i][n].size() > MM) {
				// 计算邻居节点与其所有邻居的距离
				vector<pair<double, int>> distPairs;
				for (int nn: layerEdgeLists[i][n]) distPairs.emplace_back(items[n].dist(items[nn]), nn);
				
				// 按距离排序
				sort(distPairs.begin(), distPairs.end());
				
				// 只保留最近的MM个邻居
				layerEdgeLists[i][n].clear();
				for (int d = 0; d < min(int(distPairs.size()), MM); d++) layerEdgeLists[i][n].push_back(distPairs[d].second);
			}
		}
		
		// 更新入口点为当前层的第一个选定邻居
		ep = selectedNeighbors[0];
	}
	
	// 如果新节点在最高层，则更新入口节点
	if (l == layerEdgeLists.size() - 1) enterNode = nid;
}
