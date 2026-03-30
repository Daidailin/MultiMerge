

**这版代码比上一版明显前进了一大步，已经不再只是“边界骨架”，而是做出了一个“可真实扫描、可分类、可产出占位输出”的第一版实现。**
但如果按我们前面约定的阶段目标来验收，它**只完成了“扫描器第一版”这一半，尚未完成“高性能流式合并内核”**。也就是说：

- **`StreamMergeEngine` 边界与接口**：完成
- **`scanSingleFile()` 真实扫描与分类**：基本完成第一版
- **`mergeStreaming()` 真正的顺序推进合并**：**未完成**
- **回退机制真正落地**：**未完成** performance-optimization-design

下面我按“达成目标、关键问题、改进建议、测试建议”来审查。

------

## 一、已经完成并且做得不错的部分

### 1. `scanSingleFile()` 已经从占位变成了真实扫描器

这一点是最重要的进步。你现在已经做了：

- 文件打开
- 编码检测
- 表头读取
- 分隔符检测
- 时间列解析
- monotonic 检测
- interval 统计
- uniform 判定
- `FileMetadata` 填充
- `classifyFile()` 分类

这说明“扫描阶段决定策略”的主思路已经真正开始落地，不再只是接口设计。这个方向是对的。StreamMergeEngine

### 2. 取消语义比之前好了一些

你在 `scanFiles()` 里已经开始区分“扫描失败”和“用户取消”，取消时会清空 `error`，这比上一版直接把取消当错误要更合理。`mergeStreaming()` 里也开始设置 `result.cancelled = true` 并清空错误信息，这说明你已经在往“取消不等于失败”的正确方向修了。StreamMergeEngine

### 3. 合并阶段已经能生成输出文件骨架

虽然现在还不是真正合并，但你已经把这些基础设施搭起来了：

- 输出文件创建
- 输出编码设置
- 输出分隔符处理
- 表头合成
- 逐行输出
- flush 节奏控制
- 合并阶段进度上报

这说明“输出管道”已经成型，后面只要把真实数据填进去即可。StreamMergeEngine

------

## 二、是否完成了既定目标

我的判断是：

### 已完成的目标

- **完成了 `StreamMergeEngine` 的边界与接口定义**
- **完成了 `scanSingleFile()` 第一版真实实现**
- **完成了 `mergeStreaming()` 的输出骨架**

### 未完成的目标

- **没有完成真正的数据合并**
- **没有完成顺序推进主路径**
- **没有完成“基于第一个文件时间轴”的真实驱动**
- **没有完成插值**
- **没有完成回退路径**
- **没有完成“结果正确”的流式输出**

所以如果按“下一步实现高性能流式内核”来验收，我会给出：

> **阶段性通过，但不能认定为已完成既定目标。**

因为现在的 `mergeStreaming()` 本质上还是一个“占位输出器”，不是一个“合并引擎”。StreamMergeEngine

------

## 三、当前代码里最重要的几个问题

下面这些是我认为必须优先修的点。

### 1. 一个很严重的索引采样 bug：当前写法几乎只会保存第一条索引

你现在的代码是：

```
if (outMetadata.indices.size() % 1000 == 0) {
    outMetadata.indices.push_back(TimeIndex{currentNs, offset});
}
```

这段逻辑有明显问题。因为 `indices.size()` 只有在 push 时才会增加：

- 一开始 size = 0，满足条件，push 一次
- 之后 size = 1
- 以后如果没有再次 push，size 永远是 1
- `1 % 1000 != 0`
- 于是后面永远不会再 push

也就是说，**绝大多数情况下你只会保存第一条索引**。这不是“稀疏索引”，这是“几乎没有索引”。这是当前最严重的 bug。StreamMergeEngine

#### 正确做法

采样条件应该基于“数据行号”或“扫描计数”，而不是 `indices.size()`。

例如用独立的 `rowIndex`：

```
if (rowIndex % 1000 == 0) {
    indices.push_back(...);
}
```

------

### 2. interval 采样逻辑也有同样的问题

你现在写的是：

