# MultiMerge

MultiMerge 是一个多文件数据合并工具，用于合并多个时间序列数据文件。它支持不同的插值方法和合并引擎，能够处理不同格式的数据文件。

## 功能特性

- 支持多个时间序列数据文件的合并
- 支持不同的插值方法（最近邻、线性）
- 支持两种合并引擎：
  - 传统合并引擎（DataFileMerger）
  - 流式合并引擎（StreamMergeEngine）
- 自动检测文件分隔符
- 支持自定义输出文件路径
- 支持命令行参数配置

## 项目结构

```
MultiMerge/
├── CMakeLists.txt       # CMake 构建配置
├── MultiMerge.pro       # Qt 项目配置
├── README.md            # 项目说明
├── src/
│   ├── core/
│   │   ├── time/        # 时间处理相关
│   │   │   ├── TimePoint.h
│   │   │   ├── TimePoint.cpp
│   │   │   ├── TimeParser.h
│   │   │   └── TimeParser.cpp
│   │   ├── interpolate/ # 插值算法
│   │   │   ├── Interpolator.h
│   │   │   └── Interpolator.cpp
│   │   └── merge/       # 合并引擎
│   │       ├── DataFileMerger.h
│   │       ├── DataFileMerger.cpp
│   │       ├── StreamMergeEngine.h
│   │       └── StreamMergeEngine.cpp
│   ├── io/              # 文件读写
│   │   ├── FileReader.h
│   │   └── FileReader.cpp
│   ├── utils/           # 工具类
│   │   ├── DelimiterDetector.h
│   │   └── DelimiterDetector.cpp
│   └── main/cli/        # 命令行入口
│       └── main.cpp
└── tests/               # 测试文件
```

## 构建方法

### 使用 CMake

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

### 使用 Qt Creator

1. 打开 `MultiMerge.pro` 文件
2. 构建项目

## 使用方法

### 命令行参数

```
MultiMerge [options] file1 file2 ...

选项：
  -h, --help                 显示帮助信息
  -v, --version              显示版本信息
  -o, --output <output>      输出文件路径
  -i, --interpolation <interpolation>  插值类型 (nearest/linear)
  -s, --stream               使用流式合并引擎
```

### 示例

1. 使用默认参数合并文件：
   ```bash
   MultiMerge file1.txt file2.txt
   ```

2. 指定输出文件和插值方法：
   ```bash
   MultiMerge -o merged.txt -i linear file1.txt file2.txt file3.txt
   ```

3. 使用流式合并引擎：
   ```bash
   MultiMerge -s -o merged.txt file1.txt file2.txt
   ```

## 输入文件格式

输入文件应该是带有表头的文本文件，第一列是时间戳，格式为 `HH:MM:SS:MS`（小时:分钟:秒:毫秒）。后续列是对应时间点的数据值。

示例：

```
Time,Value1,Value2
00:00:00:000,1.0,2.0
00:00:01:000,1.5,2.5
00:00:02:000,2.0,3.0
```

## 输出文件格式

输出文件与输入文件格式类似，第一列是合并后的时间戳，后续列是各个输入文件的数据值。如果某个时间点在某个输入文件中不存在，会使用指定的插值方法进行填充。

## 插值方法

- **nearest**：最近邻插值，使用最近的时间点的值
- **linear**：线性插值，使用相邻时间点的线性插值

## 合并引擎

- **DataFileMerger**：传统合并引擎，先读取所有文件到内存，然后进行合并
- **StreamMergeEngine**：流式合并引擎，逐行处理文件，内存占用更低

## 依赖

- Qt 5.15 或更高版本
- C++17 兼容的编译器

## 许可证

MIT 许可证