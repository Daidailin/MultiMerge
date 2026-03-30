# Tolerance 功能修复最终报告

## 修复时间
2026-03-22

## 修复概述
根据 day5_1.txt 的修改计划，完成了 tolerance 功能的全面修复，统一了代码库中的 tolerance 语义。

---

## 修复文件清单

### 1. CLI 参数修复 - [cli/main.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/cli/main.cpp)

**变更内容:**
- 修改 `-t` 参数帮助文案：单位改为**微秒**，语义明确为 `-1`=无限制, `0`=精确匹配
- 默认值从 `0` 改为 `-1`
- 删除 `none` 插值选项，只支持 `nearest/linear`
- 添加参数验证逻辑，拒绝 `< -1` 的值

**关键修改:**
```cpp
// 修改前
QCommandLineOption toleranceOption(
    QStringList() << "t" << "tolerance",
    "时间容差（毫秒），0 表示不限制（默认：0）",
    "milliseconds",
    "0"
);

// 修改后
QCommandLineOption toleranceOption(
    QStringList() << "t" << "tolerance",
    "时间容差（微秒），-1 表示不限制，0 表示精确匹配（默认：-1）",
    "microseconds",
    "-1"
);
```

---

### 2. Legacy 引擎修复 - [core/DataFileMerger.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/DataFileMerger.cpp)

**变更内容:**
- 在调用 `Interpolator::interpolate()` 时添加 `m_config.timeToleranceUs` 参数
- 修改变量名避免与 `MergeResult& result` 冲突

**关键修改:**
```cpp
// 修改前
InterpolationResult result = Interpolator::interpolate(
    otherFile.timePoints,
    columnData,
    targetTime,
    m_config.interpolation
);

// 修改后
InterpolationResult interpResult = Interpolator::interpolate(
    otherFile.timePoints,
    columnData,
    targetTime,
    m_config.interpolation,
    m_config.timeToleranceUs
);
```

---

### 3. Stream 引擎修复 - [core/StreamMergeEngine.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/StreamMergeEngine.cpp)

**变更内容:**
- 修改 `interpolateBetweenRows()` 和 `interpolateValue()` 函数签名，添加 `toleranceNs` 参数
- 新增 `withinTolerance()` 辅助函数
- 实现完整的容差检查逻辑
- 在 `mergeStreaming()` 中计算 toleranceNs 并传递给所有插值调用

**关键修改:**
```cpp
// 新增辅助函数
static bool withinTolerance(qint64 distanceNs, qint64 toleranceNs)
{
    return toleranceNs < 0 || distanceNs <= toleranceNs;
}

// 函数签名修改
static QString interpolateBetweenRows(const StreamRow& lower,
                                     const StreamRow& upper,
                                     int sourceColumnIndex,
                                     qint64 targetNs,
                                     StreamMergeEngine::InterpolationMethod method,
                                     qint64 toleranceNs);  // 新增参数

// 容差检查逻辑
if (method == StreamMergeEngine::InterpolationMethod::Nearest) {
    const qint64 nearestDist = qMin(dl, du);
    if (!withinTolerance(nearestDist, toleranceNs)) {
        return QStringLiteral("NaN");
    }
    return (dl <= du) ? lowerValue() : upperValue();
}

// linear
if (!withinTolerance(dl, toleranceNs) || !withinTolerance(du, toleranceNs)) {
    return QStringLiteral("NaN");
}
```

---

### 4. FileReader 修复 - [io/FileReader.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/io/FileReader.cpp) ⚠️ 残留风险点修复

**变更内容:**
- 修复 `findNearestTime()` 函数中的 tolerance 语义，与 Interpolator 保持一致
- 统一语义：`-1`=unlimited, `0`=exact match, `>0`=容差值

**关键修改:**
```cpp
// 修改前 (旧语义: 0=放开, >0=限制)
if (toleranceUs == 0 || diff <= toleranceUs) {
    return 0;
}
...
if (toleranceUs > 0 && nearestDiff > toleranceUs) {
    return -1;
}

// 修改后 (新语义: -1=unlimited, 0=exact match)
if (toleranceUs < 0 || diff <= toleranceUs) {
    return 0;
}
...
if (toleranceUs >= 0 && nearestDiff > toleranceUs) {
    return -1;
}
```

---

## 统一后的 Tolerance 语义

| toleranceUs 值 | 语义 | 说明 |
|----------------|------|------|
| `-1` | unlimited | 无限制，总是返回最近点 |
| `0` | exact match | 精确匹配，超出范围返回 -1 |
| `>0` | tolerance | 容差范围内返回最近点，超出返回 -1 |

---

## 测试验证

### 测试数据
- **tol_t1_base.txt**: 时间点 0.0s, 1.0s, 2.0s
- **tol_t1_src.txt**: 时间点 0.05s, 1.05s, 2.05s (偏移 50ms = 50000us)

### 测试结果

| 测试 | tolerance | 引擎 | 期望 | 结果 |
|------|-----------|------|------|------|
| Test 1 | 0 (精确匹配) | Stream | NaN | ✅ 通过 |
| Test 2 | 100000 (100ms) | Stream | 插值成功 | ✅ 通过 |
| Test 3 | 0 (精确匹配) | Legacy | NaN | ✅ 通过 |
| Test 4 | -1 (无限制) | Stream | 插值成功 | ✅ 通过 |
| Test 5 | FileReader修复后 | Stream | NaN | ✅ 通过 |

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
- [x] **FileReader findNearestTime 统一 tolerance 语义** ⚠️ 新增修复

---

## 代码库 Tolerance 语义一致性

修复后，以下文件采用统一的 tolerance 语义：

1. ✅ [cli/main.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/cli/main.cpp) - CLI 参数处理
2. ✅ [core/DataFileMerger.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/DataFileMerger.cpp) - Legacy 引擎
3. ✅ [core/StreamMergeEngine.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/StreamMergeEngine.cpp) - Stream 引擎
4. ✅ [core/Interpolator.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/Interpolator.cpp) - 插值算法
5. ✅ [io/FileReader.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/io/FileReader.cpp) - 文件读取

---

## 修复完成总结

所有 tolerance 相关修复均已验证通过：
1. ✅ CLI 参数文案和语义修正
2. ✅ DataFileMerger 传递 toleranceUs 参数
3. ✅ StreamMergeEngine 实现 tolerance 检查逻辑
4. ✅ **FileReader 统一 tolerance 语义** (残留风险点已修复)

代码库中不再存在两套 tolerance 规则，所有组件采用统一的语义：
- `-1` = unlimited
- `0` = exact match
- `>0` = tolerance value

---

生成时间：2026-03-22
版本：v2 (Final)
