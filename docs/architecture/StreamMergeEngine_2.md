**这版代码已经完成了“定义 StreamMergeEngine 边界与接口”这个阶段目标，而且完成质量不错；但还没有完成我们约定的下一阶段目标。**
更准确地说，你现在完成的是**P0 的骨架层**，还没有进入**P0 的功能落地层**。这和我们前面讨论的分阶段路线是一致的：先把核心引擎边界立住，再实现预扫描、顺序推进主路径和回退机制。 performance-optimization-design

------

# 一、你这版代码已经达成了什么

从代码本身看，下面这些目标已经实现了，而且方向是对的：

## 1. 核心边界已经清晰

`StreamMergeEngine` 已经被定义成一个独立的核心引擎，对外只暴露：

- 配置读写：`setConfig()` / `config()`
- 同步执行入口：`merge()`
- 取消控制：`cancel()` / `isCancelled()`

这说明你已经把它和 GUI、CLI 参数解析、上层控制逻辑隔离开了。这正是我们要的“核心业务引擎边界”。StreamMergeEngine

## 2. 结果对象设计是合格的

你定义了：

- `FileMetadata`
- `MergeStats`
- `MergeResult`

这三个结构体组合起来，已经足够承载后续：

- 文件分类结果
- 各阶段耗时
- 警告/错误
- 输出文件路径
- 统计信息

这点做得很好，因为它让后面 CLI 输出、GUI 展示、性能对比测试都能共用同一份结果模型。StreamMergeEngine

## 3. 主流程骨架搭起来了

`doMerge()` 已经把主流程顺序串好了：

1. 输入校验
2. 扫描阶段
3. 分类统计
4. 取消检查
5. 合并阶段
6. 汇总 stats
7. 发进度和状态信号

这个流程骨架是合理的，后面你只需要把“扫描”和“合并”的内部实现替换进去，不需要改外部接口。StreamMergeEngine

## 4. 文件分类策略入口已经有了

你已经有了 `FileCategory` 和 `classifyFile()`，分类分成：

- `SequentialUniform`
- `SequentialCorrected`
- `IndexedFallback`

这非常关键，因为它是后续“顺序主路径 / 修正路径 / 回退路径”的总开关。这个设计符合我们之前确定的主架构。StreamMergeEngine performance-optimization-design

------

# 二、这版代码还没有完成什么

这里要说得直接一点：

**如果按“定义边界与接口”来验收，你通过了。**
**如果按“下一步实现部分是否已完成”来验收，还没有。**

原因很明确：

## 1. `scanSingleFile()` 目前仍是占位实现

你现在只是给 `FileMetadata` 填了默认值：

- `totalRows = 0`
- `isMonotonic = true`
- `isUniform = false`
- `category = SequentialCorrected`

并没有真正去做：

- 表头解析
- 时间列解析
- 起止时间统计
- 单增性检测
- 均匀性检测
- 必要时建立时间索引

所以“预扫描 + 文件分类”这个 P0 子目标还没完成。StreamMergeEngine

## 2. `mergeStreaming()` 仍是空骨架

目前这个函数除了检查取消，只做了：

- `result.stats.totalOutputRows = 0`
- `return true`

也就是说：

- 没有基准时间轴
- 没有顺序推进
- 没有滑动窗口
- 没有插值
- 没有输出文件写入
- 没有回退机制

所以“流式顺序合并主路径”这个核心目标还没实现。StreamMergeEngine

## 3. 统计字段多数还没有真正落地

`MergeStats` 里有：

- `writeElapsedMs`
- `peakMemoryBytes`
- `totalOutputRows`

但现在只有部分字段被赋值。尤其是：

- `writeElapsedMs` 没有统计
- `peakMemoryBytes` 没有统计
- `totalOutputRows` 始终是 0

这意味着结果对象设计是好的，但数据还没真正接上。

------

# 三、我认为目前最重要的几个问题

下面是我认为你现在最该优先修的点。

