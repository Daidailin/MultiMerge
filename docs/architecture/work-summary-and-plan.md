# MultiMerge 项目工作总结与实施规划

> **文档版本**: v3.3
> **总结时间**: 2026-03-21
> **总结范围**: 已完成功能、待收敛问题、下一步计划
> **重要更正**: v3.3 更新插值器语义冻结、输出表头策略

---

## 一、项目整体状态

### 1.1 当前所处阶段

**M1：编译验证 + 语义收敛 + 正确性测试准备阶段**

核心模块已编码完成，但关键语义和验证仍未收敛。项目主干已经成型，但尚未完成端到端的行为验证。

### 1.2 已具备的核心模块

| 模块 | 状态 | 说明 |
|------|------|------|
| CLI 框架 | ✅ 已落地 | 参数解析、引擎切换基本可用 |
| DataFileMerger（旧引擎） | ✅ 已落地 | 完整实现，与新引擎语义已统一 |
| StreamMergeEngine（新引擎） | ✅ 主流程完成 | 扫描→分类→合并流程已通 |
| TimePoint 时间对象 | ✅ 已落地 | 纳秒精度运算、24小时循环处理 |
| TimeParser 时间解析器 | ✅ 已统一 | 同时支持冒号和点号格式 |
| Interpolator 插值器 | ✅ 已冻结 | nearest/linear 语义已定义，tolerance 已生效 |
| DelimiterDetector 分隔符检测 | ✅ 已落地 | 基本功能完整 |

### 1.3 已收敛的关键问题

| 问题 | 状态 | 解决方案 |
|------|------|---------|
| 新旧引擎时间解析格式不统一 | ✅ 已修复 | TimeParser 同时支持冒号和点号格式 |
| FileReader 独立解析逻辑 | ✅ 已统一 | FileReader::parseLine 改为调用 TimeParser |

---

## 二、已完成工作总结

### 2.1 时间系统

**TimePoint 类** - 时间点表示与运算
- 支持毫秒和微秒两种精度（内部纳秒计算）
- 24 小时循环标准化处理（跨天场景）
- 算术运算：加减法（`operator+`, `operator-`）
- 比较运算：大小比较（`operator<`, `operator>`, `operator==`）
- 格式化输出：自动补零

**关键代码位置**：
- [TimePoint.h](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/TimePoint.h)
- [TimePoint.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/TimePoint.cpp)

**TimeParser 类** - 时间字符串解析
- 单例模式实现（线程安全）
- 支持格式：`HH:MM:SS.fff`（点号毫秒）、`HH:MM:SS.ffffff`（点号微秒）、`HH:MM:SS:fff`（冒号毫秒）、`HH:MM:SS:ffffff`（冒号微秒）
- 精度自动推断（1-3位毫秒，4-6位微秒）

**关键代码位置**：
- [TimeParser.h](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/TimeParser.h)
- [TimeParser.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/TimeParser.cpp)

---

### 2.2 数据合并引擎

#### DataFileMerger（旧引擎）

**功能模块**：
- 文件预扫描与单调性检测
- 基于时间对齐的合并逻辑
- 最近邻/线性插值

**关键代码位置**：
- [core/DataFileMerger.h](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/DataFileMerger.h)
- [core/DataFileMerger.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/DataFileMerger.cpp)

#### StreamMergeEngine（新引擎）

**已实现的主流程**：
- 边界与接口定义（Config、FileMetadata、MergeResult）
- 预扫描功能（文件扫描、时间解析）
- 文件分类（SequentialUniform / SequentialCorrected / IndexedFallback）
- 顺序推进主路径（滑动窗口查找）
- IndexedFallback（乱序文件全加载 + 排序 + 二分查找）
- 取消语义（与失败状态分离）
- 输入校验（文件可读性、重复检查）

**关键代码位置**：
- [core/StreamMergeEngine.h](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/StreamMergeEngine.h)
- [core/StreamMergeEngine.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/StreamMergeEngine.cpp)

**⚠️ 架构已具备但未验证的优化点**：
- 预测查找算法：架构已预留，需验证实际是否生效
- 轻量级时间索引：代码中有采样逻辑，需性能测试确认

---

### 2.3 插值算法

**Interpolator 类**