```
if (acc.sampledIntervals.size() < 10000 ||
    acc.sampledIntervals.size() % m_config.scanSampleStride == 0) {
    acc.sampledIntervals.push_back(intervalNs);
}
```

这里的问题和上面一样：
`sampledIntervals.size()` 是“已经采样了多少个”，不是“已经扫描到第几个 interval”。

结果是：

- 前 10000 个 interval 会一直 push
- 超过 10000 后，是否继续 push 取决于“当前已采样数量”是否刚好整除 stride
- 这会造成**非常怪异的采样分布**
- 它不是“每隔 N 行采样一次”，而是“按已采样结果数量采样”

这会让 uniform 判定失真。StreamMergeEngine

#### 正确做法

采样判断也应该基于：

- 原始 interval 序号
- 或原始数据行号

不是基于样本容器当前 size。

------

### 3. `mergeStreaming()` 目前并没有做真实合并

这是当前和既定目标差距最大的地方。

你现在在 `mergeStreaming()` 里做的是：

- 取第一个文件的 metadata
- 拼表头
- 按 `totalRows` 写行
- 第一列写时间或 `row_n`
- 其他列全部填 `"0"`

这说明：

- 没有读取真实行数据
- 没有从文件 2..N 读取值
- 没有插值
- 没有顺序推进窗口
- 没有 fallback
- 没有基于真实基准时间轴进行对齐

所以它目前最多算是“**结构化假输出**”，不能算“合并结果”。StreamMergeEngine

------

### 4. `mergeStreaming()` 把稀疏索引当成了稠密时间轴来用

你现在这段逻辑：

```
if (row < timeAxisFile.indices.size()) {
    const TimeIndex& idx = timeAxisFile.indices[row];
    ...
} else {
    rowData << QString("row_%1").arg(row);
}
```

这说明你假设 `indices[row]` 能对应“第 row 行的时间”。
但你前面扫描保存的是“稀疏索引”，而且当前还有索引采样 bug。结果就是：

- 基准时间轴并不完整
- 后续大部分行会走到 `row_%1`
- 输出时间列会严重错误

哪怕你把索引采样 bug 修了，只要还是稀疏索引，就**不能直接按 row 下标访问并当作完整时间轴**。StreamMergeEngine

#### 这说明什么

你现在还缺一个关键设计决定：

- **基准文件是否保存完整时间轴？**
- 还是在合并阶段重新顺序读取基准文件？
- 稀疏索引只用于回退，不用于主时间轴？

我建议：

> **基准文件主时间轴不要依赖稀疏索引。**
> 第一版最稳妥的方式，是在合并阶段重新顺序读取基准文件。

------

### 5. `QTextStream + file.pos()` 的 offset 有风险

你当前扫描时用的是：

- `QTextStream in(&file);`
- `const qint64 offset = file.pos();`
- `QString line = in.readLine();`

这里有一个经典 Qt 风险：
`QTextStream` 有自己的缓冲，`QFile::pos()` 不一定总能严格表示“这一行文本的起始字节偏移”。

如果你后面真的要拿 `offset` 做随机 seek 或回退读取，这个实现是有潜在风险的。StreamMergeEngine

#### 建议

如果 offset 后面是强依赖项，建议改成：

- 扫描阶段用 `QFile::readLine()` 做原始字节读取
- 或专门验证当前文件场景下 `QTextStream + pos()` 是否可用

这是一个实现层面的高风险点，值得优先关注。

------

### 6. monotonic 判定与你们之前的“严格单增”目标不完全一致

你现在写的是：

```
if (currentNs < acc.prevTimeNs) {
    acc.isMonotonic = false;
}
```

这意味着：

- `currentNs == prevTimeNs` 会被视为“仍然 monotonic”
- 也就是**重复时间戳不会触发 fallback**

但我们之前讨论的顺序推进与插值模型，本质上更偏向“严格单增”，至少重复时间在插值中需要单独处理，否则可能出现：

- `upperTime == lowerTime`
- 插值分母为 0
- 最近邻/线性逻辑不稳定

所以这里要么你明确采用“非降序”语义，要么就应该改成严格单增。当前代码和既定主路径设计之间是有一点不一致的。

