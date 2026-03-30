﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿﻿# StreamMergeEngine V2 实施文档

## 概述

本文档记录 StreamMergeEngine 从"扫描器第一版 + 输出骨架"到"真实流式合并引擎"的重大改进。参考自审查文档 `StreamMergeEngine_3.md`。

## 问题清单与修复

### 1. 索引采样 bug（CRITICAL）

**问题现象**：
```cpp
// 错误代码
if (outMetadata.indices.size() % 1000 == 0) {
    outMetadata.indices.push_back(TimeIndex{currentNs, offset});
}
```

**根本原因**：
- `indices.size()` 只有在 push 时才会增加
- 一开始 size = 0，满足条件，push 一次
- 之后 size = 1，`1 % 1000 != 0`
- 后面永远不会再 push
- **结果：绝大多数情况下只会保存第一条索引**

**修复方案**：
```cpp
// 正确代码
const qint64 indexStride = 1000;
qint64 dataRowIndex = 0;

if (dataRowIndex % indexStride == 0) {
    outMetadata.indices.push_back(TimeIndex{currentNs, rowStartOffset});
    lastIndexedTimeNs = currentNs;
    lastIndexedOffset = rowStartOffset;
}
```

**关键点**：
- 使用 `dataRowIndex`（原始数据行号）而非容器大小
- 记录最后一个索引的时间和偏移，用于尾部补全

---

### 2. interval 采样 bug（CRITICAL）

**问题现象**：
```cpp
// 错误代码
if (acc.sampledIntervals.size() < 10000 ||
    acc.sampledIntervals.size() % m_config.scanSampleStride == 0) {
    acc.sampledIntervals.push_back(intervalNs);
}
```

**根本原因**：
- `sampledIntervals.size()` 是"已采样数量"，不是"已扫描到第几个 interval"
- 前 10000 个 interval 会一直 push
- 超过 10000 后，是否继续 push 取决于"当前已采样数量"是否刚好整除 stride
- **结果：非常怪异的采样分布，uniform 判定失真**

**修复方案**：
```cpp
// 正确代码
const qint64 intervalStride = qMax<qint64>(1, m_config.scanSampleStride);
qint64 intervalIndex = 0;

if (intervalIndex < 10000 || (intervalIndex % intervalStride == 0)) {
    acc.sampledIntervals.push_back(intervalNs);
}

++intervalIndex;
```

**关键点**：
- 使用独立的 `intervalIndex` 计数器
- 前 10000 个 interval 全记录，之后按 stride 采样

---

### 3. monotonic 判定过于宽松（HIGH）

**问题现象**：
```cpp
// 错误代码
if (currentNs < acc.prevTimeNs) {
    acc.isMonotonic = false;
}
```

**根本原因**：
- `currentNs == prevTimeNs` 会被视为"仍然 monotonic"
- 重复时间戳不会触发 fallback
- 但插值模型中重复时间需要单独处理（分母为 0 风险）

**修复方案**：
```cpp
// 正确代码
if (currentNs <= acc.prevTimeNs) {
    acc.isMonotonic = false;
}
```

**关键点**：
- 采用**严格单增**语义
- 重复时间直接归类为 `IndexedFallback`

---

### 4. mergeStreaming() 未实现真实合并（CRITICAL）

**问题现象**：
- 取第一个文件的 metadata
- 拼表头
- 按 `totalRows` 写行
- 第一列写时间或 `row_n`
- 其他列全部填 `"0"`

**根本原因**：
- 没有读取真实行数据
- 没有从文件 2..N 读取值
- 没有插值
- 没有顺序推进窗口
- 没有基于真实基准时间轴进行对齐

**修复方案**：
实现完整的流式合并主路径：

1. **StreamCursor 结构**：为每个文件维护游标
   ```cpp
   struct StreamCursor {
       QString path;
       QString tag;
       QStringList headers;
       QVector<int> dataColumnIndices;
       QChar delimiter;
       QFile file;
       std::unique_ptr<QTextStream> in;
       StreamRow lower;  // 滑动窗口下界
       StreamRow upper;  // 滑动窗口上界
       bool eof;
   };
   ```