## 1. 取消语义不够干净

这是我最想先指出的问题。

现在在 `scanFiles()` 里，如果用户取消，你返回的是：

- `false`
- `error = "任务在扫描阶段被取消"`

然后 `doMerge()` 会把它当成扫描失败，并发 `errorOccurred(error)`。
这会导致“用户主动取消”被当成“错误”。StreamMergeEngine

### 这有什么问题

对 CLI/GUI 来说，“取消”和“失败”是两类完全不同的状态：

- 取消：用户主动终止，通常不应该弹错误
- 失败：系统异常、文件损坏、解析失败，才应该报错

### 建议怎么改

我建议你引入一个更明确的状态表达方式，至少二选一：

### 方案 A：统一用 `MergeResult`

让内部函数也区分：

- `success`
- `cancelled`
- `error`

### 方案 B：增加状态码

例如：

```
enum class EngineStatus {
    Ok,
    Cancelled,
    Failed
};
```

然后 `scanFiles()` / `mergeStreaming()` 返回状态码，而不是单纯 `bool`。

**这是你当前最值得立刻修的点。**

------

## 2. `validateInputs()` 还不够完整

现在只检查了：

- 输入文件数 >= 2
- 输出路径非空
- 输入文件存在且是文件

这是必要的，但还不够。StreamMergeEngine

### 建议补充

至少再加这些检查：

- 输出路径不能与任一输入路径相同
- 输出目录是否存在、可写
- 输入文件是否重复
- 输入文件是否可读
- 输出文件若已存在，是否允许覆盖

尤其是“输出文件和输入文件同路径”这个坑，一定要提前挡住。

------

## 3. `scanSingleFile()` 当前默认值有误导性

你现在默认：

- `isMonotonic = true`
- `isUniform = false`
- `category = SequentialCorrected`

这对于占位阶段可以理解，但它会带来一个问题：
**在扫描逻辑没实现前，分类统计看起来像是“真的扫过了”，其实没有。** StreamMergeEngine

### 建议

可以考虑增加一个显式状态，例如：

```
bool scanCompleted = false;
```

或者把默认分类改成更中性的“未分类”，比如：

```
enum class FileCategory {
    Unknown,
    SequentialUniform,
    SequentialCorrected,
    IndexedFallback
};
```

这样调试时不会误判。

------

## 4. `mergeStreaming()` 当前返回 `true` 容易掩盖未实现状态

现在只要没有取消，`mergeStreaming()` 就直接返回 `true`。这意味着 `doMerge()` 很可能把一次“没做任何实际工作”的调用当成成功路径继续走。StreamMergeEngine

### 建议

在真正实现之前，宁可明确失败，也不要伪成功。

例如：

```
result.errorMessage = QStringLiteral("mergeStreaming 尚未实现");
return false;
```

这样对测试和联调更安全。

------

## 5. `warningOccurred` 信号目前没有实际价值

虽然你定义了 `warningOccurred(const QString&)`，但当前代码里没有发出它。

### 建议

后续扫描阶段这些场景都适合发 warning：

- 行数不一致
- 个别时间解析失败但可继续
- 文件被判定为回退路径
- 均匀性不满足阈值
- 存在少量重复时间点

这样 GUI 和 CLI 都能展示非致命问题。

------

# 四、是否达成我们既定目标

我按“阶段目标”来给你判断：

## 目标 1：定义 StreamMergeEngine 边界与接口

**结论：已完成，且完成度较高。**

你已经做到了：

- 边界清晰
- 结果结构合理
- 流程骨架完整
- 配置与状态通路齐全

这个阶段我会给 **85~90 分**。

## 目标 2：实现下一步的核心能力

也就是前面规划里说的：

- 预扫描
- 文件分类
- 顺序推进
- 回退机制
- 结果写出

**结论：还没有完成。**

目前完成度大约是 **20~30%**，主要是“框架搭好，但功能没落地”。
这不是坏事，反而说明你没有在第一步就把实现和架构搅在一起，这点是对的。

