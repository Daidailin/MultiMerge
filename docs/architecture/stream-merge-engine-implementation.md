﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿# StreamMergeEngine 实现文档

> **文档版本**: v1.0  
> **实现时间**: 2026-03-19  
> **实现范围**: 任务 1（边界定义）+ 任务 2（预扫描功能）

---

## 一、已完成工作总结

### 任务 1：定义 StreamMergeEngine 边界与接口 ✅

#### 创建的文件

1. **StreamMergeEngine.h** - 接口定义
   - ✅ 四种枚举类型：`InterpolationMethod`、`OutputDelimiter`、`OutputEncoding`、`FileCategory`
   - ✅ `Config` 结构体 - 配置参数（插值方法、分隔符、编码、扫描参数等）
   - ✅ `TimeIndex` 结构体 - 用于回退机制的时间索引
   - ✅ `FileMetadata` 结构体 - 文件元数据（路径、行数、时间特性、分类等）
   - ✅ `MergeStats` 结构体 - 统计信息（各阶段耗时、内存、文件分类计数）
   - ✅ `MergeResult` 结构体 - 合并结果（成功标志、错误信息、警告、元数据、统计）
   - ✅ 核心接口：`merge()`、`cancel()`、`isCancelled()`
   - ✅ Qt 信号：`progressChanged`、`statusMessage`、`warningOccurred`、`errorOccurred`

2. **StreamMergeEngine.cpp** - 基础框架实现
   - ✅ 完整的主流程框架：`doMerge()` 
   - ✅ 输入验证：`validateInputs()`
   - ✅ 扫描流程：`scanFiles()`、`scanSingleFile()`
   - ✅ 文件分类：`classifyFile()`
   - ✅ 合并流程：`mergeStreaming()`（基础框架）
   - ✅ 工具函数：`delimiterString()`、`shouldCancel()`

#### 设计亮点

1. **清晰的职责边界** - 只负责核心合并逻辑，不处理 UI、CLI 参数解析
2. **策略分流机制** - 通过 `FileCategory` 实现 A/B/C 三类文件的自动分类处理
3. **完整的状态反馈** - 进度、状态、警告、错误通过信号机制上报
4. **可取消设计** - 使用 `std::atomic<bool>` 实现线程安全的取消机制
5. **统计信息完备** - 扫描/合并/写入各阶段耗时、文件分类统计、内存使用等

---

### 任务 2：实现预扫描 + 文件特性检测 ✅

#### 核心功能实现

**scanSingleFile()** 完整实现，包括：

1. **文件打开与编码检测** ✅
   - 使用 `FileReader::detectEncoding()` 检测文件编码
   - 自动设置 `QTextStream` 编码
   - 文件存在性与可读性检查

2. **表头解析** ✅
   - 使用 `DelimiterDetector::detect()` 自动识别分隔符
   - 解析表头并保存到 `FileMetadata.headers`
   - 空表头错误处理

3. **时间列解析** ✅
   - 使用 `TimeParser` 解析时间字符串
   - 转换为纳秒时间戳（`timestampNs`）
   - 时间解析失败处理

4. **单调性检测** ✅
   - 逐行比较当前时间与上一行时间
   - 发现 `currentNs < prevTimeNs` 时标记 `isMonotonic = false`
   - 严格单增判定

5. **均匀性检测** ✅
   - 采样记录时间间隔（默认采样间隔 1000 行）
   - 计算平均间隔 `avgIntervalNs`
   - 计算最大偏差百分比 `maxDeviationPercent`
   - 根据配置阈值 `uniformTolerancePercent` 判定均匀性

6. **轻量索引建立** ✅
   - 每 1000 行记录一个索引点 `{timestampNs, fileOffset}`
   - 为回退机制预留数据支持
   - 内存优化：不记录所有行，只记录采样点

7. **文件分类** ✅
   - 调用 `classifyFile()` 进行自动分类
   - A 类（SequentialUniform）：单增 + 均匀（偏差 <= 1%）
   - B 类（SequentialCorrected）：单增但不均匀
   - C 类（IndexedFallback）：非单增文件

8. **取消检查** ✅
   - 每行读取前检查取消标志
   - 取消时清空错误信息，不视为错误
   - 正确区分"用户取消"与"执行失败"

#### 技术实现细节