| 插值方法 | 状态 | 说明 |
|---------|------|------|
| nearest | ✅ 已冻结 | 最近点插值，tolerance 控制容差 |
| linear | ✅ 已冻结 | 线性插值，前后点都必须在 tolerance 内 |

**关键代码位置**：
- [core/Interpolator.h](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/Interpolator.h)
- [core/Interpolator.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/Interpolator.cpp)

**行为规范**：[merge-behavior-spec.md](file:///e:/TraeCN/TraeCN_project/test/docs/contracts/merge-behavior-spec.md)

---

### 2.4 输出表头策略

**策略**：直接使用原始列名，不添加文件名前缀

**示例**：
- 输入1：`Time, Temp, Pressure`
- 输入2：`Time, Sensor1, Sensor2`
- 输出：`Time, Temp, Pressure, Sensor1, Sensor2`

**关键代码位置**：
- [core/StreamMergeEngine.cpp#L236](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/StreamMergeEngine.cpp#L236)

---

### 2.5 命令行接口

**已实现的命令选项**：
```bash
MultiMerge.exe [选项] <输入文件 1> <输入文件 2> ...

选项：
  -o, --output <文件名>       输出文件路径
  -i, --interpolation <方法>  插值方法（nearest/linear）
  -d, --delimiter <分隔符>    输出分隔符（space/comma/tab）
  -e, --encoding <编码>       输出编码（UTF-8/GBK）
  -t, --tolerance <微秒>     时间容差（0=精确匹配，-1=无限制）
  -E, --engine <引擎>         合并引擎：legacy（旧引擎）, stream（新流式引擎）
  -v, --verbose               详细输出模式
  -h, --help                  显示帮助
  --version                   显示版本号
```

**关键代码位置**：
- [cli/main.cpp](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/cli/main.cpp)

**⚠️ 已知问题**：
- 帮助文案写"至少 1 个文件"，但 StreamMergeEngine::validateInputs() 要求至少 2 个

---

### 2.6 辅助模块

| 模块 | 状态 | 说明 |
|------|------|------|
| FileReader | ✅ 已落地 | 文件读取、行解析 |
| DelimiterDetector | ✅ 已落地 | 分隔符检测 |

**关键代码位置**：
- [io/FileReader.h](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/io/FileReader.h)
- [utils/DelimiterDetector.h](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/utils/DelimiterDetector.h)

---

### 2.7 测试基础设施

**测试目录结构**：
```
test_data/
├── input/                 # 测试输入数据
│   ├── 01_dot_ms.txt     # 点号毫秒格式 (HH:MM:SS.fff)
│   ├── 02_dot_us.txt     # 点号微秒格式 (HH:MM:SS.ffffff)
│   ├── 03_colon_ms.txt   # 冒号毫秒格式 (HH:MM:SS:fff)
│   ├── 04_colon_us.txt   # 冒号微秒格式 (HH:MM:SS:ffffff)
│   ├── sensor1.txt       # 原始测试数据
│   └── sensor2.txt       # 原始测试数据
├── output/                # 测试输出结果
│   ├── test01_dotms_colonus.txt
│   ├── test02_dotus_colonms.txt
│   ├── test03_colonms_dotus.txt
│   ├── test04_colonus_dotms.txt
│   └── test05_sensor_merge.txt
└── scripts/
    └── run_format_tests.ps1
```

**测试覆盖矩阵**：

| 测试编号 | 输入1 | 输入2 | 验证点 | 状态 |
|---------|-------|-------|--------|------|
| test01 | 01_dot_ms | 04_colon_us | 点号毫秒 + 冒号微秒 | ✅ |
| test02 | 02_dot_us | 03_colon_ms | 点号微秒 + 冒号毫秒 | ✅ |
| test03 | 03_colon_ms | 02_dot_us | 冒号毫秒 + 点号微秒 | ✅ |
| test04 | 04_colon_us | 01_dot_ms | 冒号微秒 + 点号毫秒 | ✅ |
| test05 | sensor1 | sensor2 | 原始数据合并 | ✅ |

**测试通过率**：5/5 (100%)

**关键代码位置**：
- [test_data/scripts/run_format_tests.ps1](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/test_data/scripts/run_format_tests.ps1)
- [test_data/input/README.md](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/test_data/input/README.md)

---

## 三、已解决的技术问题

### 3.1 本次开发周期解决的关键问题

| 问题 | 解决方案 | 状态 |
|------|---------|------|
| UTF-8 BOM 导致 MinGW 编译错误 | PowerShell UTF8Encoding(false) 移除 BOM | ✅ 已解决 |
| Q_OBJECT 类链接错误（vtable） | 启用 CMAKE_AUTOMOC ON | ✅ 已解决 |
| std::isnan 未定义 | 添加 #include <cmath> | ✅ 已解决 |
| Placement new 匹配失败 | 添加 -fno-aligned-new，QVector<QSharedPointer> 替代 | ✅ 已解决 |

### 3.2 BOM 移除 Skill

**位置**：[.trae/skills/remove-bom/](file:///e:/TraeCN/TraeCN_project/test/.trae/skills/remove-bom/)

**使用方式**：
```powershell
& "E:\TraeCN\TraeCN_project\test\.trae\skills\remove-bom\remove-bom.ps1" -Path "E:\YourProject"
```

---

## 四、待收敛的关键问题（最高优先级）

### 4.1 时间解析格式不统一 ✅ 已修复

**问题**：TimeParser 只支持冒号格式，FileReader 使用点号格式，两条解析路径不一致。

**解决方案**：
- TimeParser 新增 `parseDotFractionalTime()` 方法，同时支持冒号和点号格式
- FileReader::parseLine() 改为调用 TimeParser::instance().parse()
- 统一为单一真相来源

**验证**：5种格式测试全部通过（点号毫秒、点号微秒、冒号毫秒、冒号微秒、原始数据）

**文档**：[time-format-conversion.md](file:///e:/TraeCN/TraeCN_project/test/docs/contracts/time-format-conversion.md)

### 4.2 tolerance 参数 ✅ 已完成 (2026-03-22)

**已确定的 tolerance 行为**：
- `tolerance = -1`：无限制（默认）
- `tolerance = 0`：必须精确匹配，否则 NaN
- `tolerance > 0`：在容差范围内查找/插值

**实现状态**：
- ✅ Interpolator 类已更新，支持 tolerance 参数
- ✅ Nearest 和 Linear 方法都支持 tolerance 检查
- ✅ DataFileMerger 传递 toleranceUs 参数
- ✅ StreamMergeEngine 实现 tolerance 检查逻辑
- ✅ FileReader 统一 tolerance 语义
- ✅ CLI -t 参数文案、单位、默认值已修正

**验证测试**：
- tolerance = 0 → 正确输出 NaN
- tolerance = 100000us → 成功插值
- tolerance = -1 → 成功插值（无限制）

**规范文档**：[merge-behavior-spec.md](file:///e:/TraeCN/TraeCN_project/test/docs/contracts/merge-behavior-spec.md)
**详细报告**：[tolerance_fix_final_report.md](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/test_data/tolerance_fix_final_report.md)

---

### 4.3 输出表头策略 ✅ 已更新

**策略变更**：直接使用原始列名，不添加 `文件名.` 前缀

**代码位置**：[StreamMergeEngine.cpp#L236](file:///e:/TraeCN/TraeCN_project/test/QT_Project/MultiMerge/core/StreamMergeEngine.cpp#L236)

---

### 4.4 CLI 文案与引擎约束（P1）

**问题描述**：
- main.cpp 帮助文案：`至少 1 个文件`
- StreamMergeEngine::validateInputs()：`if (inputFiles.size() < 2)`

**修复方向**：统一文案或统一约束

---

## 五、待实现功能清单

### 5.1 M1 阶段（P0）

- [x] 修复时间解析格式不统一问题
- [ ] 功能正确性测试：两个严格单增文件合并
- [ ] IndexedFallback 测试：乱序文件处理
- [ ] 混合模式测试：A + B + C 类文件混合

### 5.2 M1 阶段（P1）

- [x] tolerance 参数真正生效
- [x] None 插值语义明确定义
- [ ] CLI 文案与引擎约束统一
- [ ] 性能基准测试（10万/50万/100万行）

### 5.3 GUI 开发（P2）

| 功能 | 优先级 |
|------|--------|
| 主窗口框架（gui/ 目录、MainWindow） | P2 |
| 文件管理面板（QTableView） | P2 |
| 数据预览面板 | P2 |
| 配置面板 | P2 |
| 进度面板 | P2 |
| 后台异步处理（QThread） | P2 |

### 5.4 高级功能（P3）

- [ ] CSV/Excel/JSON 导出
- [ ] 前向填充插值
- [ ] 自定义时间网格对齐
- [ ] 配置模板保存/加载
- [ ] 多线程并行（预留接口）

### 5.5 测试与文档（P2/P3）

- [ ] 单元测试（TimePoint、TimeParser、StreamMergeEngine）
- [ ] 集成测试
- [ ] API 文档（Doxygen）
- [ ] 用户手册

---

## 六、下一步实施计划

### 6.1 本周：M1 语义收敛（最高优先级）

**Day 1-2：时间格式统一**
- 确定时间格式方案（点号格式优先）
- 修复 TimeParser 或 FileReader
- 验证新旧两条引擎都能正确解析测试数据

**Day 3-4：功能正确性测试**
- 单增文件合并测试
- IndexedFallback 测试
- 混合模式测试

**Day 5：其他语义问题**
- tolerance 参数闭环
- None 插值语义定义
- CLI 文案修正

**交付物**：
- 时间格式问题修复
- 功能正确性验证报告
- M1 里程碑关闭

---

### 6.2 下一步里程碑

| 里程碑 | 目标 | 验收标准 |
|--------|------|---------|
| **M1: 编译验证完成** | 本周 | ✅ 编译无错误<br>✅ 语义收敛<br>✅ 功能测试通过 |
| **M2: GUI Alpha** | 2 周后 | 可导入文件、可配置参数、可执行合并 |
| **M3: GUI Beta** | 4 周后 | 完整 GUI + 配置模板 + 多格式导出 |
| **M4: RC 版本** | 5 周后 | 性能达标、测试覆盖>80%、无严重 Bug |
| **M5: v1.0 发布** | 6 周后 | 多平台验证、用户测试通过、文档完整 |

---

## 七、已验证事实 vs 待验证预期

### 7.1 已验证事实

| 说法 | 验证结果 |
|------|---------|
| 项目有两条引擎（DataFileMerger + StreamMergeEngine） | ✅ 确认 |
| CLI 已支持 -E legacy/stream 切换 | ✅ 确认 |
| 时间系统模块存在且有具体实现 | ✅ 确认 |
| 插值器有最近邻和线性插值实现 | ✅ 确认 |
| StreamMergeEngine 有三类文件分流逻辑 | ✅ 确认 |
| BOM 问题已通过 UTF8Encoding(false) 修复 | ✅ 确认 |
| QVector<QSharedPointer> 替代方案有效 | ✅ 确认 |

### 7.2 架构已具备但待验证

| 说法 | 当前状态 |
|------|---------|
| 预测查找 O(1) | 分类与采样已完成，预测查找逻辑待验证 |
| 轻量级时间索引（1.6MB/百万行） | 采样逻辑已实现，待性能测试 |
| 内存降低 10-40 倍 | 架构方向正确，待实测数据 |
| 多线程预留接口 | 代码有预留，待实现 |

### 7.3 不准确的表述（已修正）

| 原表述 | 修正后 |
|--------|--------|
| "StreamMergeEngine V3 完成 90%~100%" | "主流程已完成，关键语义和验证仍未收敛" |
| "预测查找 O(1) 已完成" | "分类与采样已完成，预测查找未见完整落地" |
| "无插值（直传）已完成" | "当前实现为精确匹配，否则空值/NaN" |
| "TIME 列自动识别已完成" | "当前仍默认首列为时间列" |
| "仅测试数据格式不匹配" | "新旧时间解析链路本身不统一" |
| "性能优势已验证" | "架构已朝该方向实现，尚待验证" |

---

## 八、文档状态

**文档版本**: v3.0
**更新内容**:
- 修正多处完成度偏乐观的表述
- 新增第四章"待收敛的关键问题"
- 更新第七章"已验证事实 vs 待验证预期"
- 调整里程碑验收标准

**下次更新**: M1 里程碑关闭后

---

**文档状态**: ✅ 已更新至 v3.0，反映更准确的项目状态
**核心结论**: 项目主干已成型，核心模块已落地，但语义收敛和正确性验证仍在进行中