------

### 7. progress 信号发得太频繁

你现在在合并阶段对每一行都发一次：

```
emit progressChanged(percent, QStringLiteral("正在合并数据行"));
```

如果是 10 万行、50 万行、100 万行，这会给 UI 线程和信号槽系统带来明显开销，甚至让“优化后的程序”反而因为进度更新过频变慢。StreamMergeEngine

#### 建议

限制为：

- 每 1% 发一次
- 或每 1000 / 5000 行发一次

------

### 8. `classifyFile()` 被调用了两次

你在：

- `scanSingleFile()` 末尾调用了一次
- `scanFiles()` 里又调用了一次

这不算严重 bug，但属于冗余，会让职责边界有点混乱。StreamMergeEngine

#### 建议

二选一：

- 要么 `scanSingleFile()` 负责完整产出 metadata + category
- 要么 `scanFiles()` 统一做分类

我更倾向第一种，因为扫描函数最了解扫描结果。

------

## 四、我对你当前实现的评价

### 做得好的地方

- 方向正确
- 代码结构比上一版扎实很多
- `scanSingleFile()` 的实现思路基本符合我们的设计路线
- 输出管道已经搭起来了
- 你已经开始把真实项目里的 `TimeParser / FileReader / DelimiterDetector` 复用进来，这很好。

### 还没达到的地方

- 还不能叫“流式合并引擎”
- 还不能叫“完成 P0 实现”
- 还不能用于验证性能目标
- 还不能用于验证合并结果正确性

所以我会把这版代码定义为：

> **“扫描器第一版 + 输出骨架版”，不是“流式合并完成版”。**

------

## 五、我建议你接下来按这个顺序修

### 第一优先级：先修扫描阶段两个采样 bug

先修：

1. `indices.size() % 1000 == 0`
2. `sampledIntervals.size() % stride == 0`

这两个 bug 不修，扫描结果本身就不可信。StreamMergeEngine

------

### 第二优先级：明确主时间轴来源

你现在必须决定：

- 基准文件在合并阶段如何提供**完整时间轴**

我建议第一版采用：

- **重新顺序读取基准文件**
- metadata 只提供统计信息
- 稀疏索引用于 fallback，不用于驱动主合并循环

这会比“试图靠 metadata 里的稀疏索引直接驱动整个合并”更稳。

------

### 第三优先级：把 `mergeStreaming()` 改成真正的主路径

下一版至少要做到：

1. 顺序读取基准文件
2. 解析每个 target time
3. 为每个从文件维护一个滑动窗口
4. 先实现 nearest
5. 再实现 linear
6. C 类文件再走 fallback

这才是你真正要交付的内核。

------

### 第四优先级：把“占位输出”改成“未实现即失败”或“真实合并”

如果你下一版还没准备好真实合并，我反而建议：

- 不要输出全 0 文件
- 直接返回“尚未实现”

因为现在这种“生成了一个看似正常、实则错误的输出文件”的行为，对联调和测试都不友好。StreamMergeEngine

------

## 六、测试建议

你现在最该补的不是大而全测试，而是以下 6 个最关键测试：

### 1. 索引采样回归测试

验证 5000 行时：

- 索引数不是 1
- 真的是按预期 stride 采样

### 2. interval 采样正确性测试

验证：

- 样本分布和“按原始行号采样”一致

### 3. 严格单增测试

验证：

- 单增文件被分类成 `SequentialUniform` 或 `SequentialCorrected`

### 4. 重复时间测试

验证：

- 你到底是允许重复，还是直接 fallback
- 这个行为必须被固定下来

### 5. 基准时间轴测试

当前 `mergeStreaming()` 会输出 `row_123` 这种占位值，这个应该有测试暴露出来

### 6. 取消测试

验证：

- 取消不触发错误语义
- `result.cancelled == true`

------

## 七、最终结论

我的最终判断是：

> **你这版代码完成了“扫描器真实化”的目标，但没有完成“流式合并内核”的既定目标。**

更具体一点：