------

# 五、我给你的具体建议

## 建议 1：下一步立刻实现 `scanSingleFile()`

这是你当前最正确的下一步。

优先做这几件事：

- 读取表头
- 识别分隔符和编码
- 逐行解析时间列
- 统计：
  - `totalRows`
  - `startTimeNs`
  - `endTimeNs`
  - `avgIntervalNs`
- 检测：
  - `isMonotonic`
  - `isUniform`
  - `maxDeviationPercent`
- 对异常文件建立 `indices`

这是所有后续策略的基础。

------

## 建议 2：把 `mergeStreaming()` 改成“未实现即失败”

在真正实现前，不要伪成功。

否则你后面做 CLI 接入测试时，很容易出现“流程看起来跑通了，但其实没输出有效结果”的假阳性。

------

## 建议 3：先修取消语义

我建议你在下一个 commit 里优先修这个问题：

- 扫描取消 → `cancelled = true`
- 合并取消 → `cancelled = true`
- 取消不发 `errorOccurred`
- 只发状态消息，例如“任务已取消”

这个改动不大，但收益很大。

------

## 建议 4：给 `MergeResult` 增加更明确的状态语义

当前 `success + cancelled` 虽然能用，但长远看不够稳。

可以考虑增加：

```
enum class ResultCode {
    Ok,
    Cancelled,
    InvalidInput,
    ScanFailed,
    MergeFailed,
    WriteFailed
};
```

这样后续 CLI/GUI 可以更容易分流展示。

------

## 建议 5：尽快建立最小单元测试

我建议你现在就建两个测试文件：

- `test_stream_engine_boundary.cpp`
- `test_stream_engine_scan.cpp`

第一批先测这些：

### 边界测试

- 输入文件少于 2 个
- 输出路径为空
- 输入文件不存在
- 调用 cancel 后 isCancelled 为 true

### 当前骨架测试

- `merge()` 在扫描未实现时的行为
- `classifyFile()` 三类分类是否正确
- `delimiterString()` 返回值是否正确

这样你后面往里填逻辑时不会乱。

------

# 六、我的最终评价

我的评价是：

**这次实现是合格的，而且方向是对的。**
它已经完成了我们说的“先把 StreamMergeEngine 边界定义好”这个目标。
但它还只是一个**高质量骨架**，不是一个**已可交付的流式合并引擎**。

一句话总结就是：

> **接口设计通过，阶段目标 1 完成；功能目标还未完成，下一步应立即进入 scanSingleFile() 的真实实现，并先修取消语义。**





那就直接进入下一步：**把 `scanSingleFile()` 做成真正可用的扫描器**。
这是你当前骨架之后最关键的一步，因为后续的：

- 文件分类
- 顺序推进主路径
- 回退机制
- 性能统计

都依赖扫描结果是否准确。这个优先级也和你前面的实施规划一致：先完成流式内核，再推进 GUI。 performance-optimization-design

------

# 一、`scanSingleFile()` 的目标

这个函数不是“简单读文件”，而是要完成**文件建模**。

它至少要产出这几类信息：

1. **基本信息**
   - 文件路径
   - 表头
   - 总行数
2. **时间特征**
   - 起始时间
   - 结束时间
   - 是否单增
   - 是否均匀
   - 平均间隔
   - 最大偏差百分比
3. **策略信息**
   - 该文件应该走哪种路径：
     - 顺序均匀主路径
     - 顺序修正路径
     - 索引回退路径
4. **必要的回退数据**
   - 对异常文件建立轻量 `TimeIndex`

也就是说，它的本质是：

> **把一个原始文本文件，转换成一个可用于后续合并决策的 `FileMetadata`。**

------

# 二、这个函数应该怎么定义职责

## 它应该负责

- 打开文件
- 检测编码/分隔符
- 解析表头
- 逐行读取时间列
- 统计时间特征
- 必要时构建索引
- 填充 `FileMetadata`

