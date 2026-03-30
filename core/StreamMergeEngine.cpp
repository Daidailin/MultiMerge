﻿﻿﻿#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QSet>
#include <QSharedPointer>
#include <QTextCodec>
#include <QTextStream>
#include <QVector>
#include <algorithm>
#include <limits>
#include "core/StreamMergeEngine.h"
#include "TimeParser.h"
#include "io/FileReader.h"
#include "utils/DelimiterDetector.h"

namespace {

struct StreamRow {
    bool valid = false;
    qint64 timeNs = 0;
    QStringList cells;
};

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

static bool isTimeColumnName(const QString& name);
static QString makeFileTag(const QString& path);
static QString safeCell(const QStringList& cells, int index);

static QString interpolateBetweenRows(const StreamRow& lower,
                                     const StreamRow& upper,
                                     int sourceColumnIndex,
                                     qint64 targetNs,
                                     StreamMergeEngine::InterpolationMethod method,
                                     qint64 toleranceNs);

struct StreamCursor {
    QString path;
    QString tag;
    QStringList headers;
    QVector<int> dataColumnIndices;

    QChar delimiter = '\t';
    QFile file;
    QSharedPointer<QTextStream> in;

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
    // 逐个字段初始化，避免触发被删除的赋值运算符
    cursor.path = metadata.path;
    cursor.tag = makeFileTag(metadata.path);
    cursor.headers.clear();
    cursor.dataColumnIndices.clear();
    cursor.delimiter = '\t';
    cursor.lower = StreamRow{};
    cursor.upper = StreamRow{};
    cursor.eof = false;

    if (cursor.file.isOpen()) {
        cursor.file.close();
    }
    cursor.file.setFileName(metadata.path);

    cursor.in.reset();

    if (!cursor.file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        error = QStringLiteral("无法打开输入文件：%1").arg(metadata.path);
        return false;
    }

    cursor.in = QSharedPointer<QTextStream>::create(&cursor.file);

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

    cursor.delimiter = DelimiterDetector::toChar(DelimiterDetector::detect(headerLine));
    cursor.headers = headerLine.split(cursor.delimiter, QString::KeepEmptyParts);

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

        const QStringList parts = line.split(cursor.delimiter, QString::KeepEmptyParts);
        if (parts.isEmpty()) {
            continue;
        }

        const QString timeText = safeCell(parts, 0);
        if (timeText.isEmpty()) {
            error = QStringLiteral("时间列为空：%1").arg(cursor.path);
            return false;
        }

        TimeMerge::ParseResult parseResult = TimeMerge::TimeParser::instance().parse(timeText);
        if (!parseResult.isSuccess()) {
            error = QStringLiteral("时间解析失败：文件=%1, 时间值=%2, 错误=%3")
                        .arg(cursor.path,
                             timeText,
                             TimeMerge::ParseResult::errorCodeToString(parseResult.errorCode()));
            return false;
        }