- **架构方向：对**
- **扫描实现：基本对**
- **合并实现：还没到位**
- **当前最紧急的问题：两个采样 bug + 基准时间轴设计错误**

如果打分的话，我会给：

- **边界与接口设计：90/100**
- **扫描阶段实现：75/100**
- **合并阶段实现：20/100**
- **整体阶段完成度：约 55/100** StreamMergeEngine

你这一步是“走在正确路上的半成品”，不是失败，但也还不能收工。

你当前最关键的两个问题是：
一，`scanSingleFile()` 的索引采样和 interval 采样都错误地基于容器 `size()`，会导致采样失真；
二，`mergeStreaming()` 现在把 `indices` 当完整时间轴使用，但 `indices` 本质上应该是稀疏索引，不该驱动主合并循环。

下面我给你两部分代码：

1. **修好两个采样 bug 的 `scanSingleFile()`**
2. **一个正确的 `mergeStreaming()` 主路径骨架**
   - 基准文件顺序读取
   - 从文件滑动窗口推进
   - 支持 `None / Nearest / Linear`
   - `IndexedFallback` 留出明确挂点，不再假装合并成功

------

## 先说改动原则

这版实现遵循三个原则：

- **采样基于“原始行号/区间号”**，不是样本容器大小
- **基准时间轴来自基准文件顺序读取**
- **fallback 没实现前，明确报未实现，不输出伪正确结果**

------

## 一、先替换匿名命名空间里的辅助结构和函数

把你 `cpp` 顶部匿名命名空间改成下面这样。
这个版本保留了你的 `ScanAccumulator`，同时增加了流式合并所需的辅助结构。

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

struct StreamRow {
    bool valid = false;
    qint64 timeNs = 0;
    QStringList cells;
};

struct StreamCursor {
    QString path;
    QString tag;
    QStringList headers;
    QVector<int> dataColumnIndices;

    QChar delimiter = '\t';
    QFile file;
    std::unique_ptr<QTextStream> in;

    StreamRow lower;
    StreamRow upper;

    bool eof = false;
};

static bool isTimeColumnName(const QString& name)
{
    const QString h = name.trimmed();
    return h.compare(QStringLiteral("Time"), Qt::CaseInsensitive) == 0
        || h.compare(QStringLiteral("Timestamp"), Qt::CaseInsensitive) == 0
        || h.compare(QStringLiteral("时间"), Qt::CaseInsensitive) == 0;
}

static QString makeFileTag(const QString& path)
{
    const QFileInfo fi(path);
    const QString base = fi.completeBaseName().trimmed();
    return base.isEmpty() ? fi.fileName() : base;
}

static QString safeCell(const QStringList& cells, int index)
{
    return (index >= 0 && index < cells.size()) ? cells.at(index).trimmed() : QString();
}

static bool openStreamCursor(StreamCursor& cursor,
                             const StreamMergeEngine::FileMetadata& metadata,
                             QString& error)
{
    cursor = StreamCursor{};
    cursor.path = metadata.path;
    cursor.tag = makeFileTag(metadata.path);
    cursor.file.setFileName(metadata.path);

    if (!cursor.file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("无法打开输入文件：%1").arg(metadata.path);
        return false;
    }

    cursor.in = std::make_unique<QTextStream>(&cursor.file);

    if (QTextCodec* codec = FileReader::detectEncoding(metadata.path)) {
        cursor.in->setCodec(codec);
    }

    if (cursor.in->atEnd()) {
        error = QStringLiteral("文件为空：%1").arg(metadata.path);
        return false;
    }

    const QString headerLine = cursor.in->readLine();
    if (headerLine.trimmed().isEmpty()) {
        error = QStringLiteral("表头为空：%1").arg(metadata.path);
        return false;
    }

    cursor.delimiter = DelimiterDetector::detect(headerLine);
    cursor.headers = headerLine.split(cursor.delimiter, Qt::KeepEmptyParts);

    if (cursor.headers.isEmpty()) {
        error = QStringLiteral("无法解析表头：%1").arg(metadata.path);
        return false;
    }

    for (int i = 0; i < cursor.headers.size(); ++i) {
        if (!isTimeColumnName(cursor.headers.at(i))) {
            cursor.dataColumnIndices.push_back(i);
        }
    }

    return true;
}

