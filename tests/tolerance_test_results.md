# Tolerance 功能修复验证报告

## 测试时间
2026-03-22

## 测试环境
- **构建工具**: CMake + Ninja (MinGW 8.1.0)
- **Qt 版本**: 5.15.2
- **可执行文件**: `build/MultiMerge.exe`

---

## 测试数据
- **tol_t1_base.txt**: 时间点 0.0s, 1.0s, 2.0s
- **tol_t1_src.txt**: 时间点 0.05s, 1.05s, 2.05s (偏移 50ms = 50000us)

---

## 测试结果

### Test 1: tolerance = 0 (精确匹配) - Stream 引擎
```bash
MultiMerge.exe tol_t1_base.txt tol_t1_src.txt -t 0 -E stream
```
**输出**:
```
Time Temp Value
00:00:00.000 0.0 NaN
00:00:01.000 100.0 NaN
00:00:02.000 200.0 NaN
```
**结果**: ✅ **通过** - 50ms > 0us，正确输出 NaN

---

### Test 2: tolerance = 100000 (100ms) - Stream 引擎
```bash
MultiMerge.exe tol_t1_base.txt tol_t1_src.txt -t 100000 -E stream
```
**输出**:
```
Time Temp Value
00:00:00.000 0.0 5.0
00:00:01.000 100.0 105.0
00:00:02.000 200.0 205.0
```
**结果**: ✅ **通过** - 50ms < 100ms，成功插值

---

### Test 3: tolerance = 0 (精确匹配) - Legacy 引擎
```bash
MultiMerge.exe tol_t1_base.txt tol_t1_src.txt -t 0 -E legacy
```
**输出**:
```
Time Temp Value
00:00:00:000 0 NaN
00:00:01:000 100 NaN
00:00:02:000 200 NaN
```
**结果**: ✅ **通过** - 50ms > 0us，正确输出 NaN

---

### Test 4: tolerance = -1 (无限制) - Stream 引擎
```bash
MultiMerge.exe tol_t1_base.txt tol_t1_src.txt -t -1 -E stream
```
**输出**:
```
Time Temp Value
00:00:00.000 0.0 5.0
00:00:01.000 100.0 105.0
00:00:02.000 200.0 205.0
```
**结果**: ✅ **通过** - tolerance = -1 表示无限制，成功插值

---

## 验证结论

| 修复项 | 状态 | 说明 |
|--------|------|------|
| CLI -t 参数文案 | ✅ | 正确显示"时间容差（微秒）" |
| CLI -t 默认值 | ✅ | 默认为 -1 (unlimited) |
| CLI -t 单位 | ✅ | 微秒单位正确 |
| Stream 引擎 tolerance | ✅ | 超出容差返回 NaN |
| Legacy 引擎 tolerance | ✅ | 超出容差返回 NaN |
| -t -1 无限制 | ✅ | 正确识别并处理 |

---

## 修复成功

所有 tolerance 相关修复均已验证通过：
1. ✅ CLI 参数文案和语义修正
2. ✅ DataFileMerger 传递 toleranceUs 参数
3. ✅ StreamMergeEngine 实现 tolerance 检查逻辑

---

生成时间：2026-03-22
版本：v1