        outRow.valid = true;
        outRow.timeNs = parseResult.timePoint().toNanoseconds();
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

static bool withinTolerance(qint64 distanceNs, qint64 toleranceNs)
{
    return toleranceNs < 0 || distanceNs <= toleranceNs;
}

static QString interpolateValue(const StreamCursor& cursor,
                                int sourceColumnIndex,
                                qint64 targetNs,
                                StreamMergeEngine::InterpolationMethod method,
                                qint64 toleranceNs)
{
    return interpolateBetweenRows(cursor.lower,
                                  cursor.upper,
                                  sourceColumnIndex,
                                  targetNs,
                                  method,
                                  toleranceNs);
}

static QStringList buildOutputHeaders(const QVector<StreamMergeEngine::FileMetadata>& metadatas)
{
    QStringList headers;
    headers << QStringLiteral("Time");

    for (const auto& md : metadatas) {
        for (const QString& header : md.headers) {
            if (!isTimeColumnName(header) && !header.trimmed().isEmpty()) {
                headers << header.trimmed();
            }
        }
    }

    return headers;
}

struct FallbackSeries {
    QString path;
    QString tag;
    QStringList headers;
    QVector<int> dataColumnIndices;
    QVector<StreamRow> rows;
};

static bool openFallbackSeries(FallbackSeries& series,
                               const StreamMergeEngine::FileMetadata& metadata,
                               QString& error)
{
    // 逐个字段初始化
    series.path = metadata.path;
    series.tag = makeFileTag(metadata.path);
    series.headers.clear();
    series.dataColumnIndices.clear();
    series.rows.clear();

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

    const QChar delimiter = DelimiterDetector::toChar(DelimiterDetector::detect(headerLine));
    series.headers = headerLine.split(delimiter, QString::KeepEmptyParts);
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

        const QStringList parts = line.split(delimiter, QString::KeepEmptyParts);
        const QString timeText = safeCell(parts, 0);
        if (timeText.isEmpty()) {
            error = QStringLiteral("时间列为空：%1").arg(metadata.path);
            return false;
        }

        TimeMerge::ParseResult parseResult = TimeMerge::TimeParser::instance().parse(timeText);
        if (!parseResult.isSuccess()) {
            error = QStringLiteral("时间解析失败：文件=%1, 时间值=%2, 错误=%3")
                        .arg(metadata.path,
                             timeText,
                             TimeMerge::ParseResult::errorCodeToString(parseResult.errorCode()));
            return false;
        }

        StreamRow row;
        row.valid = true;
        row.timeNs = parseResult.timePoint().toNanoseconds();
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

static QString interpolateBetweenRows(const StreamRow& lower,
                                     const StreamRow& upper,
                                     int sourceColumnIndex,
                                     qint64 targetNs,
                                     StreamMergeEngine::InterpolationMethod method,
                                     qint64 toleranceNs)
{
    const auto lowerValue = [&]() { return safeCell(lower.cells, sourceColumnIndex); };
    const auto upperValue = [&]() { return safeCell(upper.cells, sourceColumnIndex); };

    if (!lower.valid && !upper.valid) {
        return QStringLiteral("NaN");
    }

    if (!lower.valid) {
        const qint64 du = qAbs(upper.timeNs - targetNs);
        return withinTolerance(du, toleranceNs) ? upperValue() : QStringLiteral("NaN");
    }

    if (!upper.valid) {
        const qint64 dl = qAbs(targetNs - lower.timeNs);
        return withinTolerance(dl, toleranceNs) ? lowerValue() : QStringLiteral("NaN");
    }

    if (lower.timeNs == upper.timeNs) {
        const qint64 d = qAbs(targetNs - lower.timeNs);
        return withinTolerance(d, toleranceNs) ? upperValue() : QStringLiteral("NaN");
    }

    const qint64 dl = qAbs(targetNs - lower.timeNs);
    const qint64 du = qAbs(upper.timeNs - targetNs);

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

    bool ok1 = false;
    bool ok2 = false;
    const double v1 = lowerValue().toDouble(&ok1);
    const double v2 = upperValue().toDouble(&ok2);

    if (!ok1 || !ok2) {
        // 数值转换失败时，回退为 nearest，但仍需 tolerance 检查
        const qint64 nearestDist = qMin(dl, du);
        if (!withinTolerance(nearestDist, toleranceNs)) {
            return QStringLiteral("NaN");
        }
        return (dl <= du) ? lowerValue() : upperValue();
    }

    const double ratio =
        static_cast<double>(targetNs - lower.timeNs) /
        static_cast<double>(upper.timeNs - lower.timeNs);

    const double interpolated = v1 + (v2 - v1) * ratio;
    return QString::number(interpolated, 'g', 15);
}

}

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

    const QChar delimiter = DelimiterDetector::toChar(DelimiterDetector::detect(headerLine));
    outMetadata.headers = headerLine.split(delimiter, QString::KeepEmptyParts);

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

        const QStringList parts = line.split(delimiter, QString::KeepEmptyParts);
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
        if (!parseResult.isSuccess()) {
            error = QStringLiteral("时间解析失败：文件=%1, 时间值=%2, 错误=%3")
                        .arg(path,
                             timeText,
                             TimeMerge::ParseResult::errorCodeToString(parseResult.errorCode()));
            return false;
        }

        const qint64 currentNs = parseResult.timePoint().toNanoseconds();
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

    // 计算容差（纳秒）
    const qint64 toleranceNs =
        (m_config.timeToleranceUs < 0) ? -1 : (m_config.timeToleranceUs * 1000);

    QVector<QSharedPointer<StreamCursor>> streamingCursors;
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
            QSharedPointer<StreamCursor> cursor = QSharedPointer<StreamCursor>::create();
            if (!openStreamCursor(*cursor, metadatas[i], error)) {
                result.errorMessage = error;
                return false;
            }
            streamingIndices.append(i);
            streamingCursors.push_back(cursor);
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
        if (!primeUpper(*streamingCursors[i], error)) {
            result.errorMessage = error;
            return false;
        }
    }

    const QStringList outputHeaders = buildOutputHeaders(metadatas);
    out << outputHeaders.join(delimiterStr) << "\n";

    QElapsedTimer writeTimer;
    writeTimer.start();

    StreamCursor& base = *streamingCursors[baseStreamingPos];
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

            StreamCursor& cursor = *streamingCursors[i];
            if (!advanceCursorToTarget(cursor, targetNs, error)) {
                result.errorMessage = error;
                return false;
            }

            for (int sourceCol : cursor.dataColumnIndices) {
                rowData << interpolateValue(cursor,
                                            sourceCol,
                                            targetNs,
                                            m_config.interpolationMethod,
                                            toleranceNs);
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
                                                  m_config.interpolationMethod,
                                                  toleranceNs);
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