static bool readNextParsedRow(StreamCursor& cursor, StreamRow& outRow, QString& error)
{
    outRow = StreamRow{};

    while (!cursor.in->atEnd()) {
        const QString line = cursor.in->readLine();
        if (line.trimmed().isEmpty()) {
            continue;
        }

        const QStringList parts = line.split(cursor.delimiter, Qt::KeepEmptyParts);
        if (parts.isEmpty()) {
            continue;
        }

        const QString timeText = safeCell(parts, 0);
        if (timeText.isEmpty()) {
            error = QStringLiteral("时间列为空：%1").arg(cursor.path);
            return false;
        }

        TimeMerge::ParseResult parseResult = TimeMerge::TimeParser::instance().parse(timeText);
        if (!parseResult.success) {
            error = QStringLiteral("时间解析失败：文件=%1, 时间值=%2, 错误=%3")
                        .arg(cursor.path,
                             timeText,
                             parseResult.errorCodeToString(parseResult.errorCode));
            return false;
        }

        outRow.valid = true;
        outRow.timeNs = parseResult.time.toNanoseconds();
        outRow.cells = parts;
        return true;
    }

    return true;
}

static bool primeUpper(StreamCursor& cursor, QString& error)
{
    if (!readNextParsedRow(cursor, cursor.upper, error)) {
        return false;
    }

    if (!cursor.upper.valid) {
        cursor.eof = true;
    }

    return true;
}

static bool advanceCursorToTarget(StreamCursor& cursor, qint64 targetNs, QString& error)
{
    while (cursor.upper.valid && cursor.upper.timeNs < targetNs) {
        cursor.lower = cursor.upper;

        if (!readNextParsedRow(cursor, cursor.upper, error)) {
            return false;
        }

        if (!cursor.upper.valid) {
            cursor.eof = true;
            break;
        }
    }

    return true;
}

static QString interpolateValue(const StreamCursor& cursor,
                                int sourceColumnIndex,
                                qint64 targetNs,
                                StreamMergeEngine::InterpolationMethod method)
{
    const auto lowerValue = [&]() { return safeCell(cursor.lower.cells, sourceColumnIndex); };
    const auto upperValue = [&]() { return safeCell(cursor.upper.cells, sourceColumnIndex); };

    if (!cursor.lower.valid && !cursor.upper.valid) {
        return QString();
    }

    if (!cursor.lower.valid) {
        return upperValue();
    }

    if (!cursor.upper.valid) {
        return lowerValue();
    }

    if (cursor.lower.timeNs == cursor.upper.timeNs) {
        return upperValue();
    }

    if (method == StreamMergeEngine::InterpolationMethod::None) {
        if (cursor.lower.timeNs == targetNs) {
            return lowerValue();
        }
        if (cursor.upper.timeNs == targetNs) {
            return upperValue();
        }
        return QString();
    }

    if (method == StreamMergeEngine::InterpolationMethod::Nearest) {
        const qint64 dl = qAbs(targetNs - cursor.lower.timeNs);
        const qint64 du = qAbs(cursor.upper.timeNs - targetNs);
        return (dl <= du) ? lowerValue() : upperValue();
    }

    bool ok1 = false;
    bool ok2 = false;
    const double v1 = lowerValue().toDouble(&ok1);
    const double v2 = upperValue().toDouble(&ok2);

    if (!ok1 || !ok2) {
        const qint64 dl = qAbs(targetNs - cursor.lower.timeNs);
        const qint64 du = qAbs(cursor.upper.timeNs - targetNs);
        return (dl <= du) ? lowerValue() : upperValue();
    }

    const double ratio =
        static_cast<double>(targetNs - cursor.lower.timeNs) /
        static_cast<double>(cursor.upper.timeNs - cursor.lower.timeNs);

    const double interpolated = v1 + (v2 - v1) * ratio;
    return QString::number(interpolated, 'g', 15);
}

