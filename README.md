# MultiMerge

多文件数据合并工具 - 支持时间序列数据的智能对齐和插值

## 功能特性

- **时间格式支持**: 支持点号和冒号两种格式（HH:MM:SS.fff / HH:MM:SS:fff）
- **精度自动识别**: 毫秒和微秒精度自动推断
- **插值算法**: 最近邻插值和线性插值
- **双引擎架构**: 
  - Legacy 引擎（DataFileMerger）- 稳定可靠
  - Stream 引擎（StreamMergeEngine）- 高性能流式处理
- **文件分类**: 自动检测文件类型（顺序均匀/顺序校正/索引回退）
- **Tolerance 控制**: 精确匹配和容差匹配可配置

## 快速开始

### 编译

```bash
cd build
cmake ..
cmake --build .
```

### 使用

```bash
# 基本用法
MultiMerge.exe -o output.txt input1.txt input2.txt

# 指定插值方法
MultiMerge.exe -i linear -o output.txt input1.txt input2.txt

# 设置时间容差（微秒）
MultiMerge.exe -t 100000 -o output.txt input1.txt input2.txt

# 选择引擎
MultiMerge.exe -E stream -o output.txt input1.txt input2.txt
```

### 命令行选项

| 选项 | 说明 |
|------|------|
| `-o, --output` | 输出文件路径 |
| `-i, --interpolation` | 插值方法（nearest/linear） |
| `-d, --delimiter` | 输出分隔符（space/comma/tab） |
| `-e, --encoding` | 输出编码（UTF-8/GBK） |
| `-t, --tolerance` | 时间容差（微秒，-1=无限制，0=精确匹配） |
| `-E, --engine` | 合并引擎（legacy/stream） |
| `-v, --verbose` | 详细输出模式 |
| `-h, --help` | 显示帮助 |
| `--version` | 显示版本号 |

## 项目结构

```
MultiMerge/
├── cli/                    # 命令行接口
│   └── main.cpp
├── core/                   # 核心合并引擎
│   ├── DataFileMerger.cpp  # 旧引擎
│   ├── DataFileMerger.h
│   ├── StreamMergeEngine.cpp  # 新引擎
│   ├── StreamMergeEngine.h
│   └── Interpolator.cpp    # 插值算法
├── io/                     # 文件读取
│   ├── FileReader.cpp
│   └── FileReader.h
├── utils/                  # 工具类
│   └── DelimiterDetector.cpp
├── TimeParser.cpp          # 时间解析器
├── TimePoint.cpp           # 时间点对象
├── tests/                  # 测试用例
│   ├── input/              # 输入测试数据
│   ├── expected/           # 预期输出
│   ├── output/             # 实际输出
│   ├── scripts/            # 测试脚本
│   └── *.md                # 测试报告
└── build/                  # 构建目录
```

## 测试用例

测试用例位于 `tests/` 目录，包含：

### 输入数据 (tests/input/)
- 时间格式测试：点号毫秒、点号微秒、冒号毫秒、冒号微秒
- 文件类型测试：A 类（顺序均匀）、B 类（顺序校正）、C 类（乱序）
- Tolerance 测试：精确匹配、容差匹配、无限制
- 混合模式测试：A+B+C 类文件混合

### 预期输出 (tests/expected/)
- 各种场景的标准输出
- 用于验证合并结果正确性

### 测试脚本 (tests/scripts/)
- `run_format_tests.ps1` - 时间格式测试
- `run_basic_correctness_tests.ps1` - 基本正确性测试
- `run_day4_chaos_tests.ps1` - 复杂场景测试

### 测试报告
- `tolerance_fix_final_report.md` - Tolerance 功能修复报告
- `day5_final_report.md` - Day5 测试报告
- `correctness_report.md` - 正确性测试报告

## 运行测试

```powershell
# 运行格式测试
cd tests
.\scripts\run_format_tests.ps1

# 运行 Tolerance 测试
.\run_tolerance_tests.ps1
```

## 技术规格

### Tolerance 语义

| toleranceUs 值 | 语义 | 说明 |
|----------------|------|------|
| `-1` | unlimited | 无限制，总是返回最近点 |
| `0` | exact match | 精确匹配，超出范围返回 NaN |
| `>0` | tolerance | 容差范围内返回最近点，超出返回 NaN |

### 时间格式

- **点号格式**: `HH:MM:SS.fff` (毫秒) / `HH:MM:SS.ffffff` (微秒)
- **冒号格式**: `HH:MM:SS:fff` (毫秒) / `HH:MM:SS:ffffff` (微秒)
- **内部精度**: 纳秒计算

### 文件分类

- **A 类 (SequentialUniform)**: 时间严格单增且间隔均匀
- **B 类 (SequentialCorrected)**: 时间严格单增但间隔不均匀
- **C 类 (IndexedFallback)**: 时间乱序，需要全加载排序

## 开发状态

- 🔄 M1.5: 核心功能已具备，产品契约与工程基线尚未冻结
- ⏭️ M2: GUI Alpha 版本（待开发）
- ⏭️ M3: 功能增强（待开发）
- ⏭️ M4: 性能优化（待开发）
- ⏭️ M5: v1.0 发布（待开发）

## 许可证

MIT License

## 贡献

欢迎提交 Issue 和 Pull Request！
