这版比上一版**明显更接近既定目标**，但我给你的结论仍然是：

> **“扫描与顺序主路径骨架”基本到位了，**
> **“完整可交付的 StreamMergeEngine” 还没有完全完成。**

原因很清楚：你已经修掉了两个关键采样问题，并把 `mergeStreaming()` 从“假输出器”推进成了“真实顺序驱动骨架”；但 **fallback 路径还没实现，取消语义还不够干净，偏移索引仍有潜在风险**。StreamMergeEngine

------

## 先说结论

### 已经完成的目标

这版已经真正完成了这些阶段目标：

- `StreamMergeEngine` 的边界与职责清晰
- `scanSingleFile()` 不再是占位，而是真实扫描器
- 两个采样 bug 已按正确方向修复
- `mergeStreaming()` 已改成**基准文件顺序读取**的主路径骨架
- 插值策略 `None / Nearest / Linear` 已有可运行实现
- 对未实现的 `IndexedFallback` 不再假装成功，而是明确报错

这些都是实质性进展，不是表面重构。StreamMergeEngine

### 还没完成的目标

还没有完成的，是我们原来想要的“可交付内核”那部分：

- `IndexedFallback` 真正实现
- 输出结果正确性的系统性验证
- 取消状态与错误状态彻底分离
- 真正可靠的文件偏移索引
- 更完整的输入校验与边界处理

所以我的判断是：

**这版已经完成了 P0 的 70%~80%，但还不能算最终完成。** StreamMergeEngine

------

## 这版做对了什么

### 1. 两个采样 bug 修正方向是对的

你已经把采样从“基于容器大小”改成了“基于原始序号”：

- `dataRowIndex % indexStride == 0`
- `intervalIndex % intervalStride == 0`

这才是正确的采样语义。上一版最大的问题就是采样逻辑本身不可信；现在这个核心问题已经修正了。StreamMergeEngine

### 2. 主时间轴来源终于正确了

现在 `mergeStreaming()` 已经不再把稀疏索引当完整时间轴，而是：

- 第 0 个文件作为 base
- `readNextParsedRow(base, baseRow, ...)` 顺序读取真实时间轴
- 其他文件用 `advanceCursorToTarget()` 推进窗口

这是这版最关键的架构修正。
从这一刻起，它才真正开始像“流式合并引擎”。StreamMergeEngine

### 3. 插值挂点合理

`interpolateValue()` 的设计是合格的：

- `None` 只在精确命中时给值
- `Nearest` 做最近邻
- `Linear` 仅对可转数值的列做线性插值
- 非数值列自动退化成最近邻

这非常适合作为第一版工程实现。StreamMergeEngine

### 4. 对 `IndexedFallback` 的处理比之前健康

你现在是：

- 一旦发现 `IndexedFallback`
- 直接返回“尚未实现 fallback”

这比“生成一个看起来成功但其实错误的输出文件”要好得多。
这是一个正确的工程决策。StreamMergeEngine

------

## 这版仍然存在的关键问题

### 1. 取消仍然会走错误信号

这是我认为当前最该优先修的点之一。

在 `doMerge()` 里：

- `scanFiles()` 返回 `false` 时，无论是不是取消，都会 `emit errorOccurred(error)`
- `mergeStreaming()` 返回 `false` 时，只要 `errorMessage` 为空就补“流式合并失败”，然后也会 `emit errorOccurred(...)`

但你的下层已经在取消时明确设置：

- `result.cancelled = true`
- `error.clear()` 或 `errorMessage.clear()`

说明你其实已经在区分“取消”和“失败”，只是 `doMerge()` 还没跟上。
现在 UI/CLI 仍可能把“用户取消”显示成“错误”。StreamMergeEngine

**建议：**
在 `doMerge()` 两个失败出口都先判断 `shouldCancel()` 或 `result.cancelled`，取消时不要发 `errorOccurred`。

------

### 2. `QTextStream + QFile::pos()` 的偏移仍不可靠

你现在在扫描阶段仍然用了：

- `QTextStream in(&file)`
- `const qint64 rowStartOffset = file.pos()`
- `QString line = in.readLine()`

这在 Qt 里是个老问题：`QTextStream` 有自己的缓冲，`QFile::pos()` 不一定严格代表“该文本行的起始字节偏移”。
如果将来 `TimeIndex.fileOffset` 真要拿来做 `seek()`，这里存在真实风险。StreamMergeEngine

