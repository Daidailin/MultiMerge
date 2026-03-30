下面给出一版可直接落地的 StreamMergeEngine 边界与接口定义，目标是把它做成一个独立的高性能合并内核：上层 CLI/GUI 只关心“给我输入文件、配置和输出路径”，底层自己完成扫描、分类、顺序合并、必要时回退。这也符合你当前项目“CLI 已有核心能力，下一步优先做性能内核重构”的节奏。

设计目标
StreamMergeEngine 的职责应该非常清晰：

它负责：

接收输入文件与合并配置

预扫描文件并判断时间序列特性

选择主路径或回退路径

执行流式合并

输出结果文件

报告进度、状态、错误

支持取消

它不负责：

文件选择 UI

参数输入 UI

日志持久化策略

配置文件读写

具体命令行参数解析

复杂的测试数据生成

也就是说，它是一个核心业务引擎，不是 UI 控件，也不是 CLI 入口。

边界定义
输入边界
引擎只接受三类输入：

输入文件列表

输出文件路径

合并配置

输出边界
引擎只输出三类结果：

合并结果对象

输出文件

进度/状态/错误信号

依赖边界
引擎内部可以依赖这些模块：

TimePoint / TimeParser

FileReader 的编码/分隔符探测能力

Interpolator 的插值逻辑或等价内部实现

但它不应该直接依赖：

MainWindow

CLI 参数解析器

任何 GUI 控件类

推荐接口实现
下面这版代码重点是定义边界与接口，不是把全部算法写完。它的价值在于：后面你实现扫描器、窗口推进、回退机制时，不需要反复改外部调用方式。

StreamMergeEngine.h
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMetaType>

#include <atomic>
#include <cstdint>

// 根据你现有项目结构替换为真实头文件
#include "TimePoint.h"

class StreamMergeEngine : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(StreamMergeEngine)

public:
    enum class InterpolationMethod {
        None,
        Nearest,
        Linear
    };
    Q_ENUM(InterpolationMethod)

    enum class OutputDelimiter {
        Space,
        Comma,
        Tab
    };
    Q_ENUM(OutputDelimiter)
    
    enum class OutputEncoding {
        Utf8,
        Gbk
    };
    Q_ENUM(OutputEncoding)
    
    enum class FileCategory {
        SequentialUniform,   // 单增 + 近似均匀，走主路径
        SequentialCorrected, // 单增但不够理想，走顺序推进 + 修正
        IndexedFallback      // 异常文件，走索引/二分回退
    };
    Q_ENUM(FileCategory)
    
    struct Config {
        InterpolationMethod interpolationMethod = InterpolationMethod::Linear;
        OutputDelimiter outputDelimiter = OutputDelimiter::Tab;
        OutputEncoding outputEncoding = OutputEncoding::Utf8;
    
        qint64 timeToleranceUs = 0;
        bool verbose = false;
    
        // 扫描阶段参数
        int scanSampleStride = 1000;
        int uniformTolerancePercent = 1;
    
        // 输出缓冲
        int outputFlushRowCount = 10000;
    };
    
    struct TimeIndex {
        qint64 timestampNs = 0;
        qint64 fileOffset = 0;
    };
    
    struct FileMetadata {
        QString path;
        QStringList headers;
    
        int totalRows = 0;
        qint64 startTimeNs = 0;
        qint64 endTimeNs = 0;
        qint64 avgIntervalNs = 0;
    
        bool isMonotonic = true;
        bool isUniform = false;
        int maxDeviationPercent = 0;
    
        FileCategory category = FileCategory::SequentialUniform;
    
        // 只给需要回退的文件使用
        QVector<TimeIndex> indices;
    };
    
    struct MergeStats {
        qint64 scanElapsedMs = 0;
        qint64 mergeElapsedMs = 0;
        qint64 writeElapsedMs = 0;
        qint64 totalElapsedMs = 0;
    
        qint64 peakMemoryBytes = 0;
    
        int totalInputFiles = 0;
        int totalOutputRows = 0;
    
        int sequentialUniformFiles = 0;
        int sequentialCorrectedFiles = 0;
        int fallbackFiles = 0;
    };
    
    struct MergeResult {
        bool success = false;
        bool cancelled = false;
    
        QString outputFilePath;
        QString errorMessage;
        QStringList warnings;
    
        QVector<FileMetadata> fileMetadatas;
        MergeStats stats;
    };

