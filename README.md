# MultiMerge

数据文件合并工具 - 基于时间戳对齐合并多个数据文件

## 功能特性

- ✅ 支持合并多个具有时间戳的数据文件
- ✅ 自动检测文件分隔符（逗号、分号、制表符、空格）
- ✅ 支持多种时间格式（HH:MM:SS.fff）
- ✅ 三种插值方法：最近邻、线性插值、不插值
- ✅ 灵活的输出格式（空格、逗号、制表符分隔）
- ✅ 支持 UTF-8 和 GBK 编码
- ✅ 可配置的时间容差
- ✅ 详细输出模式

## 编译要求

- Qt 5.9 或更高版本
- C++17 兼容编译器
- MSVC 2015 或更高版本

## 编译说明

1. 使用 Qt Creator 打开 `MultiMerge.pro`
2. 选择 Desktop Qt 5.9 MSVC2015 64bit 套件
3. 按 Ctrl+B 编译

## 使用方法

### 命令行

```bash
MultiMerge.exe [选项] <输入文件 1> <输入文件 2> ...
```

### 选项

| 选项 | 说明 | 默认值 |
|------|------|--------|
| `-o, --output <文件名>` | 输出文件名 | `merged_result.txt` |
| `-i, --interpolation <方法>` | 插值方法：nearest, linear, none | `nearest` |
| `-d, --delimiter <分隔符>` | 输出分隔符：space, comma, tab | `space` |
| `-e, --encoding <编码>` | 输出编码：UTF-8, GBK | `UTF-8` |
| `-t, --tolerance <毫秒>` | 时间容差（毫秒） | `0` |
| `-v, --verbose` | 详细输出模式 | 关闭 |
| `-h, --help` | 显示帮助信息 | - |
| `--version` | 显示版本信息 | - |

### 示例

```bash
# 合并两个传感器文件
MultiMerge.exe sensor1.txt sensor2.txt -o result.txt -v

# 使用线性插值，逗号分隔
MultiMerge.exe data1.txt data2.txt data3.txt -i linear -d comma

# 使用 GBK 编码输出
MultiMerge.exe file1.txt file2.txt -e GBK
```

## 输入文件格式

支持的时间格式：
- `HH:MM:SS.fff` (时：分：秒。毫秒)
- 自动检测分隔符（逗号、分号、制表符、空格）

示例：
```
Time;Sensor1;Sensor2;Sensor3
00:00:00.000;1.5;2.3;3.1
00:00:01.000;1.6;2.4;3.2
00:00:02.000;1.7;2.5;3.3
```

## 输出格式

输出文件包含：
- 时间列（来自第一个文件）
- 第一个文件的所有数据列
- 其他文件的数据列（通过插值对齐到第一个文件的时间点）

**注意**：如果列名重复，会发出警告并跳过重复的列，保留第一个文件的数据。

## 项目结构

```
MultiMerge/
├── cli/                  # 命令行接口
│   └── main.cpp
├── core/                 # 核心业务逻辑
│   ├── Interpolator.h/cpp   # 插值算法
│   └── DataFileMerger.h/cpp # 合并引擎
├── io/                   # 文件 I/O
│   └── FileReader.h/cpp     # 文件读取器
├── utils/                # 工具类
│   └── DelimiterDetector.h/cpp # 分隔符检测
├── TimePoint.h/cpp       # 时间点表示
├── TimeParser.h/cpp      # 时间字符串解析
└── MultiMerge.pro        # Qt 项目文件
```

## 许可证

MIT License

## 作者

Developed with ❤️