## 它不应该负责

- 解析整行所有数据列
- 做插值
- 写输出文件
- 决定整体合并逻辑

所以它应该是一个**轻量扫描器**，而不是“半个合并器”。

------

# 三、实现原理

## 3.1 核心思路：一次扫描，收集必要信息

扫描过程应尽量只做一遍，避免重复读盘。

### 扫描时做的事

每读取一行数据：

1. 取当前文件偏移 `offset`
2. 解析时间列
3. 把时间转成 `timestampNs`
4. 更新：
   - `totalRows`
   - `startTimeNs`
   - `endTimeNs`
   - `isMonotonic`
   - interval 统计
5. 如果发现该文件可能需要回退：
   - 把 `{timestampNs, offset}` 放进 `indices`

这样扫描结束时，就已经具备后续决策所需的大部分信息。

------

## 3.2 为什么不要一开始就解析整行数值列

因为扫描阶段的目标不是“还原全部数据”，而是“判断这个文件怎么处理”。

扫描阶段最贵的操作应该避免：

- 全列 split
- 全列转 double
- 构建完整二维数据矩阵

你现在最需要的是**时间维度信息**，而不是数据值本身。

------

## 3.3 为什么建议统一用 `qint64 timestampNs`

虽然你现有系统有 `TimePoint`，但扫描阶段建议内部统一存成纳秒整数。

原因：

- 比较更快
- 计算区间更方便
- 更适合做 monotonic / interval 检测
- 减少对象临时构造

对外仍然可以保留 `TimePoint` 语义；但扫描器内部建议走整数时间戳。

------

# 四、推荐实现策略

------

## 4.1 扫描结果要分两段生成

### 第一段：实时统计

边扫边统计：

- 首行时间
- 末行时间
- 行数
- 单增性
- interval 样本

### 第二段：扫描结束后分类

扫描完成后再统一做：

- 平均间隔计算
- 偏差统计
- `isUniform` 判断
- `category` 赋值

不要在读每一行时就频繁切换 category，容易把逻辑写乱。

------

## 4.2 建议引入一个局部扫描统计器

我建议你在 `scanSingleFile()` 里不要直接用一堆局部变量乱飞，而是定义一个局部辅助结构：

```
struct ScanAccumulator {
    int totalRows = 0;

    bool hasFirstTime = false;
    qint64 firstTimeNs = 0;
    qint64 lastTimeNs = 0;
    qint64 prevTimeNs = 0;

    bool isMonotonic = true;

    qint64 intervalSumNs = 0;
    qint64 intervalMinNs = std::numeric_limits<qint64>::max();
    qint64 intervalMaxNs = 0;
    int intervalCount = 0;

    int abnormalIntervalCount = 0;
};
```

这样扫描逻辑会清晰很多。

------

## 4.3 什么时候建立 `indices`

这是一个关键点。

### 方案 A：所有文件都建立索引

优点：

- 后续回退最方便
- 扫描逻辑统一

缺点：

- 占用更多内存
- 对大文件不够克制

### 方案 B：只对异常文件建立索引

优点：

- 更节省内存
- 符合主路径“顺序推进优先”

缺点：

- 若扫描后才发现异常，需要重新处理

### 我的建议

**第一版采用折中方案：扫描时都先记录索引，但只保存轻量的 `{timestampNs, offset}`。**

因为：

- 逻辑最稳
- 便于调试
- 后面如果确认顺序主路径稳定，再优化成“只为 fallback 文件建索引”

也就是说，第一版先优先正确性与可调试性，不要过早极限省内存。

------

# 五、`scanSingleFile()` 的详细实现方法

下面我给你一版**推荐实现流程**。

------

## 步骤 1：打开文件并准备解析上下文

你现在已经有现成的 `FileReader` 能力支持：

- 编码检测
- 分隔符检测
- 时间解析

建议尽量复用，而不是在扫描器里重新造一套。这样能保持 CLI 现有格式兼容性。

