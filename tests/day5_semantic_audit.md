# Day5 语义审计报告

## 审计时间
2026-03-22

## 审计范围
1. CLI -t 参数语义
2. tolerance 在 Interpolator、DataFileMerger、StreamMergeEngine 中的传递
3. 取消语义实现
4. 文档 vs 代码契约

---

## 1. tolerance 审计

### 1.1 CLI -t 参数

**代码位置**: main.cpp L175-181

```cpp
QCommandLineOption toleranceOption(
    QStringList() << "t" << "tolerance",
    "时间容差（毫秒），0 表示不限制（默认：0）",
    "milliseconds",
    "0"
);
```

**转换逻辑**: main.cpp L320-326
```cpp
if (tolerance > 0) {
    config.timeToleranceUs = tolerance * 1000;  // 毫秒→微秒
} else {
    config.timeToleranceUs = 0;  // 0 表示不限制
}
```

**问题**:
1. 帮助文案写"毫秒"，但内部转微秒
2. 0 被解释为"不限制"而非"精确匹配"
3. -1 分支未处理

### 1.2 Interpolator tolerance 支持

**签名**: Interpolator.h L34-38
```cpp
static InterpolationResult interpolate(
    const QVector<TimeMerge::TimePoint>& timePoints,
    const QVector<double>& values,
    const TimeMerge::TimePoint>& targetTime,
    InterpolationMethod method,
    int64_t toleranceUs = -1  // 默认-1表示无限制
);
```

**nearest 行为**: Interpolator.cpp L28-50
- toleranceUs >= 0 时，超容差返回 NaN
- toleranceUs = -1 时，无限制

**linear 行为**: Interpolator.cpp L52-87
- 目标在范围外时，toleranceUs >= 0 且 diff > tolerance → NaN
- toleranceUs = -1 时，外推到端点

### 1.3 DataFileMerger tolerance 传递

**代码**: DataFileMerger.cpp L190-195
```cpp
InterpolationResult result = Interpolator::interpolate(
    otherFile.timePoints,
    columnData,
    targetTime,
    m_config.interpolation
    // ❌ 未传 toleranceUs！使用默认值 -1
);
```

**问题**: DataFileMerger 调用时**未传** m_config.timeToleranceUs，导致实际总是-1（无限制）

### 1.4 StreamMergeEngine tolerance 支持

**函数签名**: StreamMergeEngine.cpp L44-48
```cpp
static QString interpolateBetweenRows(
    const StreamRow& lower,
    const StreamRow& upper,
    int sourceColumnIndex,
    qint64 targetNs,
    StreamMergeEngine::InterpolationMethod method
    // ❌ 没有 tolerance 参数！
);
```

**调用链**:
1. advanceCursorToTarget() - 无 tolerance 检查
2. interpolateBetweenRows() - 无 tolerance 参数
3. fallback 路径 - 同样无 tolerance

**结论**: Stream 引擎**完全未实现** tolerance 逻辑

---

## 2. 取消语义审计

### 2.1 StreamMergeEngine 取消

**方法签名**: StreamMergeEngine.h L125
```cpp
void cancel() noexcept;
```

**实现**: StreamMergeEngine.cpp L455-458
```cpp
void StreamMergeEngine::cancel() noexcept {
    m_cancelled.store(true);
}
```

**状态检查点**: doMerge() 中多处检查 shouldCancel()

**取消时行为**:
- result.cancelled = true
- result.errorMessage.clear()
- 发"任务已取消"消息

### 2.2 DataFileMerger 取消

**实现**: DataFileMerger.cpp L11,32,87
```cpp
m_cancelled = false;  // 重置
if (m_cancelled) { ... }  // 检查
```

**取消时行为**: 返回 result，warnings 包含"操作已取消"

### 2.3 CLI 取消支持

**结论**: CLI **无取消参数**，无法通过 CLI 触发取消

---

## 3. 代码 vs 文档契约差异清单

| 项目 | 文档定义 | 代码实际 | 严重性 |
|------|----------|----------|--------|
| -t 单位 | 微秒 | 毫秒（文案）+ 微秒（转换） | 低 |
| -t 0 语义 | 精确匹配 | 不限制 | **高** |
| -t -1 语义 | 无限制 | 未实现 | **高** |
| legacy tolerance | 生效 | 忽略（总是-1） | **高** |
| stream tolerance | 生效 | 完全未实现 | **高** |
| CLI 取消 | 支持 | 不支持 | 中 |
| 取消语义分离 | cancelled+无error | 已实现 | 低 |

---

## 4. 审计结论

### 4.1 tolerance 状态
- **Interpolator**: ✅ 支持完整
- **DataFileMerger**: ⚠️ 未传递，实际恒为-1
- **StreamMergeEngine**: ❌ 完全未实现
- **CLI**: ⚠️ 文案不一致

### 4.2 修复优先级
1. **高**: Stream tolerance 实现缺失
2. **高**: DataFileMerger tolerance 未传递
3. **高**: CLI -t 文案修正
4. **中**: CLI 取消参数

---

## 5. 测试建议

### 5.1 tolerance 测试重点
1. 验证 Interpolator 单体：tolerance=0, >0, -1 行为
2. 验证 legacy 实际是否忽略 tolerance
3. 验证 stream tolerance 是否真的不工作

### 5.2 取消测试重点
1. API 层取消（需写单元测试）
2. 验证 cancelled + errorMessage.clear 分离语义

---

生成时间：2026-03-22
版本：v1
