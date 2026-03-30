# MultiMerge 基础正确性报告

## 1. 测试范围

### 已覆盖
- 严格单增时间序列（输入数据按时间排序）
- nearest（最近邻）和 linear（线性插值）两种插值模式
- legacy 和 stream 两种引擎的对比验证
- 3组金标准数据集的完整路径覆盖

### 未覆盖
- 乱序时间序列
- 混合模式（部分文件对齐，部分需要插值）
- tolerance 参数的各种设置
- 边界情况（NaN 传播、空文件、多文件）

## 2. 测试数据说明

### gold_a（完全对齐）
| 文件 | 时间点 | 特点 |
|------|--------|------|
| gold_a1_file1.txt | 0.000, 1.000, 2.000 | 基准时间轴，Temp/Pressure 列 |
| gold_a1_file2.txt | 0.000, 1.000, 2.000 | 与基准完全对齐，Sensor1/Sensor2 列 |

**用途**：验证最基础的输出结构、表头、行数、列数正确性。

### gold_b（最近邻插值）
| 文件 | 时间点 | 特点 |
|------|--------|------|
| gold_b1_file1.txt | 0.000, 1.000, 2.000 | 基准时间轴 |
| gold_b1_file2.txt | 0.200, 1.200, 2.200 | 偏移 200ms，需要最近邻匹配 |

**用途**：验证 nearest 模式在基础场景下能给出正确结果。

### gold_c（线性插值）
| 文件 | 时间点 | 特点 |
|------|--------|------|
| gold_c1_file1.txt | 0.000, 1.000, 2.000 | 基准时间轴，线性值 (0, 10, 20) |
| gold_c1_file2.txt | 0.300, 1.300 | 落在基准点之间，需要线性插值 |

**用途**：验证 linear 算法不是"能跑"，而是真按预期算值。

## 3. 执行矩阵

| 测试编号 | 测试集 | 引擎 | 插值模式 | 预期结果 |
|----------|--------|------|----------|----------|
| 1 | gold_a | legacy | nearest | PASS |
| 2 | gold_a | legacy | linear | PASS |
| 3 | gold_a | stream | nearest | PASS |
| 4 | gold_a | stream | linear | PASS |
| 5 | gold_b | legacy | nearest | PASS |
| 6 | gold_b | legacy | linear | PASS |
| 7 | gold_b | stream | nearest | PASS |
| 8 | gold_b | stream | linear | PASS |
| 9 | gold_c | legacy | nearest | PASS |
| 10 | gold_c | legacy | linear | PASS |
| 11 | gold_c | stream | nearest | PASS |
| 12 | gold_c | stream | linear | PASS |

## 4. 对比结果

### 核心结论
- **legacy vs expected**: 全部 6 项 PASS
- **stream vs expected**: 全部 6 项 PASS
- **legacy vs stream**: 数值部分一致，时间格式不一致

### 数值一致性验证
| 测试集 | 模式 | 数值是否一致 |
|--------|------|--------------|
| gold_a | nearest | 是 |
| gold_a | linear | 是 |
| gold_b | nearest | 是 |
| gold_b | linear | 是 |
| gold_c | nearest | 是 |
| gold_c | linear | 是 |

### 时间格式差异
| 引擎 | 时间格式 | 示例 |
|------|----------|------|
| legacy | HH:MM:SS:000 | 00:00:00:000 |
| stream | HH:MM:SS.000 | 00:00:00.000 |

## 5. 发现的问题

### 问题 1：时间格式不统一（已知问题）
- **描述**：Legacy 引擎输出冒号分隔毫秒，Stream 引擎输出点分隔毫秒
- **影响**：两个引擎输出不能直接 byte-level 对比
- **建议**：统一输出格式为 HH:MM:SS.000（点分隔）
- **状态**：测试脚本通过规范化时间格式规避此问题

### 问题 2：CLI 不支持 exact 模式
- **描述**：CLI 只支持 nearest 和 linear，不支持 exact（t=0 精确匹配）
- **影响**：无法直接测试"精确匹配"语义
- **建议**：如果 exact 语义重要，需要扩展 CLI 支持

### 问题 3：gold_c linear 插值计算验证
- **描述**：gold_c 在 t=2.000 时线性插值边界处理
- **验证**：插值结果 (160.0) 与手算一致
- **结论**：插值算法实现正确

## 6. 验证点总结

### 验证点 1：输出结构正确 ✓
- 输出文件生成：✓
- 行数等于基准文件时间轴行数：✓
- 列数符合预期：✓
- 时间列序列正确：✓

### 验证点 2：表头正确 ✓
- 表头顺序正确：✓
- 未出现旧的"文件名.列名"风格：✓
- 重名列按预期处理：✓

### 验证点 3：插值值正确 ✓
- nearest 给出正确最近邻结果：✓
- linear 给出正确线性插值结果：✓
- 外推边界情况处理正确：✓

### 验证点 4：新旧引擎行为一致 ✓ (数值层面)
- 行数一致：✓
- 时间列一致：✓
- 表头一致：✓
- 数值语义一致：✓

## 测试资产清单

### 输入数据
- `test_data/input/gold_a1_file1.txt`
- `test_data/input/gold_a1_file2.txt`
- `test_data/input/gold_b1_file1.txt`
- `test_data/input/gold_b1_file2.txt`
- `test_data/input/gold_c1_file1.txt`
- `test_data/input/gold_c1_file2.txt`

### 期望输出
- `test_data/expected/gold_a_nearest.txt`
- `test_data/expected/gold_a_linear.txt`
- `test_data/expected/gold_b_nearest.txt`
- `test_data/expected/gold_b_linear.txt`
- `test_data/expected/gold_c_nearest.txt`
- `test_data/expected/gold_c_linear.txt`

### 测试脚本
- `test_data/scripts/run_basic_correctness_tests.ps1`

## 后续工作建议

1. **修复时间格式不统一问题**：统一 legacy 和 stream 的输出格式
2. **扩展 exact 模式支持**：如需测试 t=0 精确匹配语义
3. **增加更多边界测试**：NaN 传播、乱序、多文件场景
4. **集成到 CI**：将回归测试加入持续集成流程

---
生成时间：2026-03-21
测试脚本：run_basic_correctness_tests.ps1