### 这里要做的事

- 校验文件存在
- 打开文件
- 识别编码
- 识别分隔符
- 创建读取流

------

## 步骤 2：读取表头

第一行应做：

- 解析表头
- 保存到 `outMetadata.headers`

同时做两个校验：

1. 表头不能为空
2. 时间列必须能定位到

如果你的项目约定“第一列就是时间列”，那简单很多；
如果支持自动识别“time / 时间 / timestamp”，那这里要做列定位逻辑。

### 建议

如果当前项目尚未明确时间列定位机制，第一版先假设：

- **第一列为时间列**

先把主路径做起来。

------

## 步骤 3：逐行扫描数据行

每行做下面这些事：

### 1. 记录当前 offset

用于后续索引/回退。

### 2. 取时间字段

只解析时间列，不解析其他数据列。

### 3. 时间解析

调用你现有 `TimeParser`。

如果失败：

- 视为扫描错误，直接返回 false
- 或加入 warning 并跳过

### 我的建议

第一版对时间列错误直接失败。
因为时间列是主索引，不能含糊。

### 4. 更新统计信息

- 第一行时间
- 当前行时间
- 与上一行比较是否单增
- interval 统计

### 5. 保存轻量索引

```
indices.push_back({timestampNs, offset});
```

------

## 步骤 4：扫描结束后做均匀性分析

不要在循环里频繁判“均匀不均匀”，循环里只收集事实，结束后统一分析。

### 建议的计算方式

#### 平均间隔

```
avgIntervalNs = intervalSumNs / intervalCount;
```

#### 最大偏差百分比

对每个 interval 计算：

```
deviationPercent = abs(interval - avgInterval) * 100 / avgInterval;
```

然后取最大值。

### 第一版建议阈值

- `<= 1%`：视为均匀
- `<= 10%`：视为可修正
- `> 10%`：建议回退

这个和你前面文件里“均匀/单增驱动路径选择”的思路一致，但表达得更工程化一些。performance-optimization-design

------

## 步骤 5：填充 `FileMetadata`

最后统一写入：

- `path`
- `headers`
- `totalRows`
- `startTimeNs`
- `endTimeNs`
- `avgIntervalNs`
- `isMonotonic`
- `isUniform`
- `maxDeviationPercent`
- `indices`

然后调用 `classifyFile(outMetadata)`。

------

# 六、推荐代码草案

下面给你一版**可直接作为第一版实现起点**的代码草案。
我会尽量保持和你现在的 `StreamMergeEngine` 风格一致。

------

## 头文件辅助声明建议

如果你不想把局部统计结构暴露到头文件，可以放在 `cpp` 内匿名命名空间里。

```
namespace {
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
}
```

------

## `scanSingleFile()` 第一版代码草案