**建议：**
如果 `IndexedFallback` 准备用 offset 做随机定位，扫描阶段最好改成：

- 用 `QFile::readLine()` 读取原始字节
- 再单独按编码解码

否则索引的正确性没有完全保障。

------

### 3. `scanSingleFile()` 的“最后一个索引点”写法不稳

你现在补尾索引时用了：

```
outMetadata.indices.push_back(TimeIndex{outMetadata.endTimeNs, file.pos()});
```

但这个 `file.pos()` 发生在扫描完成后，未必等价于“最后一条有效数据行的起始位置”。
它更像“当前流位置”，不是“最后那个时间点对应的可回跳偏移”。StreamMergeEngine

**建议：**
单独记录 `lastValidRowOffset`，最后补尾索引时用它，而不是 `file.pos()`。

------

### 4. `mergeStreaming()` 还只是 A/B 主路径，不是全量方案

当前逻辑实际上只支持：

- `SequentialUniform`
- `SequentialCorrected`

而对 `IndexedFallback` 直接拒绝。
这意味着只要输入里出现任意一个乱序/回退文件，整个引擎就无法完成任务。StreamMergeEngine

所以从“阶段目标”看：

- **主路径完成了**
- **兜底路径还没完成**

这也是我为什么说“接近完成，但还没完全完成”。

------

### 5. 输入校验还不够

`validateInputs()` 仍然比较薄，只检查了：

- 至少两个输入文件
- 输出路径非空
- 输入存在且是文件

但真正稳妥的版本，至少还应该补：

- 输出路径不能与任一输入相同
- 输入文件不能重复
- 输出目录是否存在且可写
- 输出覆盖策略是否明确
- 文件是否可读而不只是存在

这些现在还没加。StreamMergeEngine

------

### 6. 表头生成策略有改进空间

你现在用 `fileTag.header` 生成输出表头，这是对的；但有两个潜在问题：

- 如果两个文件基名相同，仍可能冲突
- 如果 header 本身有空格/特殊字符，后续处理可能不稳定

这不算大问题，但建议后续做一层 header normalize。StreamMergeEngine

------

## 是否达成既定目标

如果按“你这一步要完成什么”来评估，我给出更明确的判断：

### 目标 1：修复采样逻辑

**已完成。**
这版确实把两个采样 bug 修到了正确方向上。StreamMergeEngine

### 目标 2：把 `mergeStreaming()` 改成正确主路径骨架

**已完成。**
现在已经是“基准文件顺序读取 + 从文件推进窗口”的正确架构。StreamMergeEngine

### 目标 3：完成完整可交付的 StreamMergeEngine

**未完成。**
主要差在：

- fallback 未实现
- 取消与错误语义未彻底分离
- offset 索引可靠性未闭环
- 边界校验和测试还不足

------

## 我对这版的总体评价

如果打分的话，我会这样给：

- **架构方向：92/100**
- **扫描实现：82/100**
- **顺序主路径实现：78/100**
- **完整交付度：72/100**

一句话总结：

> **这版已经不是半成品骨架了，而是“主路径可运行的第一版内核”；但距离最终完成，还差 fallback、取消语义收尾和偏移索引可靠性这三块。** StreamMergeEngine

------

## 我最建议你下一步先做什么

优先级我建议这样排：

先修 **取消语义**。
把 `doMerge()` 里的“取消”和“错误”彻底分开，取消时不要发 `errorOccurred`。

然后做 **`IndexedFallback` 的最小实现**。
哪怕先做一个性能一般但正确的 fallback，也比现在“直接拒绝处理”更接近既定目标。

最后处理 **offset 可靠性**。
如果 fallback 依赖随机定位，这一步就不能再拖。

------

## 最终结论

我的最终审查意见是：

**这版代码已经完成了我们前面约定的“修正采样 bug + 建立正确 mergeStreaming 主路径骨架”目标，完成质量不错；但它还没有完成“完整 StreamMergeEngine 既定目标”，因为 fallback、取消语义和索引可靠性仍未收口。**