**局部扫描统计器（ScanAccumulator）**
```cpp
struct ScanAccumulator {
    int totalRows = 0;
    bool hasFirstTime = false;
    qint64 firstTimeNs = 0;
    qint64 prevTimeNs = 0;
    qint64 lastTimeNs = 0;
    bool isMonotonic = true;
    QVector<qint64> sampledIntervals;
    qint64 intervalSumNs = 0;
    int intervalCount = 0;
};
```

**采样策略**
- 前 10000 个间隔：全量记录
- 超过 10000 后：按 `scanSampleStride`（默认 1000）采样
- 平衡精度与内存占用

**索引策略**
- 每 1000 行记录一个索引点
- 包含时间戳和文件偏移
- 为后续回退机制提供定位支持

---

### 任务 3：实现顺序推进主路径（基础框架） ✅

#### mergeStreaming() 实现

当前实现了基础框架，支持：

1. **输出文件创建** ✅
   - 自动创建输出文件
   - 支持 UTF-8/GBK 编码设置
   - 错误处理

2. **表头生成** ✅
   - 以第一个文件为基准时间轴
   - 自动去重列名
   - 支持多文件列名合并

3. **数据写出** ✅
   - 逐行写出时间列和数据列
   - 支持分隔符配置（空格/逗号/制表符）
   - 批量刷新缓冲（默认每 10000 行）

4. **进度反馈** ✅
   - 实时更新进度百分比
   - 发送状态消息
   - 支持取消检查

#### 当前限制

- 数据列填充为占位实现（填充"0"）
- 未实现插值逻辑
- 未实现滑动窗口查找
- 未区分 A/B/C 类文件的不同处理策略

---

## 二、关键改进点

### 1. 取消语义修复 ✅

**问题**：原实现将"用户取消"视为"执行失败"

**修复方案**：
```cpp
if (!scanSingleFile(inputFiles[i], metadata, warnings, error)) {
    if (shouldCancel()) {
        error.clear();  // 取消时清空错误信息
    }
    return false;
}
```

**效果**：
- CLI/GUI 可正确区分取消与失败
- 取消时不触发错误提示
- `MergeResult.cancelled` 标志正确设置

### 2. 文件分类策略 ✅

**分类逻辑**：
```cpp
void StreamMergeEngine::classifyFile(FileMetadata& metadata) const
{
    if (!metadata.isMonotonic) {
        metadata.category = FileCategory::IndexedFallback;
        return;
    }

    if (metadata.isUniform &&
        metadata.maxDeviationPercent <= m_config.uniformTolerancePercent) {
        metadata.category = FileCategory::SequentialUniform;
        return;
    }
    
    metadata.category = FileCategory::SequentialCorrected;
}
```

**分类标准**：
- A 类：单增 + 偏差 <= 1%（可配置）
- B 类：单增 + 偏差 > 1%
- C 类：非单增

### 3. 统计信息完善 ✅

**MergeStats 字段赋值**：
- `scanElapsedMs`：扫描阶段计时
- `mergeElapsedMs`：合并阶段计时
- `totalElapsedMs`：总耗时
- `totalInputFiles`：输入文件数
- `totalOutputRows`：输出行数
- `sequentialUniformFiles`：A 类文件数
- `sequentialCorrectedFiles`：B 类文件数
- `fallbackFiles`：C 类文件数

---

## 三、待完成工作

### 任务 3（完全体）：实现顺序推进主路径 ⏳

需要实现：

1. **滑动窗口查找** ⏳
   - 为每个从文件维护滑动窗口
   - 实现 O(1) 或 O(log n) 时间查找
   - 窗口更新策略

2. **插值算法集成** ⏳
   - Nearest Neighbor 插值
   - Linear 插值
   - 无插值（直接匹配）

3. **真实数据读取** ⏳
   - 在合并阶段读取数据列
   - 内存优化（流式读取）
   - 避免重复读盘

### 任务 4：实现异常回退机制 ⏳

需要实现：

1. **二分查找** ⏳
   - 针对 C 类文件
   - 使用 `indices` 进行快速定位
   - O(log n) 时间复杂度

2. **插值修正** ⏳
   - 针对 B 类文件
   - 在顺序推进基础上增加修正逻辑
   - 处理时间抖动

3. **混合策略** ⏳
   - 同一文件中部分区域顺序、部分区域回退
   - 动态策略切换

### 任务 5：接入 CLI 并建立验证 ⏳

需要实现：