```
bool StreamMergeEngine::scanSingleFile(const QString& path,
                                       FileMetadata& outMetadata,
                                       QStringList& warnings,
                                       QString& error)
{
    outMetadata = FileMetadata{};
    outMetadata.path = path;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("无法打开输入文件: %1").arg(path);
        return false;
    }

    QTextStream in(&file);

    // TODO:
    // 后续替换为你项目中已有的编码/分隔符检测逻辑
    // 当前先假设文件已能被 QTextStream 正确读取，且第一列为时间列

    if (in.atEnd()) {
        error = QStringLiteral("文件为空: %1").arg(path);
        return false;
    }

    // 读取表头
    QString headerLine = in.readLine();
    if (headerLine.trimmed().isEmpty()) {
        error = QStringLiteral("表头为空: %1").arg(path);
        return false;
    }

    // TODO:
    // 后续应复用 DelimiterDetector
    // 当前先用制表符，必要时再扩展
    const QChar delimiter = '\t';
    outMetadata.headers = headerLine.split(delimiter);

    if (outMetadata.headers.isEmpty()) {
        error = QStringLiteral("无法解析表头: %1").arg(path);
        return false;
    }

    ScanAccumulator acc;
    outMetadata.indices.clear();

    while (!in.atEnd()) {
        if (shouldCancel()) {
            error.clear();
            return false;
        }

        const qint64 offset = file.pos();
        QString line = in.readLine();

        if (line.trimmed().isEmpty()) {
            continue;
        }

        const QStringList parts = line.split(delimiter);
        if (parts.isEmpty()) {
            warnings.append(QStringLiteral("检测到空数据行，已跳过: %1").arg(path));
            continue;
        }

        const QString timeText = parts.at(0).trimmed();
        if (timeText.isEmpty()) {
            error = QStringLiteral("时间列为空: %1").arg(path);
            return false;
        }

        // TODO:
        // 用你的 TimeParser 替换这里
        // 假设 parseTimeToNs() 返回 {success, timestampNs}
        auto parseResult = parseTimeToNs(timeText);
        if (!parseResult.success) {
            error = QStringLiteral("时间解析失败: 文件=%1, 时间值=%2")
                        .arg(path, timeText);
            return false;
        }

        const qint64 currentNs = parseResult.timestampNs;

        outMetadata.indices.push_back(TimeIndex{currentNs, offset});

        if (!acc.hasFirstTime) {
            acc.hasFirstTime = true;
            acc.firstTimeNs = currentNs;
            acc.prevTimeNs = currentNs;
            acc.lastTimeNs = currentNs;
            acc.totalRows = 1;
            continue;
        }

        if (currentNs <= acc.prevTimeNs) {
            acc.isMonotonic = false;
        }

        const qint64 intervalNs = currentNs - acc.prevTimeNs;
        if (intervalNs > 0) {
            acc.intervalSumNs += intervalNs;
            ++acc.intervalCount;

            // 第一版先全量记录 interval，后续再改采样
            acc.sampledIntervals.push_back(intervalNs);
        }

        acc.prevTimeNs = currentNs;
        acc.lastTimeNs = currentNs;
        ++acc.totalRows;
    }

    if (!acc.hasFirstTime) {
        error = QStringLiteral("文件没有有效数据行: %1").arg(path);
        return false;
    }

    outMetadata.totalRows = acc.totalRows;
    outMetadata.startTimeNs = acc.firstTimeNs;
    outMetadata.endTimeNs = acc.lastTimeNs;
    outMetadata.isMonotonic = acc.isMonotonic;

    if (acc.intervalCount > 0) {
        outMetadata.avgIntervalNs = acc.intervalSumNs / acc.intervalCount;
    }

    int maxDeviationPercent = 0;
    bool isUniform = true;

    if (outMetadata.avgIntervalNs > 0) {
        for (qint64 intervalNs : acc.sampledIntervals) {
            const qint64 diff = qAbs(intervalNs - outMetadata.avgIntervalNs);
            const int deviationPercent =
                static_cast<int>((diff * 100) / outMetadata.avgIntervalNs);

            maxDeviationPercent = qMax(maxDeviationPercent, deviationPercent);

            if (deviationPercent > m_config.uniformTolerancePercent) {
                isUniform = false;
            }
        }
    } else {
        isUniform = false;
    }

    outMetadata.maxDeviationPercent = maxDeviationPercent;
    outMetadata.isUniform = isUniform;

    classifyFile(outMetadata);

    if (m_config.verbose) {
        warnings.append(QStringLiteral(
            "扫描完成: %1, rows=%2, monotonic=%3, uniform=%4, avgIntervalNs=%5, category=%6")
            .arg(path)
            .arg(outMetadata.totalRows)
            .arg(outMetadata.isMonotonic ? "true" : "false")
            .arg(outMetadata.isUniform ? "true" : "false")
            .arg(outMetadata.avgIntervalNs)
            .arg(static_cast<int>(outMetadata.category)));
    }

    return true;
}
```

------

# 七、这版实现的注意事项

## 1. `file.pos()` 的时机要小心

你这里如果要记录“当前行起始 offset”，必须保证 offset 是**读这一行之前的位置**。

