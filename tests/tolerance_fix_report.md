# Tolerance 功能修复报告

## 修复时间
2026-03-22

## 修复内容
根据 day5_1.txt 的修改计划，完成了以下三个修复：

---

## 1. CLI 参数修复 (main.cpp)

### 修改前
```cpp
QCommandLineOption toleranceOption(
    QStringList() << "t" << "tolerance",
    "时间容差（毫秒），0 表示不限制（默认：0）",
    "milliseconds",
    "0"
);
```

### 修改后
```cpp
QCommandLineOption toleranceOption(
    QStringList() << "t" << "tolerance",
    "时间容差（微秒），-1 表示不限制，0 表示精确匹配（默认：-1）",
    "microseconds",
    "-1"
);
```

### 关键变更
- 单位从**毫秒**改为**微秒**
- 默认值从 `0` 改为 `-1`
- 语义明确：`-1`=无限制, `0`=精确匹配, `>0`=容差值
- 删除 `none` 插值选项，只支持 `nearest/linear`

---

## 2. Legacy 引擎修复 (DataFileMerger.cpp)

### 修改前
```cpp
InterpolationResult result = Interpolator::interpolate(
    otherFile.timePoints,
    columnData,
    targetTime,
    m_config.interpolation
);
```

### 修改后
```cpp
InterpolationResult interpResult = Interpolator::interpolate(
    otherFile.timePoints,
    columnData,
    targetTime,
    m_config.interpolation,
    m_config.timeToleranceUs
);
```

### 关键变更
- 添加 `m_config.timeToleranceUs` 参数传递
- 修改变量名 `result` → `interpResult` 避免与 `MergeResult& result` 冲突

---

## 3. Stream 引擎修复 (StreamMergeEngine.cpp)

### 3.1 函数签名修改

**interpolateBetweenRows:**
```cpp
// 修改前
static QString interpolateBetweenRows(const StreamRow& lower,
                                     const StreamRow& upper,
                                     int sourceColumnIndex,
                                     qint64 targetNs,
                                     StreamMergeEngine::InterpolationMethod method);

// 修改后
static QString interpolateBetweenRows(const StreamRow& lower,
                                     const StreamRow& upper,
                                     int sourceColumnIndex,
                                     qint64 targetNs,
                                     StreamMergeEngine::InterpolationMethod method,
                                     qint64 toleranceNs);
```

**interpolateValue:**
```cpp
// 修改前
static QString interpolateValue(const StreamCursor& cursor,
                                int sourceColumnIndex,
                                qint64 targetNs,
                                StreamMergeEngine::InterpolationMethod method);

// 修改后
static QString interpolateValue(const StreamCursor& cursor,
                                int sourceColumnIndex,
                                qint64 targetNs,
                                StreamMergeEngine::InterpolationMethod method,
                                qint64 toleranceNs);
```

### 3.2 新增辅助函数
```cpp
static bool withinTolerance(qint64 distanceNs, qint64 toleranceNs)
{
    return toleranceNs < 0 || distanceNs <= toleranceNs;
}
```

### 3.3 容差检查逻辑实现

**核心逻辑:**
- `toleranceNs < 0`: 无限制，总是通过
- `toleranceNs >= 0`: 检查距离是否在容差范围内
- 超出容差返回 `"NaN"`

**单边有效情况:**
```cpp
if (!lower.valid) {
    const qint64 du = qAbs(upper.timeNs - targetNs);
    return withinTolerance(du, toleranceNs) ? upperValue() : QStringLiteral("NaN");
}

if (!upper.valid) {
    const qint64 dl = qAbs(targetNs - lower.timeNs);
    return withinTolerance(dl, toleranceNs) ? lowerValue() : QStringLiteral("NaN");
}
```

**Nearest 模式:**
```cpp
if (method == StreamMergeEngine::InterpolationMethod::Nearest) {
    const qint64 nearestDist = qMin(dl, du);
    if (!withinTolerance(nearestDist, toleranceNs)) {
        return QStringLiteral("NaN");
    }
    return (dl <= du) ? lowerValue() : upperValue();
}
```

**Linear 模式:**
```cpp
// linear
if (!withinTolerance(dl, toleranceNs) || !withinTolerance(du, toleranceNs)) {
    return QStringLiteral("NaN");
}
```

### 3.4 mergeStreaming 中传递 tolerance
```cpp
// 计算容差（纳秒）
const qint64 toleranceNs =
    (m_config.timeToleranceUs < 0) ? -1 : (m_config.timeToleranceUs * 1000);

// streaming 路径
rowData << interpolateValue(cursor,
                            sourceCol,
                            targetNs,
                            m_config.interpolationMethod,
                            toleranceNs);

// fallback 路径
rowData << interpolateBetweenRows(lower,
                                  upper,
                                  sourceCol,
                                  targetNs,
                                  m_config.interpolationMethod,
                                  toleranceNs);
```

---

## 测试计划

### 测试数据
- **tol_t1_base.txt**: 时间点 0.0s, 1.0s, 2.0s
- **tol_t1_src.txt**: 时间点 0.05s, 1.05s, 2.05s (偏移 50ms = 50000us)

### 测试用例

| 测试 | tolerance | 期望结果 | 说明 |
|------|-----------|----------|------|
| tol_exact | 0 | NaN | 精确匹配，50ms > 0 |
| tol_100ms | 100000 | 插值成功 | 50ms < 100ms |
| tol_50ms | 50000 | 插值成功 | 50ms <= 50ms |
| tol_40ms | 40000 | NaN | 50ms > 40ms |
| tol_unlimited | -1 | 插值成功 | 无限制 |

### 运行测试
```powershell
cd test_data
.\run_tolerance_tests.ps1
```

---

## 修复验证清单

- [x] CLI -t 参数文案修正
- [x] CLI -t 默认值改为 -1
- [x] CLI -t 单位改为微秒
- [x] CLI 删除 none 插值选项
- [x] DataFileMerger 传递 toleranceUs 参数
- [x] StreamMergeEngine interpolateBetweenRows 添加 toleranceNs 参数
- [x] StreamMergeEngine interpolateValue 添加 toleranceNs 参数
- [x] StreamMergeEngine 实现 withinTolerance 辅助函数
- [x] StreamMergeEngine 实现容差检查逻辑
- [x] StreamMergeEngine mergeStreaming 传递 tolerance

---

## 后续建议

1. **构建验证**: 在 MSVC2015 环境中重新构建项目
2. **功能测试**: 运行 run_tolerance_tests.ps1 验证修复效果
3. **回归测试**: 确保原有功能不受影响
4. **文档更新**: 更新 README 中的 CLI 参数说明

---

生成时间：2026-03-22
版本：v1
