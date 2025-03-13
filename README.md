# Hierarchical Navigable Small World for AKNN 
Naive implementation of HNSW for AKNN based on [paper](https://arxiv.org/pdf/1603.09320.pdf)

* Naive implementation without any optimization and used a lot of STL.
* Only implemented the simple SELECT-NEUGHBORS-SIMPLE method for neighborhood selection.

## Simple Random Vector Test Result
* Dimension = 4
* M: number of added neighbors = 10
* MMax: Max number of neighbors in layers >= 1 = 30
* MMax0: Max number of neighbors in layers 0 = 30
* efConstruction: Search numbers in construction 10
* ml: Max number of layers = 2
* Test vector sample each component i.i.d from Unif[0, 1]
* Query sample each component i.i.d from Unif[0, 1] with 100 queries
* Run AKNN with K = 5 to compute top-1 recall

| Num Items   | Top-1 Recall | Brute Force (ms/q) | HNSW (ms/q) | 
| ----------- | ----------- | ----------- | ----------- |
| 10k         | 100%        | 2.72        | 0.15        |
| 100k        | 100%        | 35.32       | 0.26        |
| 1M          | 100%        | 483.74      | 0.42        |

## 构建和运行

### 使用 Makefile 构建

项目现在支持使用 Makefile 进行构建，您可以使用以下命令：

```bash
# 编译项目
make

# 运行项目
make run

# 清理编译产物
make clean
```

### 使用 Bazel 构建

项目也支持使用 Bazel 进行构建：

```bash
# 编译项目
bazel build //:hnsw_main

# 运行项目
bazel run //:hnsw_main
```