所以更稳妥的写法通常是：

```
const qint64 offset = file.pos();
QString line = in.readLine();
```

但要注意 `QTextStream` 自身缓冲可能让 `file.pos()` 和文本流行为不完全一致。
这点是 Qt 文本流实现里最容易踩坑的地方。

### 建议

如果你后续回退强依赖精确 offset，最好考虑：

- 扫描阶段直接用 `QFile::readLine()` 而不是 `QTextStream::readLine()`
- 或单独验证 `QTextStream + pos()` 是否满足你的文件场景

**这是实现里最值得你警惕的一个细节。**

------

## 2. 第一版可以接受“全量 interval 记录”，但后面要优化

当前代码里我示例用了：

```
QVector<qint64> sampledIntervals;
```

第一版是为了实现简单。
但大文件下你最终应改成：

- 前 N 行全量
- 后续按 stride 采样

否则扫描本身也会吃掉不必要内存。

------

## 3. `delimiter` 不要长期写死

我在草案里先写死为 `'\t'`，只是为了让扫描逻辑结构清晰。
正式实现时应该复用你现有的自动分隔符探测能力。你项目总结中已经明确这属于现有能力的一部分。

------

## 4. 时间解析失败建议第一版直接失败

这是一个很重要的工程选择。

因为时间列是主索引；
如果时间列都不可信，后面的顺序推进和回退都失去意义。

所以第一版建议：

- 时间列错误 => 直接失败
- 其他非关键问题 => warning

------

# 八、测试验证怎么做

这里我给你一套**最小但高价值**的测试集合。

------

## 8.1 单元测试：扫描正确性

### 用例 1：严格单增、严格均匀

输入：

- 00:00:00.000
- 00:00:01.000
- 00:00:02.000

断言：

- `isMonotonic == true`
- `isUniform == true`
- `avgIntervalNs == 1s`
- `category == SequentialUniform`

------

### 用例 2：单增、轻微抖动

输入：

- 0
- 1000
- 1999
- 3001

断言：

- `isMonotonic == true`
- `isUniform` 依据阈值判断
- `category` 不是 fallback

------

### 用例 3：局部倒序

输入：

- 0
- 1000
- 900
- 2000

断言：

- `isMonotonic == false`
- `category == IndexedFallback`

------

### 用例 4：重复时间

输入：

- 0
- 1000
- 1000
- 2000

断言：

- 第一版如果你定义“严格单增”，则：
  - `isMonotonic == false`
  - fallback

------

### 用例 5：空文件 / 只有表头

断言：

- 返回 false
- errorMessage 正确

------

## 8.2 集成测试：扫描 + 分类

准备几类真实 txt/csv 样本：

- UTF-8
- GBK
- tab / comma / semicolon

验证：

- headers 正确
- rows 正确
- category 正确

------

## 8.3 性能测试：扫描阶段基准

目标不是现在就极限快，而是先保证：

- 大文件能扫完
- 内存不过高
- 分类正确

记录：

- 文件大小
- 行数
- 扫描耗时
- `indices.size()`

------

# 九、我给你的实施建议

你现在最适合这样推进：

## 第一步

先把 `scanSingleFile()` 按上面思路实现到“可用”。

## 第二步

修复取消语义：

- 取消不视为 error
- 让 `MergeResult.cancelled = true`

## 第三步

补 5 个扫描单测

## 第四步

再开始写 `mergeStreaming()`：

- 基准文件时间轴
- 从文件滑动窗口
- nearest
- linear
- fallback

------

# 十、我的最终建议

一句话概括：

> **你现在不要急着写合并主循环，先把 `scanSingleFile()` 做扎实，因为它决定了后面所有路径选择是否正确。**

你这一步最重要的不是“把文件读出来”，而是：

- 正确识别文件特性
- 正确分类
- 正确为后续策略提供元数据

这一步做稳了，后面的 `mergeStreaming()` 会顺很多。