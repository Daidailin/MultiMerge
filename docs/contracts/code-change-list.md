# 代码改动列表

> **版本**: v1.0
> **日期**: 2026-03-21
> **依据**: [multimerge-behavior-spec-v1.md](./multimerge-behavior-spec-v1.md)

---

## 一、需要修改的代码文件

### 1.1 CLI 主程序

**文件**: `cli/main.cpp`

| 行号 | 当前位置 | 应改为 | 原因 |
|------|---------|--------|------|
| 135 | `QString("输入文件列表（至少 1 个）")` | `QString("输入文件列表（至少 2 个）")` | 统一为"至少 2 个" |
| 275-276 | `请至少指定一个输入文件` | `请至少指定两个输入文件` | 与引擎约束一致 |

### 1.2 流式引擎

**文件**: `core/StreamMergeEngine.cpp`

| 行号 | 位置 | 状态 | 说明 |
|------|------|------|------|
| 567-573 | `validateInputs()` | ✅ 已正确 | 要求至少 2 个输入文件 |
| 237 | `QStringLiteral("%1.%2").arg(tag, header)` | ✅ 已修改 | 输出表头不添加文件名前缀 |
| 400-404 | `interpolateBetweenRows()` | ✅ 已修改 | 移除 None 分支 |

**文件**: `core/StreamMergeEngine.h`

| 行号 | 位置 | 状态 | 说明 |
|------|------|------|------|
| - | `InterpolationMethod::None` | ✅ 已移除 | 只保留 Nearest 和 Linear |

### 1.3 插值器

**文件**: `core/Interpolator.h`

| 位置 | 状态 | 说明 |
|------|------|------|
| `InterpolationResult` 结构体 | ✅ 已添加 | 返回值 + isValid + warningMessage |
| `tolerance` 参数 | ✅ 已支持 | toleranceUs 参数 |
| `None` 枚举 | ✅ 已移除 | 只保留 Nearest 和 Linear |

**文件**: `core/Interpolator.cpp`

| 位置 | 状态 | 说明 |
|------|------|------|
| `nearest()` 实现 | ✅ 已修改 | tolerance 检查 |
| `linear()` 实现 | ✅ 已修改 | tolerance 检查 |
| 警告信息 | ✅ 已添加 | 记录超容差情况 |

### 1.4 数据合并器

**文件**: `core/DataFileMerger.cpp`

| 行号 | 位置 | 状态 | 说明 |
|------|------|------|------|
| 185-205 | `Interpolator::interpolate()` 调用 | ✅ 已修改 | 处理 InterpolationResult |

---

## 二、已完成的改动

| 文件 | 改动 | 状态 |
|------|------|------|
| Interpolator.h | 重构返回 InterpolationResult | ✅ 完成 |
| Interpolator.cpp | 实现 tolerance 语义 | ✅ 完成 |
| StreamMergeEngine.h | 移除 None 枚举 | ✅ 完成 |
| StreamMergeEngine.cpp | 移除 None 分支，修改表头 | ✅ 完成 |
| DataFileMerger.cpp | 适配 InterpolationResult | ✅ 完成 |
| main.cpp | 移除 none 选项 | ✅ 完成 |

---

## 三、待完成的改动

| 文件 | 改动 | 优先级 |
|------|------|--------|
| cli/main.cpp | 修改帮助文案"至少 1 个"→"至少 2 个" | P1 |

---

## 四、验证方法

### 4.1 编译验证

```bash
cd build && cmake --build .
```

### 4.2 功能验证

```powershell
cd test_data/scripts && .\run_format_tests.ps1
```

### 4.3 边界测试

```bash
# 1个文件应该报错
.\MultiMerge.exe file1.txt -o output.txt

# tolerance=0 应该精确匹配
.\MultiMerge.exe -t 0 file1.txt file2.txt -o output.txt
```

---

**状态**: 代码改动基本完成，CLI 文案待更新