public:
    explicit StreamMergeEngine(QObject* parent = nullptr);
    explicit StreamMergeEngine(const Config& config, QObject* parent = nullptr);

    void setConfig(const Config& config);
    const Config& config() const noexcept;
    
    MergeResult merge(const QVector<QString>& inputFiles,
                      const QString& outputFilePath);
    
    void cancel() noexcept;
    bool isCancelled() const noexcept;

signals:
    void progressChanged(int percent, const QString& stage);
    void statusMessage(const QString& message);
    void warningOccurred(const QString& warning);
    void errorOccurred(const QString& error);

private:
    // ===== 总流程 =====
    MergeResult doMerge(const QVector<QString>& inputFiles,
                        const QString& outputFilePath);

    bool validateInputs(const QVector<QString>& inputFiles,
                        const QString& outputFilePath,
                        QString& error) const;
    
    // ===== 阶段 1：扫描 =====
    bool scanFiles(const QVector<QString>& inputFiles,
                   QVector<FileMetadata>& outMetadatas,
                   QStringList& warnings,
                   QString& error);
    
    bool scanSingleFile(const QString& path,
                        FileMetadata& outMetadata,
                        QStringList& warnings,
                        QString& error);
    
    void classifyFile(FileMetadata& metadata) const;
    
    // ===== 阶段 2：合并 =====
    bool mergeStreaming(const QVector<FileMetadata>& metadatas,
                        const QString& outputFilePath,
                        MergeResult& result);
    
    // ===== 工具函数 =====
    QString delimiterString(OutputDelimiter delimiter) const;
    bool shouldCancel() const noexcept;

private:
    Config m_config;
    std::atomic<bool> m_cancelled { false };
};

Q_DECLARE_METATYPE(StreamMergeEngine::Config)
Q_DECLARE_METATYPE(StreamMergeEngine::FileMetadata)
Q_DECLARE_METATYPE(StreamMergeEngine::MergeResult)
StreamMergeEngine.cpp
下面给一版接口实现骨架。它先把流程、异常边界和状态通路搭起来，后续你再把扫描器与合并器的细节填进去。

#include "StreamMergeEngine.h"