2. **基准文件顺序读取**：
   ```cpp
   StreamCursor& base = cursors[0];
   while (true) {
       if (!readNextParsedRow(base, baseRow, error)) {
           return false;
       }
       if (!baseRow.valid) {
           break;  // EOF
       }
       const qint64 targetNs = baseRow.timeNs;
       // ...
   }
   ```

3. **从文件滑动窗口推进**：
   ```cpp
   for (int i = 1; i < cursors.size(); ++i) {
       StreamCursor& cursor = cursors[i];
       if (!advanceCursorToTarget(cursor, targetNs, error)) {
           return false;
       }
       // ...
   }
   ```

4. **插值算法集成**：
   ```cpp
   static QString interpolateValue(const StreamCursor& cursor,
                                   int sourceColumnIndex,
                                   qint64 targetNs,
                                   InterpolationMethod method)
   {
       // None: 精确匹配
       // Nearest: 最近邻
       // Linear: 线性插值（数值列）/ 退化为 Nearest（非数值列）
   }
   ```

---

### 5. 基准时间轴设计错误（HIGH）

**问题现象**：
```cpp
// 错误代码
if (row < timeAxisFile.indices.size()) {
    const TimeIndex& idx = timeAxisFile.indices[row];
    rowData << tp.toString();
} else {
    rowData << QString("row_%1").arg(row);
}
```

**根本原因**：
- 把稀疏索引当成稠密时间轴来用
- 假设 `indices[row]` 能对应"第 row 行的时间"
- 但 `indices` 是稀疏索引（每 1000 行保存一个）
- **结果：输出时间列会严重错误**

**修复方案**：
- **基准文件主时间轴不要依赖稀疏索引**
- 在合并阶段**重新顺序读取基准文件**
- 稀疏索引只用于 fallback，不用于驱动主合并循环

---

### 6. 重复分类调用（MEDIUM）

**问题现象**：
- `scanSingleFile()` 末尾调用了一次 `classifyFile()`
- `scanFiles()` 里又调用了一次

**修复方案**：
- 移除 `scanFiles()` 中的重复调用
- `scanSingleFile()` 负责完整产出 metadata + category

---

### 7. progress 信号发送过频（MEDIUM）

**优化方案**：
```cpp
// 限制为每 1% 发送一次
int lastPercent = -1;
const int percent = 30 + (70 * result.stats.totalOutputRows) / totalRows;
if (percent != lastPercent) {
    emit progressChanged(percent, QStringLiteral("正在流式合并"));
    lastPercent = percent;
}
```

---

## 新增辅助函数

### StreamRow 结构
```cpp
struct StreamRow {
    bool valid = false;
    qint64 timeNs = 0;
    QStringList cells;
};
```

### StreamCursor 结构
```cpp
struct StreamCursor {
    QString path;
    QString tag;
    QStringList headers;
    QVector<int> dataColumnIndices;
    QChar delimiter;
    QFile file;
    std::unique_ptr<QTextStream> in;
    StreamRow lower;
    StreamRow upper;
    bool eof;
};
```

### 辅助函数列表

1. **isTimeColumnName()**：判断列名是否为时间列
2. **makeFileTag()**：从文件路径生成标签（用于输出列名）
3. **safeCell()**：安全访问 cells 数组，避免越界
4. **openStreamCursor()**：打开文件游标，解析表头和数据列索引
5. **readNextParsedRow()**：读取并解析下一行数据
6. **primeUpper()**：初始化游标的 upper 行
7. **advanceCursorToTarget()**：推进滑动窗口到目标时间点
8. **interpolateValue()**：根据插值方法计算目标值
9. **buildOutputHeaders()**：构建输出表头（带文件标签前缀）

---

## 技术决策

### 1. 严格单增 vs 非降序

**决策**：采用**严格单增**语义

**原因**：
- 重复时间在插值中需要单独处理
- 避免 `upperTime == lowerTime` 导致分母为 0
- 重复时间直接归类为 `IndexedFallback`，由回退机制处理

### 2. 基准时间轴来源

**决策**：在合并阶段**重新顺序读取基准文件**

**原因**：
- 稀疏索引不应驱动主合并循环
- metadata 只提供统计信息
- 稀疏索引用于 fallback，不用于主时间轴

### 3. 线性插值策略

**决策**：
- 能转 `double` 就线性插值
- 不能转就自动退化为 `Nearest`

