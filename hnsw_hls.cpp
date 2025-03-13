#include "hnsw_hls.h"

/**
 * 添加边 - 在指定层连接两个节点
 */
void HNSWGraph::addEdge(int st, int ed, int lc) {
    // 避免自环
    if (st == ed) return;
    
    // 添加双向边
    layerEdgeLists[lc][st].push_back(ed);
    layerEdgeLists[lc][ed].push_back(st);
}

/**
 * 在指定层搜索最近邻
 */
void HNSWGraph::searchLayer(const Item& q, int ep, int ef, int lc, int* results, int& resultSize) {
    // 候选集合，按距离排序
    CandidateSet candidates;
    candidates.init();
    
    // 当前找到的最近邻集合
    CandidateSet nearestNeighbors;
    nearestNeighbors.init();
    
    // 已访问节点集合
    VisitedSet isVisited;
    isVisited.init();
    
    // 计算查询点与入口点的距离
    double td = q.dist(items[ep], dim);
    candidates.insert(td, ep);
    nearestNeighbors.insert(td, ep);
    isVisited.insert(ep);
    
    // 主搜索循环
    while (!candidates.empty()) {
        // 取出距离最近的候选点
        DistIdPair ci = candidates.front();
        candidates.pop_front();
        int nid = ci.id;
        
        // 获取当前最近邻集合中距离最远的点
        if (nearestNeighbors.size == 0) break;
        DistIdPair fi = nearestNeighbors.back();
        
        // 如果当前候选点比最远的最近邻还远，则停止搜索
        if (ci.dist > fi.dist) break;
        
        // 遍历当前节点的所有邻居
        for (int i = 0; i < layerEdgeLists[lc][nid].size; i++) {
            int ed = layerEdgeLists[lc][nid][i];
            
            // 跳过已访问节点
            if (isVisited.contains(ed)) continue;
            
            isVisited.insert(ed);
            
            // 计算查询点与邻居的距离
            td = q.dist(items[ed], dim);
            
            // 如果邻居比当前最远的最近邻更近，或者最近邻集合未满，则加入候选集和最近邻集合
            if (nearestNeighbors.size == 0 || td < nearestNeighbors.back().dist || nearestNeighbors.size < ef) {
                candidates.insert(td, ed);
                nearestNeighbors.insert(td, ed);
                
                // 如果最近邻集合超过ef，则移除最远的点
                if (nearestNeighbors.size > ef) {
                    nearestNeighbors.pop_back();
                }
            }
        }
    }
    
    // 将最近邻集合转换为ID列表返回
    resultSize = nearestNeighbors.size < ef ? nearestNeighbors.size : ef;
    for (int i = 0; i < resultSize; i++) {
        results[i] = nearestNeighbors.data[i].id;
    }
}

/**
 * 插入新数据项到图中
 */
void HNSWGraph::Insert(const Item& q) {
    // 获取新节点ID
    int nid = itemNum;
    
    // 复制数据项
    for (int i = 0; i < dim; i++) {
        items[nid].values[i] = q.values[i];
    }
    
    itemNum++;
    
    // 随机采样节点所在的层级
    int l = 0;
    while (l < ml && (1.0 / ml <= randomFloat())) {
        l++;
        if (l >= layerNodeCount[l]) {
            layerNodeCount[l] = 0;
        }
    }
    
    // 如果是第一个节点，直接设为入口节点并返回
    if (nid == 0) {
        enterNode = nid;
        
        // 初始化所有层的邻居列表
        for (int i = 0; i <= l; i++) {
            layerEdgeLists[i][nid].init();
            layerNodeCount[i]++;
        }
        return;
    }
    
    // 搜索上层入口点
    int maxLayer = 0;
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (layerNodeCount[i] > 0) maxLayer = i;
    }
    
    int ep = enterNode;
    int tmpResults[MAX_CANDIDATES];
    int resultSize;
    
    for (int i = maxLayer; i > l; i--) {
        searchLayer(q, ep, 1, i, tmpResults, resultSize);
        if (resultSize > 0) ep = tmpResults[0];
    }
    
    // 初始化新节点的邻居列表
    for (int i = 0; i <= l; i++) {
        layerEdgeLists[i][nid].init();
        layerNodeCount[i]++;
    }
    
    // 从节点所在层开始，逐层向下插入
    for (int i = l; i >= 0; i--) {
        // 确定当前层的最大邻居数
        int MM = i == 0 ? MMax0 : MMax;
        
        // 在当前层搜索efConstruction个最近邻
        searchLayer(q, ep, efConstruction, i, tmpResults, resultSize);
        
        // 选择最近的M个点作为邻居
        int selectedSize = resultSize < M ? resultSize : M;
        
        // 为新节点添加边连接到选定的邻居
        for (int n = 0; n < selectedSize; n++) {
            addEdge(tmpResults[n], nid, i);
        }
        
        // 检查并调整邻居的连接，确保每个节点的邻居数不超过限制
        for (int n = 0; n < selectedSize; n++) {
            int neighbor = tmpResults[n];
            if (layerEdgeLists[i][neighbor].size > MM) {
                // 计算邻居节点与其所有邻居的距离
                DistIdPair distPairs[MAX_NEIGHBORS];
                int distPairsSize = 0;
                
                for (int j = 0; j < layerEdgeLists[i][neighbor].size; j++) {
                    int nn = layerEdgeLists[i][neighbor][j];
                    double distance = items[neighbor].dist(items[nn], dim);
                    
                    // 插入排序
                    int pos = distPairsSize;
                    while (pos > 0 && distPairs[pos-1].dist > distance) {
                        distPairs[pos] = distPairs[pos-1];
                        pos--;
                    }
                    
                    distPairs[pos].dist = distance;
                    distPairs[pos].id = nn;
                    
                    if (distPairsSize < MAX_NEIGHBORS) {
                        distPairsSize++;
                    }
                }
                
                // 只保留最近的MM个邻居
                layerEdgeLists[i][neighbor].clear();
                for (int d = 0; d < (distPairsSize < MM ? distPairsSize : MM); d++) {
                    layerEdgeLists[i][neighbor].push_back(distPairs[d].id);
                }
            }
        }
        
        // 更新入口点为当前层的第一个选定邻居
        if (resultSize > 0) {
            ep = tmpResults[0];
        }
    }
    
    // 如果新节点在最高层，则更新入口节点
    if (l >= maxLayer) {
        enterNode = nid;
    }
}

/**
 * K近邻搜索 - 查找K个最近邻
 */
void HNSWGraph::KNNSearch(const Item& q, int K, int* results) {
    // 获取最高层
    int maxLayer = 0;
    for (int i = 0; i < MAX_LAYERS; i++) {
        if (layerNodeCount[i] > 0) maxLayer = i;
    }
    
    // 从入口节点开始
    int ep = enterNode;
    int tmpResults[MAX_CANDIDATES];
    int resultSize;
    
    // 从最高层开始，逐层向下搜索，每层只找最近的1个点作为下一层的入口点
    for (int l = maxLayer; l >= 1; l--) {
        searchLayer(q, ep, 1, l, tmpResults, resultSize);
        if (resultSize > 0) ep = tmpResults[0];
    }
    
    // 在底层（0层）搜索K个最近邻并返回
    searchLayer(q, ep, K, 0, results, resultSize);
} 