#include <QFileInfo>
#include <QElapsedTimer>

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
        result.errorMessage = error;
        result.warnings = warnings;
        emit errorOccurred(error);
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
        result.errorMessage = QStringLiteral("任务已取消");
        return result;
    }
    
    emit progressChanged(30, QStringLiteral("开始流式合并"));
    emit statusMessage(QStringLiteral("扫描完成，进入合并阶段"));
    
    QElapsedTimer mergeTimer;
    mergeTimer.start();
    
    if (!mergeStreaming(metadatas, outputFilePath, result)) {
        if (result.errorMessage.isEmpty()) {
            result.errorMessage = QStringLiteral("流式合并失败");
        }
        emit errorOccurred(result.errorMessage);
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
    
    for (const QString& path : inputFiles) {
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isFile()) {
            error = QStringLiteral("输入文件不存在或不可读: %1").arg(path);
            return false;
        }
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
        if (shouldCancel()) {
            error = QStringLiteral("任务在扫描阶段被取消");
            return false;
        }
    
        FileMetadata metadata;
        if (!scanSingleFile(inputFiles[i], metadata, warnings, error)) {
            return false;
        }
    
        classifyFile(metadata);
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
    Q_UNUSED(warnings)

    // 这里先给出边界定义，不展开具体扫描逻辑。
    // 后续实现应完成：
    // 1. 表头解析
    // 2. 时间列提取
    // 3. monotonic / uniform 检测
    // 4. 必要时建立轻量 TimeIndex
    // 5. start/end/rows/interval 赋值
    
    outMetadata.path = path;
    
    // TODO: 真正实现扫描逻辑
    // 这里先占位，保证接口闭环
    outMetadata.totalRows = 0;
    outMetadata.isMonotonic = true;
    outMetadata.isUniform = false;
    outMetadata.category = FileCategory::SequentialCorrected;
    
    if (outMetadata.path.isEmpty()) {
        error = QStringLiteral("扫描失败：文件路径为空");
        return false;
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
    Q_UNUSED(metadatas)
    Q_UNUSED(outputFilePath)

    if (shouldCancel()) {
        result.cancelled = true;
        result.errorMessage = QStringLiteral("任务在合并阶段被取消");
        return false;
    }
    
    // TODO:
    // 1. 以第一个文件为基准时间轴
    // 2. 为每个从文件建立滑动窗口
    // 3. A/B 类文件走顺序推进
    // 4. C 类文件走索引/二分回退
    // 5. 批量写入输出文件
    
    result.stats.totalOutputRows = 0;
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
自然语言说明：为什么这样定义
1. 为什么 merge() 只收“输入文件 + 输出路径”
因为这是最稳定的业务边界。

调用方不应该知道：

你内部是顺序推进还是二分回退

有没有缓存

有没有做文件分类

调用方只需要知道：

我给你文件

我给你配置

你产出结果

这样将来从 CLI 切到 GUI，或者从 legacy engine 切到 stream engine，上层都不用改很多。

2. 为什么要有 Config
因为配置是可变的，而引擎行为必须可控。

像这些都应该是配置项，而不是写死：

插值方法

输出分隔符

输出编码

时间容差

均匀性判定阈值

输出缓冲大小

verbose 开关

这能保证：

CLI 参数能映射进来

GUI 面板也能映射进来

单元测试可以精确构造不同场景

3. 为什么要有 FileMetadata
因为扫描阶段不是“为了读文件”，而是“为了做策略选择”。

后续真正合并时，必须知道：

这个文件是不是单增

这个文件是不是均匀

应该走哪条路径

是否需要索引

行数和时间范围是多少

这些信息应该在扫描后固化下来，而不是散落在多个局部变量里。

4. 为什么 MergeResult 要带 stats 和 warnings
因为这个引擎不是一个“只返回 true/false”的黑盒。

你后面做调优、测试、GUI 展示时，一定会关心：

扫描耗时

合并耗时

总耗时

哪些文件走了回退

是否出现警告

输出了多少行

这些信息如果不从一开始就进入结果对象，后面你会不停往外打补丁。

5. 为什么信号只放 progress/status/warning/error
因为这几类是上层真正需要的。

progressChanged：更新进度条

statusMessage：显示“正在扫描/正在合并”

warningOccurred：展示非致命问题

errorOccurred：展示致命错误

这样 CLI 和 GUI 都能复用，不会和某个具体界面耦合。

实现时最重要的注意事项
1. 不要让 StreamMergeEngine 变成“大杂烩”
它是“流程调度者”，不是所有算法都写进去。

建议后续继续拆：

StreamFileScanner

SequentialMergeWorker

FallbackIndexedReader

OutputWriter

StreamMergeEngine 只负责串起它们。

2. 不要把 legacy 的细节直接复制进来
新引擎的边界应该更干净。

旧的 DataFileMerger 可以保留作兼容层，但不要让新引擎背上旧接口包袱。

3. 第一版不要把多线程放进 mergeStreaming()
接口可以留，但实现先单线程。

因为主合并循环最容易因为：

输出顺序

文件句柄

缓存状态

取消逻辑

而变复杂。先把边界定义对、逻辑跑通，再做并行。

4. FileCategory 非常关键
这是“策略分流”的核心开关。

你后面很多复杂度都能靠它收住：

SequentialUniform

SequentialCorrected

IndexedFallback

只要分类正确，后面代码就不会乱成一锅。

下一步你应该接着实现什么
在这个边界确定之后，最合理的下一步是：

实现 scanSingleFile()

表头解析

时间列解析

monotonic / uniform 检测

填充 FileMetadata

实现 mergeStreaming()

基准文件时间轴

从文件滑动窗口

A/B 类顺序推进

C 类二分回退

把 CLI 接到 StreamMergeEngine

支持 --engine stream

与旧引擎对比输出

下面给出一版可直接落地的 StreamMergeEngine 边界与接口定义，目标是把它做成一个独立的高性能合并内核：上层 CLI/GUI 只关心“给我输入文件、配置和输出路径”，底层自己完成扫描、分类、顺序合并、必要时回退。这也符合你当前项目“CLI 已有核心能力，下一步优先做性能内核重构”的节奏。

设计目标
StreamMergeEngine 的职责应该非常清晰：

它负责：

接收输入文件与合并配置

预扫描文件并判断时间序列特性

选择主路径或回退路径

执行流式合并

输出结果文件

报告进度、状态、错误

支持取消

它不负责：

文件选择 UI

参数输入 UI

日志持久化策略

配置文件读写

具体命令行参数解析

复杂的测试数据生成

也就是说，它是一个核心业务引擎，不是 UI 控件，也不是 CLI 入口。

边界定义
输入边界
引擎只接受三类输入：

输入文件列表

输出文件路径

合并配置

输出边界
引擎只输出三类结果：

合并结果对象

输出文件

进度/状态/错误信号

依赖边界
引擎内部可以依赖这些模块：

TimePoint / TimeParser

FileReader 的编码/分隔符探测能力

Interpolator 的插值逻辑或等价内部实现

但它不应该直接依赖：

MainWindow

CLI 参数解析器

任何 GUI 控件类

推荐接口实现
下面这版代码重点是定义边界与接口，不是把全部算法写完。它的价值在于：后面你实现扫描器、窗口推进、回退机制时，不需要反复改外部调用方式。

StreamMergeEngine.h
#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMetaType>

#include <atomic>
#include <cstdint>

// 根据你现有项目结构替换为真实头文件
#include "TimePoint.h"

class StreamMergeEngine : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY_MOVE(StreamMergeEngine)

public:
    enum class InterpolationMethod {
        None,
        Nearest,
        Linear
    };
    Q_ENUM(InterpolationMethod)

    enum class OutputDelimiter {
        Space,
        Comma,
        Tab
    };
    Q_ENUM(OutputDelimiter)
    
    enum class OutputEncoding {
        Utf8,
        Gbk
    };
    Q_ENUM(OutputEncoding)
    
    enum class FileCategory {
        SequentialUniform,   // 单增 + 近似均匀，走主路径
        SequentialCorrected, // 单增但不够理想，走顺序推进 + 修正
        IndexedFallback      // 异常文件，走索引/二分回退
    };
    Q_ENUM(FileCategory)
    
    struct Config {
        InterpolationMethod interpolationMethod = InterpolationMethod::Linear;
        OutputDelimiter outputDelimiter = OutputDelimiter::Tab;
        OutputEncoding outputEncoding = OutputEncoding::Utf8;
    
        qint64 timeToleranceUs = 0;
        bool verbose = false;
    
        // 扫描阶段参数
        int scanSampleStride = 1000;
        int uniformTolerancePercent = 1;
    
        // 输出缓冲
        int outputFlushRowCount = 10000;
    };
    
    struct TimeIndex {
        qint64 timestampNs = 0;
        qint64 fileOffset = 0;
    };
    
    struct FileMetadata {
        QString path;
        QStringList headers;
    
        int totalRows = 0;
        qint64 startTimeNs = 0;
        qint64 endTimeNs = 0;
        qint64 avgIntervalNs = 0;
    
        bool isMonotonic = true;
        bool isUniform = false;
        int maxDeviationPercent = 0;
    
        FileCategory category = FileCategory::SequentialUniform;
    
        // 只给需要回退的文件使用
        QVector<TimeIndex> indices;
    };
    
    struct MergeStats {
        qint64 scanElapsedMs = 0;
        qint64 mergeElapsedMs = 0;
        qint64 writeElapsedMs = 0;
        qint64 totalElapsedMs = 0;
    
        qint64 peakMemoryBytes = 0;
    
        int totalInputFiles = 0;
        int totalOutputRows = 0;
    
        int sequentialUniformFiles = 0;
        int sequentialCorrectedFiles = 0;
        int fallbackFiles = 0;
    };
    
    struct MergeResult {
        bool success = false;
        bool cancelled = false;
    
        QString outputFilePath;
        QString errorMessage;
        QStringList warnings;
    
        QVector<FileMetadata> fileMetadatas;
        MergeStats stats;
    };

public:
    explicit StreamMergeEngine(QObject* parent = nullptr);
    explicit StreamMergeEngine(const Config& config, QObject* parent = nullptr);

    void setConfig(const Config& config);
    const Config& config() const noexcept;
    
    MergeResult merge(const QVector<QString>& inputFiles,
                      const QString& outputFilePath);
    
    void cancel() noexcept;
    bool isCancelled() const noexcept;

signals:
    void progressChanged(int percent, const QString& stage);
    void statusMessage(const QString& message);
    void warningOccurred(const QString& warning);
    void errorOccurred(const QString& error);

private:
    // ===== 总流程 =====
    MergeResult doMerge(const QVector<QString>& inputFiles,
                        const QString& outputFilePath);

    bool validateInputs(const QVector<QString>& inputFiles,
                        const QString& outputFilePath,
                        QString& error) const;
    
    // ===== 阶段 1：扫描 =====
    bool scanFiles(const QVector<QString>& inputFiles,
                   QVector<FileMetadata>& outMetadatas,
                   QStringList& warnings,
                   QString& error);
    
    bool scanSingleFile(const QString& path,
                        FileMetadata& outMetadata,
                        QStringList& warnings,
                        QString& error);
    
    void classifyFile(FileMetadata& metadata) const;
    
    // ===== 阶段 2：合并 =====
    bool mergeStreaming(const QVector<FileMetadata>& metadatas,
                        const QString& outputFilePath,
                        MergeResult& result);
    
    // ===== 工具函数 =====
    QString delimiterString(OutputDelimiter delimiter) const;
    bool shouldCancel() const noexcept;

private:
    Config m_config;
    std::atomic<bool> m_cancelled { false };
};

Q_DECLARE_METATYPE(StreamMergeEngine::Config)
Q_DECLARE_METATYPE(StreamMergeEngine::FileMetadata)
Q_DECLARE_METATYPE(StreamMergeEngine::MergeResult)
StreamMergeEngine.cpp
下面给一版接口实现骨架。它先把流程、异常边界和状态通路搭起来，后续你再把扫描器与合并器的细节填进去。

#include "StreamMergeEngine.h"

#include <QFileInfo>
#include <QElapsedTimer>

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
        result.errorMessage = error;
        result.warnings = warnings;
        emit errorOccurred(error);
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
        result.errorMessage = QStringLiteral("任务已取消");
        return result;
    }
    
    emit progressChanged(30, QStringLiteral("开始流式合并"));
    emit statusMessage(QStringLiteral("扫描完成，进入合并阶段"));
    
    QElapsedTimer mergeTimer;
    mergeTimer.start();
    
    if (!mergeStreaming(metadatas, outputFilePath, result)) {
        if (result.errorMessage.isEmpty()) {
            result.errorMessage = QStringLiteral("流式合并失败");
        }
        emit errorOccurred(result.errorMessage);
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
    
    for (const QString& path : inputFiles) {
        QFileInfo fi(path);
        if (!fi.exists() || !fi.isFile()) {
            error = QStringLiteral("输入文件不存在或不可读: %1").arg(path);
            return false;
        }
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
        if (shouldCancel()) {
            error = QStringLiteral("任务在扫描阶段被取消");
            return false;
        }
    
        FileMetadata metadata;
        if (!scanSingleFile(inputFiles[i], metadata, warnings, error)) {
            return false;
        }
    
        classifyFile(metadata);
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
    Q_UNUSED(warnings)

    // 这里先给出边界定义，不展开具体扫描逻辑。
    // 后续实现应完成：
    // 1. 表头解析
    // 2. 时间列提取
    // 3. monotonic / uniform 检测
    // 4. 必要时建立轻量 TimeIndex
    // 5. start/end/rows/interval 赋值
    
    outMetadata.path = path;
    
    // TODO: 真正实现扫描逻辑
    // 这里先占位，保证接口闭环
    outMetadata.totalRows = 0;
    outMetadata.isMonotonic = true;
    outMetadata.isUniform = false;
    outMetadata.category = FileCategory::SequentialCorrected;
    
    if (outMetadata.path.isEmpty()) {
        error = QStringLiteral("扫描失败：文件路径为空");
        return false;
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
    Q_UNUSED(metadatas)
    Q_UNUSED(outputFilePath)

    if (shouldCancel()) {
        result.cancelled = true;
        result.errorMessage = QStringLiteral("任务在合并阶段被取消");
        return false;
    }
    
    // TODO:
    // 1. 以第一个文件为基准时间轴
    // 2. 为每个从文件建立滑动窗口
    // 3. A/B 类文件走顺序推进
    // 4. C 类文件走索引/二分回退
    // 5. 批量写入输出文件
    
    result.stats.totalOutputRows = 0;
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
自然语言说明：为什么这样定义
1. 为什么 merge() 只收“输入文件 + 输出路径”
因为这是最稳定的业务边界。

调用方不应该知道：

你内部是顺序推进还是二分回退

有没有缓存

有没有做文件分类

调用方只需要知道：

我给你文件

我给你配置

你产出结果

这样将来从 CLI 切到 GUI，或者从 legacy engine 切到 stream engine，上层都不用改很多。

2. 为什么要有 Config
因为配置是可变的，而引擎行为必须可控。

像这些都应该是配置项，而不是写死：

插值方法

输出分隔符

输出编码

时间容差

均匀性判定阈值

输出缓冲大小

verbose 开关

这能保证：

CLI 参数能映射进来

GUI 面板也能映射进来

单元测试可以精确构造不同场景

3. 为什么要有 FileMetadata
因为扫描阶段不是“为了读文件”，而是“为了做策略选择”。

后续真正合并时，必须知道：

这个文件是不是单增

这个文件是不是均匀

应该走哪条路径

是否需要索引

行数和时间范围是多少

这些信息应该在扫描后固化下来，而不是散落在多个局部变量里。

4. 为什么 MergeResult 要带 stats 和 warnings
因为这个引擎不是一个“只返回 true/false”的黑盒。

你后面做调优、测试、GUI 展示时，一定会关心：

扫描耗时

合并耗时

总耗时

哪些文件走了回退

是否出现警告

输出了多少行

这些信息如果不从一开始就进入结果对象，后面你会不停往外打补丁。

5. 为什么信号只放 progress/status/warning/error
因为这几类是上层真正需要的。

progressChanged：更新进度条

statusMessage：显示“正在扫描/正在合并”

warningOccurred：展示非致命问题

errorOccurred：展示致命错误

这样 CLI 和 GUI 都能复用，不会和某个具体界面耦合。

实现时最重要的注意事项
1. 不要让 StreamMergeEngine 变成“大杂烩”
它是“流程调度者”，不是所有算法都写进去。

建议后续继续拆：

StreamFileScanner

SequentialMergeWorker

FallbackIndexedReader

OutputWriter

StreamMergeEngine 只负责串起它们。

2. 不要把 legacy 的细节直接复制进来
新引擎的边界应该更干净。

旧的 DataFileMerger 可以保留作兼容层，但不要让新引擎背上旧接口包袱。

3. 第一版不要把多线程放进 mergeStreaming()
接口可以留，但实现先单线程。

因为主合并循环最容易因为：

输出顺序

文件句柄

缓存状态

取消逻辑

而变复杂。先把边界定义对、逻辑跑通，再做并行。

4. FileCategory 非常关键
这是“策略分流”的核心开关。

你后面很多复杂度都能靠它收住：

SequentialUniform

SequentialCorrected

IndexedFallback

只要分类正确，后面代码就不会乱成一锅。

下一步你应该接着实现什么
在这个边界确定之后，最合理的下一步是：

实现 scanSingleFile()

表头解析

时间列解析

monotonic / uniform 检测

填充 FileMetadata

实现 mergeStreaming()

基准文件时间轴

从文件滑动窗口

A/B 类顺序推进

C 类二分回退

把 CLI 接到 StreamMergeEngine

支持 --engine stream

与旧引擎对比输出