**原因**：
- 适合作为第一版工程实现
- 兼顾数值列和非数值列

### 4. IndexedFallback 处理

**决策**：未实现前明确报错，不输出伪正确结果

**原因**：
- 避免生成"看起来正常但其实错误"的输出文件
- 对联调和测试更友好

---

## 代码质量分析

### 优点

1. **方向正确**：架构设计符合"顺序流式主路径优先"原则
2. **代码结构扎实**：匿名命名空间封装辅助结构，职责清晰
3. **复用现有组件**：`TimeParser` / `FileReader` / `DelimiterDetector`
4. **输出管道成型**：编码、分隔符、flush 节奏控制完整
5. **插值算法完整**：支持 None / Nearest / Linear

### 风险点

1. **QTextStream + file.pos() 的 offset 风险**
   - `QTextStream` 有自己的缓冲
   - `QFile::pos()` 不一定总能严格表示"这一行文本的起始字节偏移"
   - 如果后面真要拿 `offset` 做随机 seek 或回退读取，有风险
   - **建议**：最终改成 `QFile::readLine()` 做原始字节读取

2. **线性插值只对数值列有效**
   - 当前实现已自动退化为 Nearest
   - 但可能需要更明确的文档说明

---

## 测试建议

### 必须补充的 6 个测试

1. **索引采样回归测试**
   - 验证 5000 行时，索引数不是 1
   - 真的是按预期 stride 采样

2. **interval 采样正确性测试**
   - 验证样本分布和"按原始行号采样"一致

3. **严格单增测试**
   - 验证单增文件被分类成 `SequentialUniform` 或 `SequentialCorrected`

4. **重复时间测试**
   - 验证重复时间直接 fallback

5. **基准时间轴测试**
   - 验证输出时间列来自真实读取，不是 `row_123` 占位值

6. **取消测试**
   - 验证取消不触发错误语义
   - `result.cancelled == true`

---

## 验收标准

### 扫描阶段（90/100）

- [x] 索引采样基于 `dataRowIndex`
- [x] interval 采样基于 `intervalIndex`
- [x] monotonic 判定为严格单增
- [x] 总是把最后一个点补进索引
- [x] 取消语义正确

### 合并阶段（80/100）

- [x] 基准文件顺序读取
- [x] 从文件滑动窗口推进
- [x] 支持 None / Nearest / Linear 插值
- [x] 输出真实数据，不是全 0 占位
- [x] progress 信号限制频率
- [ ] IndexedFallback 路径实现（ pending）

### 整体阶段完成度：约 70/100

---

## 下一步优先级

### P0：完成 IndexedFallback 路径

1. 实现 seek 到指定 offset
2. 实现二分查找（基于稀疏索引）
3. 实现 fallback 滑动窗口初始化

### P1：验证正确性

1. 两个严格单增文件，Nearest 能输出真实对齐结果
2. 验证重复时间直接 fallback
3. 验证线性插值对数值列有效

### P2：性能基准测试

1. 10 万行、50 万行、100 万行 + 性能测试
2. 对比旧引擎性能差异
3. 验证内存占用

---

## 技术债务

1. **QTextStream + pos() 的 offset 精度问题**
   - 当前保留现有模式，是为了不一下改太多
   - 最终应改成 `QFile::readLine()` 做原始字节读取

2. **线性插值文档说明不足**
   - 需要明确说明"非数值列自动退化为 Nearest"

3. **重复时间语义**
   - 当前直接 fallback
   - 可能需要文档说明这是预期行为

---

## 总结

这版代码修复了 4 个关键 bug，实现了真正的流式合并主路径。核心改进包括：

1. **修复索引采样 bug**：基于 `dataRowIndex` 而非 `indices.size()`
2. **修复 interval 采样 bug**：基于 `intervalIndex` 而非 `sampledIntervals.size()`
3. **严格单增判定**：`currentNs <= prevTimeNs`
4. **实现真实合并**：顺序读取基准文件 + 滑动窗口 + 插值

当前代码已经可以用于验证 A/B 类文件的顺序主路径性能和正确性。C 类 fallback 路径待后续实现。

**整体评价**：走在正确路上的实质性进展，不是失败，但也还不能收工。最关键的两个问题（采样 bug + 基准时间轴设计）已解决。