static QStringList buildOutputHeaders(const QVector<StreamMergeEngine::FileMetadata>& metadatas)
{
    QStringList headers;
    headers << QStringLiteral("Time");

    for (const auto& md : metadatas) {
        const QString tag = makeFileTag(md.path);
        for (const QString& header : md.headers) {
            if (!isTimeColumnName(header) && !header.trimmed().isEmpty()) {
                headers << QStringLiteral("%1.%2").arg(tag, header.trimmed());
            }
        }
    }

    return headers;
}

} // namespace
```

------

## 二、替换 `scanSingleFile()`，修掉两个采样 bug

下面这版是**直接可替换**的 `scanSingleFile()`。

关键修正点：

- 索引采样用 `dataRowIndex`
- interval 采样用 `intervalIndex`
- monotonic 改成**严格单增**
- 总是把最后一个点补进索引，避免尾部没索引

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
        error = QStringLiteral("无法打开输入文件：%1").arg(path);
        return false;
    }

    QTextStream in(&file);

    if (QTextCodec* codec = FileReader::detectEncoding(path)) {
        in.setCodec(codec);
    }

    if (in.atEnd()) {
        error = QStringLiteral("文件为空：%1").arg(path);
        return false;
    }

    const QString headerLine = in.readLine();
    if (headerLine.trimmed().isEmpty()) {
        error = QStringLiteral("表头为空：%1").arg(path);
        return false;
    }

    const QChar delimiter = DelimiterDetector::detect(headerLine);
    outMetadata.headers = headerLine.split(delimiter, Qt::KeepEmptyParts);

    if (outMetadata.headers.isEmpty()) {
        error = QStringLiteral("无法解析表头：%1").arg(path);
        return false;
    }

    ScanAccumulator acc;
    outMetadata.indices.clear();

    const qint64 indexStride = 1000; // 稀疏索引步长，先固定
    const qint64 intervalStride = qMax<qint64>(1, m_config.scanSampleStride);

    qint64 dataRowIndex = 0;
    qint64 intervalIndex = 0;
    qint64 lastIndexedTimeNs = std::numeric_limits<qint64>::min();
    qint64 lastIndexedOffset = -1;

    while (!in.atEnd()) {
        if (shouldCancel()) {
            error.clear();
            return false;
        }

        const qint64 rowStartOffset = file.pos();
        const QString line = in.readLine();

        if (line.trimmed().isEmpty()) {
            continue;
        }

        const QStringList parts = line.split(delimiter, Qt::KeepEmptyParts);
        if (parts.isEmpty()) {
            warnings.append(QStringLiteral("检测到空数据行，已跳过：%1").arg(path));
            continue;
        }

        const QString timeText = safeCell(parts, 0);
        if (timeText.isEmpty()) {
            error = QStringLiteral("时间列为空：%1").arg(path);
            return false;
        }

        TimeMerge::ParseResult parseResult = TimeMerge::TimeParser::instance().parse(timeText);
        if (!parseResult.success) {
            error = QStringLiteral("时间解析失败：文件=%1, 时间值=%2, 错误=%3")
                        .arg(path,
                             timeText,
                             parseResult.errorCodeToString(parseResult.errorCode));
            return false;
        }

        const qint64 currentNs = parseResult.time.toNanoseconds();

        if (dataRowIndex % indexStride == 0) {
            outMetadata.indices.push_back(TimeIndex{currentNs, rowStartOffset});
            lastIndexedTimeNs = currentNs;
            lastIndexedOffset = rowStartOffset;
        }

        if (!acc.hasFirstTime) {
            acc.hasFirstTime = true;
            acc.firstTimeNs = currentNs;
            acc.prevTimeNs = currentNs;
            acc.lastTimeNs = currentNs;
            acc.totalRows = 1;
            ++dataRowIndex;
            continue;
        }

        if (currentNs <= acc.prevTimeNs) {
            acc.isMonotonic = false;
        }

        const qint64 intervalNs = currentNs - acc.prevTimeNs;
        if (intervalNs > 0) {
            acc.intervalSumNs += intervalNs;
            ++acc.intervalCount;

            if (intervalIndex < 10000 || (intervalIndex % intervalStride == 0)) {
                acc.sampledIntervals.push_back(intervalNs);
            }

            ++intervalIndex;
        }

        acc.prevTimeNs = currentNs;
        acc.lastTimeNs = currentNs;
        ++acc.totalRows;
        ++dataRowIndex;
    }

    if (!acc.hasFirstTime) {
        error = QStringLiteral("文件没有有效数据行：%1").arg(path);
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

    if (outMetadata.avgIntervalNs > 0 && !acc.sampledIntervals.isEmpty()) {
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

    // 确保最后一个时间点也落入索引
    if (outMetadata.indices.isEmpty() ||
        lastIndexedTimeNs != outMetadata.endTimeNs ||
        lastIndexedOffset < 0) {
        outMetadata.indices.push_back(TimeIndex{outMetadata.endTimeNs, file.pos()});
    }

    classifyFile(outMetadata);

    if (m_config.verbose) {
        warnings.append(QStringLiteral(
            "扫描完成：%1, rows=%2, monotonic=%3, uniform=%4, avgIntervalNs=%5, category=%6")
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

## 三、把 `scanFiles()` 里的重复分类去掉

因为 `scanSingleFile()` 已经调用了 `classifyFile()`，这里不用再调一次。

把这段：

```
classifyFile(metadata);
outMetadatas.push_back(std::move(metadata));
```

改成：

```
outMetadatas.push_back(std::move(metadata));
```

------

## 四、替换 `mergeStreaming()`，给你一个正确主路径骨架

这版骨架已经不是假输出了。
它的行为是：

- 第 1 个文件作为基准文件
- 基准文件逐行读取，拿真实 target time
- 其他文件都维护 `lower/upper` 滑动窗口
- `SequentialUniform` 和 `SequentialCorrected` 都走顺序推进
- `IndexedFallback` 先明确报未实现，避免输出伪正确结果

```
bool StreamMergeEngine::mergeStreaming(const QVector<FileMetadata>& metadatas,
                                       const QString& outputFilePath,
                                       MergeResult& result)
{
    if (metadatas.isEmpty()) {
        result.errorMessage = QStringLiteral("没有有效的输入文件");
        return false;
    }

    if (shouldCancel()) {
        result.cancelled = true;
        result.errorMessage.clear();
        return false;
    }

    for (const auto& md : metadatas) {
        if (md.category == FileCategory::IndexedFallback) {
            result.errorMessage =
                QStringLiteral("当前版本尚未实现 IndexedFallback 路径，请先完成 fallback 查找逻辑");
            return false;
        }
    }

    QFile outFile(outputFilePath);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.errorMessage = QStringLiteral("无法创建输出文件：%1").arg(outputFilePath);
        return false;
    }

    QTextStream out(&outFile);
    if (m_config.outputEncoding == OutputEncoding::Utf8) {
        out.setCodec("UTF-8");
    } else {
        out.setCodec("GB18030");
    }

    const QString delimiterStr = delimiterString(m_config.outputDelimiter);

    // 打开所有输入游标
    QVector<StreamCursor> cursors;
    cursors.resize(metadatas.size());

    QString error;
    for (int i = 0; i < metadatas.size(); ++i) {
        if (!openStreamCursor(cursors[i], metadatas[i], error)) {
            result.errorMessage = error;
            return false;
        }
    }

    // 第 0 个为基准文件；其余文件先 prime upper
    for (int i = 1; i < cursors.size(); ++i) {
        if (!primeUpper(cursors[i], error)) {
            result.errorMessage = error;
            return false;
        }
    }

    // 输出表头：Time + fileTag.header
    const QStringList outputHeaders = buildOutputHeaders(metadatas);
    out << outputHeaders.join(delimiterStr) << "\n";

    QElapsedTimer writeTimer;
    writeTimer.start();

    StreamCursor& base = cursors[0];
    StreamRow baseRow;

    int lastPercent = -1;
    const int totalRows = qMax(1, metadatas[0].totalRows);

    while (true) {
        if (shouldCancel()) {
            result.cancelled = true;
            result.errorMessage.clear();
            return false;
        }

        if (!readNextParsedRow(base, baseRow, error)) {
            result.errorMessage = error;
            return false;
        }

        if (!baseRow.valid) {
            break;
        }

        const qint64 targetNs = baseRow.timeNs;
        QStringList rowData;
        rowData.reserve(outputHeaders.size());

        // Time 列直接用基准文件原始时间字符串
        rowData << safeCell(baseRow.cells, 0);

        // 先追加基准文件自己的非时间列
        for (int sourceCol : base.dataColumnIndices) {
            rowData << safeCell(baseRow.cells, sourceCol);
        }

        // 再追加其他文件对齐后的列
        for (int i = 1; i < cursors.size(); ++i) {
            StreamCursor& cursor = cursors[i];

            if (!advanceCursorToTarget(cursor, targetNs, error)) {
                result.errorMessage = error;
                return false;
            }

            for (int sourceCol : cursor.dataColumnIndices) {
                rowData << interpolateValue(cursor,
                                            sourceCol,
                                            targetNs,
                                            m_config.interpolationMethod);
            }
        }

        out << rowData.join(delimiterStr) << "\n";
        ++result.stats.totalOutputRows;

        if (result.stats.totalOutputRows % m_config.outputFlushRowCount == 0) {
            out.flush();
        }

        const int percent = 30 + (70 * result.stats.totalOutputRows) / totalRows;
        if (percent != lastPercent) {
            emit progressChanged(percent, QStringLiteral("正在流式合并"));
            lastPercent = percent;
        }
    }

    out.flush();
    outFile.close();

    result.stats.writeElapsedMs = writeTimer.elapsed();
    return true;
}
```

------

## 五、这版 `mergeStreaming()` 已经解决了什么

它已经解决了你当前代码里的三个根本问题。
这些问题都出现在你当前 `StreamMergeEngine.cpp` 里。

### 1. 不再拿 `indices` 当完整时间轴

时间轴现在来自：

- `base` 基准文件
- `readNextParsedRow(base, baseRow, ...)`

这才是正确主路径。

### 2. 不再输出全 0 占位数据

现在每个从文件都会根据 `targetNs`：

- 先顺序推进到窗口位置
- 再根据插值策略给出真实值

### 3. 不再对 `IndexedFallback` 假装成功

在 fallback 没实现之前，这版会明确失败，而不是生成“看起来正常但其实错误”的输出文件。

------

## 六、你还要注意两个实现细节

### 1. `QTextStream + file.pos()` 的 offset 仍然有风险

这次我保留了你现有模式，是为了不一下改太多。
但如果你后面真的要用 `indices.fileOffset` 做 seek 回退，建议你最终改成：

- 扫描阶段用 `QFile::readLine()` 做原始字节读取
- 明确把“字节偏移”和“文本解码”分开

否则 offset 精度可能出问题。你当前代码就有这个隐患。

### 2. 线性插值只对数值列有效

我这里的 `Linear` 是：

- 能转 `double` 就线性插值
- 不能转就自动退化成 `Nearest`

这比较适合作为第一版工程实现。

------

## 七、你下一步应该怎么接着做

现在你可以按这个顺序继续：

先把上面两段代码替换进去，然后做 4 个测试：

1. **索引采样测试**
   5000 行文件下，`indices.size()` 不能再接近 1
2. **interval 采样测试**
   样本应真正按原始 interval 序号采样
3. **主路径合并测试**
   两个严格单增文件，`Nearest` 能输出真实对齐结果
4. **重复时间测试**
   验证你是否接受“重复时间”还是直接 fallback

然后再补最后一步：

- `IndexedFallback` 的 seek 查找逻辑

------

## 八、我给你的最终建议

你现在最正确的策略不是继续铺新功能，而是：

> **先把 A/B 类顺序主路径彻底跑通，再补 C 类 fallback。**

因为一旦主路径稳定了，性能和正确性都会立刻有可验证结果；
而 fallback 本来就应该是少数异常文件的兜底，不应该拖住主路径收敛。

你把这版先接进去，下一条我可以继续直接帮你写：

**`IndexedFallback` 的最小可用实现骨架**。