CXX = g++
CXXFLAGS = -std=c++11 -Wall -O3
LDFLAGS = -lm

# 目标文件
TARGET = hnsw_main

# 源文件
SRCS = main.cpp hnsw.cpp

# 头文件
HDRS = hnsw.h

# 目标文件
OBJS = $(SRCS:.cpp=.o)

# 默认目标
all: $(TARGET)

# 链接规则
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

# 编译规则
%.o: %.cpp $(HDRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# 清理规则
clean:
	rm -f $(OBJS) $(TARGET)

# 运行规则
run: $(TARGET)
	./$(TARGET)

.PHONY: all clean run 