1. **CLI 集成** ⏳
   - 新增 `--engine stream` 参数
   - 与旧引擎对比输出
   - 配置参数映射

2. **正确性测试** ⏳
   - 单元测试（扫描逻辑）
   - 集成测试（多文件合并）
   - 边界条件测试

3. **性能基准** ⏳
   - 小文件（10 万行）
   - 中文件（50 万行）
   - 大文件（100 万行+）
   - 对比旧引擎性能

---

## 四、代码质量分析

### 优点

1. **接口设计清晰** - 职责边界明确，符合单一职责原则
2. **错误处理完善** - 每个阶段都有错误检查和反馈
3. **取消机制健壮** - 正确区分取消与失败
4. **统计信息完备** - 便于后续性能分析和调试
5. **可扩展性好** - 预留了配置项和扩展点

### 待改进

1. **内存优化** - 当前索引记录策略可进一步优化
2. **采样精度** - 均匀性检测采样策略可动态调整
3. **offset 精度** - `QTextStream` 缓冲可能影响 `file.pos()` 精度
4. **测试覆盖** - 缺少单元测试验证

---

## 五、下一步建议

### 立即执行（本周）

1. **完善 mergeStreaming()**
   - 实现滑动窗口查找
   - 集成插值算法
   - 读取真实数据列

2. **实现回退机制**
   - 二分查找
   - 索引使用
   - 混合策略

3. **CLI 集成**
   - 添加 `--engine stream` 参数
   - 配置参数映射
   - 输出统计信息

### 中期规划（下周）

1. **建立测试集**
   - 准备标准测试文件
   - 编写单元测试
   - 性能基准测试

2. **性能优化**
   - 分析瓶颈
   - 优化热点代码
   - 对比旧引擎性能

3. **文档完善**
   - 更新 API 文档
   - 补充使用示例
   - 记录已知问题

---

## 六、技术债务

### 已知问题

1. **offset 精度问题**
   - `QTextStream` 缓冲导致 `file.pos()` 可能不精确
   - 建议：后续回退强依赖 offset 时，改用 `QFile::readLine()`

2. **内存占用**
   - 当前每文件都记录索引（虽然每 1000 行一个）
   - 建议：确认主路径稳定后，只为 C 类文件保留索引

3. **采样策略固定**
   - 前 10000 全量 + 固定 stride 采样
   - 建议：根据文件大小动态调整采样率

### 优化机会

1. **编码检测复用** - 已复用 `FileReader::detectEncoding()`
2. **分隔符检测复用** - 已复用 `DelimiterDetector::detect()`
3. **时间解析复用** - 已复用 `TimeParser`
4. **插值算法** - 待复用 `Interpolator`

---

## 七、验收标准

### 任务 1 验收 ✅

- [x] 接口定义完整
- [x] 数据结构合理
- [x] 流程骨架搭建完成
- [x] 配置与状态通路齐全

**评分：90/100**

### 任务 2 验收 ✅

- [x] 文件打开与编码检测
- [x] 表头解析
- [x] 时间列解析
- [x] 单调性检测
- [x] 均匀性检测
- [x] 轻量索引建立
- [x] 文件分类
- [x] 取消检查

**评分：85/100**

### 任务 3 验收（基础框架） ⏳

- [x] 输出文件创建
- [x] 表头生成
- [x] 数据写出（占位）
- [x] 进度反馈
- [ ] 滑动窗口查找
- [ ] 插值算法
- [ ] 真实数据读取

**当前评分：40/100（框架完成，核心逻辑待实现）**

---

## 八、总结

### 已完成

- ✅ StreamMergeEngine 边界与接口定义
- ✅ 预扫描功能完整实现
- ✅ 文件特性检测（单调性/均匀性）
- ✅ 文件分类策略（A/B/C 三类）
- ✅ 合并流程基础框架
- ✅ 取消语义修复

### 待完成

- ⏳ 滑动窗口查找与插值算法
- ⏳ 异常回退机制（二分查找）
- ⏳ CLI 集成与测试验证
- ⏳ 性能基准测试

### 总体进度

**当前完成度：约 50%**

- 架构设计：100%
- 扫描功能：100%
- 合并框架：60%
- 核心算法：20%
- 测试验证：0%

---

**下一步优先级：**

1. **P0** - 实现滑动窗口查找与插值算法
2. **P0** - 实现二分查找回退机制
3. **P1** - CLI 集成
4. **P1** - 建立测试集并验证