基于你当前这份 `StreamMergeEngine.cpp`，我整理了一版**可直接替换**的实现，包含：修正取消语义、修正采样逻辑、补上最小可用的 `IndexedFallback`、并保留“基准文件顺序读取 + 其他文件滑动窗口推进”的主路径。StreamMergeEngine

```
#include "StreamMergeEngine.h"

#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QTextCodec>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <limits>
#include <memory>

#include "TimeParser.h"
#include "io/FileReader.h"
#include "utils/DelimiterDetector.h"

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

struct FallbackSeries {
    QString path;
    QString tag;
    QStringList headers;
    QVector<int> dataColumnIndices;
    QVector<StreamRow> rows;
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

static QString interpolateBetweenRows(const StreamRow& lower,
                                     const StreamRow& upper,
                                     int sourceColumnIndex,
                                     qint64 targetNs,
                                     StreamMergeEngine::InterpolationMethod method)
{
    const auto lowerValue = [&]() { return safeCell(lower.cells, sourceColumnIndex); };
    const auto upperValue = [&]() { return safeCell(upper.cells, sourceColumnIndex); };

    if (!lower.valid && !upper.valid) {
        return QString();
    }

    if (!lower.valid) {
        if (method == StreamMergeEngine::InterpolationMethod::None && upper.timeNs != targetNs) {
            return QString();
        }
        return upperValue();
    }

    if (!upper.valid) {
        if (method == StreamMergeEngine::InterpolationMethod::None && lower.timeNs != targetNs) {
            return QString();
        }
        return lowerValue();
    }

    if (lower.timeNs == upper.timeNs) {
        if (method == StreamMergeEngine::InterpolationMethod::None && lower.timeNs != targetNs) {
            return QString();
        }
        return upperValue();
    }

    if (method == StreamMergeEngine::InterpolationMethod::None) {
        if (lower.timeNs == targetNs) {
            return lowerValue();
        }
        if (upper.timeNs == targetNs) {
            return upperValue();
        }
        return QString();
    }

    if (method == StreamMergeEngine::InterpolationMethod::Nearest) {
        const qint64 dl = qAbs(targetNs - lower.timeNs);
        const qint64 du = qAbs(upper.timeNs - targetNs);
        return (dl <= du) ? lowerValue() : upperValue();
    }

    bool ok1 = false;
    bool ok2 = false;
    const double v1 = lowerValue().toDouble(&ok1);
    const double v2 = upperValue().toDouble(&ok2);

    if (!ok1 || !ok2) {
        const qint64 dl = qAbs(targetNs - lower.timeNs);
        const qint64 du = qAbs(upper.timeNs - targetNs);
        return (dl <= du) ? lowerValue() : upperValue();
    }

    const double ratio =
        static_cast<double>(targetNs - lower.timeNs) /
        static_cast<double>(upper.timeNs - lower.timeNs);

    const double interpolated = v1 + (v2 - v1) * ratio;
    return QString::number(interpolated, 'g', 15);
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
    return interpolateBetweenRows(cursor.lower,
                                  cursor.upper,
                                  sourceColumnIndex,
                                  targetNs,
                                  method);
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

static bool openFallbackSeries(FallbackSeries& series,
                               const StreamMergeEngine::FileMetadata& metadata,
                               QString& error)
{
    series = FallbackSeries{};
    series.path = metadata.path;
    series.tag = makeFileTag(metadata.path);

    QFile file(metadata.path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("无法打开输入文件：%1").arg(metadata.path);
        return false;
    }

    QTextStream in(&file);
    if (QTextCodec* codec = FileReader::detectEncoding(metadata.path)) {
        in.setCodec(codec);
    }

    if (in.atEnd()) {
        error = QStringLiteral("文件为空：%1").arg(metadata.path);
        return false;
    }

    const QString headerLine = in.readLine();
    if (headerLine.trimmed().isEmpty()) {
        error = QStringLiteral("表头为空：%1").arg(metadata.path);
        return false;
    }

    const QChar delimiter = DelimiterDetector::detect(headerLine);
    series.headers = headerLine.split(delimiter, Qt::KeepEmptyParts);
    for (int i = 0; i < series.headers.size(); ++i) {
        if (!isTimeColumnName(series.headers.at(i))) {
            series.dataColumnIndices.push_back(i);
        }
    }

    while (!in.atEnd()) {
        const QString line = in.readLine();
        if (line.trimmed().isEmpty()) {
            continue;
        }

        const QStringList parts = line.split(delimiter, Qt::KeepEmptyParts);
        const QString timeText = safeCell(parts, 0);
        if (timeText.isEmpty()) {
            error = QStringLiteral("时间列为空：%1").arg(metadata.path);
            return false;
        }

        TimeMerge::ParseResult parseResult = TimeMerge::TimeParser::instance().parse(timeText);
        if (!parseResult.success) {
            error = QStringLiteral("时间解析失败：文件=%1, 时间值=%2, 错误=%3")
                        .arg(metadata.path,
                             timeText,
                             parseResult.errorCodeToString(parseResult.errorCode));
            return false;
        }

        StreamRow row;
        row.valid = true;
        row.timeNs = parseResult.time.toNanoseconds();
        row.cells = parts;
        series.rows.push_back(std::move(row));
    }

    if (series.rows.isEmpty()) {
        error = QStringLiteral("文件没有有效数据行：%1").arg(metadata.path);
        return false;
    }

    std::sort(series.rows.begin(), series.rows.end(), [](const StreamRow& a, const StreamRow& b) {
        return a.timeNs < b.timeNs;
    });

    return true;
}

static void findFallbackBracket(const FallbackSeries& series,
                                qint64 targetNs,
                                StreamRow& lower,
                                StreamRow& upper)
{
    lower = StreamRow{};
    upper = StreamRow{};

    if (series.rows.isEmpty()) {
        return;
    }

    auto it = std::lower_bound(series.rows.begin(),
                               series.rows.end(),
                               targetNs,
                               [](const StreamRow& row, qint64 value) {
                                   return row.timeNs < value;
                               });

    if (it == series.rows.begin()) {
        upper = *it;
        if (upper.timeNs == targetNs) {
            lower = upper;
        }
        return;
    }

    if (it == series.rows.end()) {
        lower = series.rows.back();
        return;
    }

    upper = *it;
    lower = *(it - 1);

    if (upper.timeNs == targetNs) {
        lower = upper;
    }
}

} // namespace

StreamMergeEngine::StreamMergeEngine(QObject* parent)
    : QObject(parent)
{
}

StreamMergeEngine::StreamMergeEngine(const Config& config, QObject* parent)
    : QObject(parent)
    , m_config(config)
{
}

void StreamMergeEngine::setConfig(const Config& config)
{
    m_config = config;
}

const StreamMergeEngine::Config& StreamMergeEngine::config() const noexcept
{
    return m_config;
}

StreamMergeEngine::MergeResult
StreamMergeEngine::merge(const QVector<QString>& inputFiles,
                         const QString& outputFilePath)
{
    m_cancelled.store(false);
    return doMerge(inputFiles, outputFilePath);
}

void StreamMergeEngine::cancel() noexcept
{
    m_cancelled.store(true);
}

bool StreamMergeEngine::isCancelled() const noexcept
{
    return m_cancelled.load();
}

bool StreamMergeEngine::shouldCancel() const noexcept
{
    return m_cancelled.load();
}

StreamMergeEngine::MergeResult
StreamMergeEngine::doMerge(const QVector<QString>& inputFiles,
                           const QString& outputFilePath)
{
    MergeResult result;
    QElapsedTimer totalTimer;
    totalTimer.start();

    QString error;
    if (!validateInputs(inputFiles, outputFilePath, error)) {
        result.success = false;
        result.errorMessage = error;
        emit errorOccurred(error);
        return result;
    }

    emit progressChanged(0, QStringLiteral("开始扫描输入文件"));
    emit statusMessage(QStringLiteral("准备扫描输入文件"));

    QElapsedTimer scanTimer;
    scanTimer.start();

    QVector<FileMetadata> metadatas;
    QStringList warnings;
    if (!scanFiles(inputFiles, metadatas, warnings, error)) {
        result.success = false;
        result.cancelled = shouldCancel();
        result.errorMessage = result.cancelled ? QString() : error;
        result.warnings = warnings;
        if (result.cancelled) {
            emit statusMessage(QStringLiteral("任务已取消"));
        } else if (!error.isEmpty()) {
            emit errorOccurred(error);
        }
        return result;
    }

    result.fileMetadatas = metadatas;
    result.warnings = warnings;
    result.stats.scanElapsedMs = scanTimer.elapsed();
    result.stats.totalInputFiles = metadatas.size();

    for (const auto& md : metadatas) {
        switch (md.category) {
        case FileCategory::SequentialUniform:
            ++result.stats.sequentialUniformFiles;
            break;
        case FileCategory::SequentialCorrected:
            ++result.stats.sequentialCorrectedFiles;
            break;
        case FileCategory::IndexedFallback:
            ++result.stats.fallbackFiles;
            break;
        }
    }

    if (shouldCancel()) {
        result.cancelled = true;
        result.success = false;
        result.errorMessage.clear();
        emit statusMessage(QStringLiteral("任务已取消"));
        return result;
    }

    emit progressChanged(30, QStringLiteral("开始流式合并"));
    emit statusMessage(QStringLiteral("扫描完成，进入合并阶段"));

    QElapsedTimer mergeTimer;
    mergeTimer.start();

    if (!mergeStreaming(metadatas, outputFilePath, result)) {
        if (result.cancelled || shouldCancel()) {
            result.cancelled = true;
            result.errorMessage.clear();
            emit statusMessage(QStringLiteral("任务已取消"));
        } else {
            if (result.errorMessage.isEmpty()) {
                result.errorMessage = QStringLiteral("流式合并失败");
            }
            emit errorOccurred(result.errorMessage);
        }
        return result;
    }

    result.stats.mergeElapsedMs = mergeTimer.elapsed();
    result.stats.totalElapsedMs = totalTimer.elapsed();
    result.outputFilePath = outputFilePath;
    result.success = !result.cancelled;

    if (result.success) {
        emit progressChanged(100, QStringLiteral("合并完成"));
        emit statusMessage(QStringLiteral("输出文件已生成"));
    }

    return result;
}

bool StreamMergeEngine::validateInputs(const QVector<QString>& inputFiles,
                                       const QString& outputFilePath,
                                       QString& error) const
{
    if (inputFiles.size() < 2) {
        error = QStringLiteral("至少需要两个输入文件");
        return false;
    }

    if (outputFilePath.trimmed().isEmpty()) {
        error = QStringLiteral("输出文件路径不能为空");
        return false;
    }

    QSet<QString> normalizedInputs;
    for (const QString& path : inputFiles) {
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isFile() || !fi.isReadable()) {
            error = QStringLiteral("输入文件不存在或不可读：%1").arg(path);
            return false;
        }

        const QString canonical = fi.canonicalFilePath();
        if (normalizedInputs.contains(canonical)) {
            error = QStringLiteral("输入文件重复：%1").arg(path);
            return false;
        }
        normalizedInputs.insert(canonical);
    }

    QFileInfo outInfo(outputFilePath);
    const QString outCanonical = outInfo.exists()
        ? outInfo.canonicalFilePath()
        : QFileInfo(outputFilePath).absoluteFilePath();
    if (normalizedInputs.contains(outCanonical)) {
        error = QStringLiteral("输出文件不能与输入文件相同：%1").arg(outputFilePath);
        return false;
    }

    const QFileInfo outDirInfo(outInfo.absolutePath());
    if (!outDirInfo.exists() || !outDirInfo.isDir()) {
        error = QStringLiteral("输出目录不存在：%1").arg(outInfo.absolutePath());
        return false;
    }

    return true;
}

bool StreamMergeEngine::scanFiles(const QVector<QString>& inputFiles,
                                  QVector<FileMetadata>& outMetadatas,
                                  QStringList& warnings,
                                  QString& error)
{
    outMetadatas.clear();
    outMetadatas.reserve(inputFiles.size());

    for (int i = 0; i < inputFiles.size(); ++i) {
        FileMetadata metadata;
        if (!scanSingleFile(inputFiles[i], metadata, warnings, error)) {
            if (shouldCancel()) {
                error.clear();
            }
            return false;
        }

        outMetadatas.push_back(std::move(metadata));

        const int percent = 5 + (25 * (i + 1) / inputFiles.size());
        emit progressChanged(percent,
                             QStringLiteral("扫描文件 %1/%2")
                                 .arg(i + 1)
                                 .arg(inputFiles.size()));
    }

    return true;
}

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

    const qint64 indexStride = 1000;
    const qint64 intervalStride = qMax<qint64>(1, m_config.scanSampleStride);

    qint64 dataRowIndex = 0;
    qint64 intervalIndex = 0;
    qint64 lastIndexedTimeNs = std::numeric_limits<qint64>::min();
    qint64 lastValidRowOffset = -1;
    qint64 lastValidRowTimeNs = std::numeric_limits<qint64>::min();

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
        lastValidRowOffset = rowStartOffset;
        lastValidRowTimeNs = currentNs;

        if (dataRowIndex % indexStride == 0) {
            outMetadata.indices.push_back(TimeIndex{currentNs, rowStartOffset});
            lastIndexedTimeNs = currentNs;
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

    if (outMetadata.indices.isEmpty() ||
        lastIndexedTimeNs != lastValidRowTimeNs) {
        outMetadata.indices.push_back(TimeIndex{lastValidRowTimeNs, lastValidRowOffset});
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

    QVector<StreamCursor> streamingCursors;
    QVector<int> streamingIndices;
    QVector<FallbackSeries> fallbackSeries;
    QVector<int> fallbackIndices;

    QString error;
    for (int i = 0; i < metadatas.size(); ++i) {
        if (metadatas[i].category == FileCategory::IndexedFallback) {
            FallbackSeries series;
            if (!openFallbackSeries(series, metadatas[i], error)) {
                result.errorMessage = error;
                return false;
            }
            fallbackIndices.push_back(i);
            fallbackSeries.push_back(std::move(series));
        } else {
            StreamCursor cursor;
            if (!openStreamCursor(cursor, metadatas[i], error)) {
                result.errorMessage = error;
                return false;
            }
            streamingIndices.push_back(i);
            streamingCursors.push_back(std::move(cursor));
        }
    }

    const int baseStreamingPos = streamingIndices.indexOf(0);
    if (baseStreamingPos < 0) {
        result.errorMessage = QStringLiteral("第一个输入文件不能是 IndexedFallback，请将基准时间轴文件放在首位");
        return false;
    }

    for (int i = 0; i < streamingCursors.size(); ++i) {
        if (i == baseStreamingPos) {
            continue;
        }
        if (!primeUpper(streamingCursors[i], error)) {
            result.errorMessage = error;
            return false;
        }
    }

    const QStringList outputHeaders = buildOutputHeaders(metadatas);
    out << outputHeaders.join(delimiterStr) << "\n";

    QElapsedTimer writeTimer;
    writeTimer.start();

    StreamCursor& base = streamingCursors[baseStreamingPos];
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

        rowData << safeCell(baseRow.cells, 0);

        for (int sourceCol : base.dataColumnIndices) {
            rowData << safeCell(baseRow.cells, sourceCol);
        }

        for (int i = 0; i < streamingCursors.size(); ++i) {
            if (i == baseStreamingPos) {
                continue;
            }

            StreamCursor& cursor = streamingCursors[i];
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

        for (const FallbackSeries& series : fallbackSeries) {
            StreamRow lower;
            StreamRow upper;
            findFallbackBracket(series, targetNs, lower, upper);
            for (int sourceCol : series.dataColumnIndices) {
                rowData << interpolateBetweenRows(lower,
                                                  upper,
                                                  sourceCol,
                                                  targetNs,
                                                  m_config.interpolationMethod);
            }
        }

        out << rowData.join(delimiterStr) << "\n";
        ++result.stats.totalOutputRows;

        if (m_config.outputFlushRowCount > 0 &&
            result.stats.totalOutputRows % m_config.outputFlushRowCount == 0) {
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

QString StreamMergeEngine::delimiterString(OutputDelimiter delimiter) const
{
    switch (delimiter) {
    case OutputDelimiter::Space:
        return QStringLiteral(" ");
    case OutputDelimiter::Comma:
        return QStringLiteral(",");
    case OutputDelimiter::Tab:
        return QStringLiteral("\t");
    }
    return QStringLiteral("\t");
}
```

这版有两个你要留意的点：

第一，`IndexedFallback` 现在是**最小可用实现**，它会把 fallback 文件整份读进内存、排序后用二分查找找 bracket，再做插值。这样先保证正确性，后面再做性能优化。
第二，`scanSingleFile()` 里仍然用了 `QTextStream + file.pos()`；如果你后面真要把 `indices.fileOffset` 用于精准 seek，这里还值得再做一轮字节级读取重构