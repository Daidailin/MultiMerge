﻿#ifndef STREAMMERGEENGINE_H
#define STREAMMERGEENGINE_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QMetaType>

#include <atomic>
#include <cstdint>

#include "TimePoint.h"

class StreamMergeEngine : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(StreamMergeEngine)

public:
    enum class InterpolationMethod {
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
        SequentialUniform,
        SequentialCorrected,
        IndexedFallback
    };
    Q_ENUM(FileCategory)
    
    struct Config {
        InterpolationMethod interpolationMethod = InterpolationMethod::Linear;
        OutputDelimiter outputDelimiter = OutputDelimiter::Tab;
        OutputEncoding outputEncoding = OutputEncoding::Utf8;
    
        qint64 timeToleranceUs = 0;
        bool verbose = false;
    
        int scanSampleStride = 1000;
        int uniformTolerancePercent = 1;
    
        int outputFlushRowCount = 10000;
    };
    
    struct TimeIndex {
        qint64 timestampNs = 0;
        qint64 fileOffset = 0;
        
        TimeIndex() : timestampNs(0), fileOffset(0) {}
        TimeIndex(qint64 ts, qint64 offset) : timestampNs(ts), fileOffset(offset) {}
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
    MergeResult doMerge(const QVector<QString>& inputFiles,
                        const QString& outputFilePath);

    bool validateInputs(const QVector<QString>& inputFiles,
                        const QString& outputFilePath,
                        QString& error) const;
    
    bool scanFiles(const QVector<QString>& inputFiles,
                   QVector<FileMetadata>& outMetadatas,
                   QStringList& warnings,
                   QString& error);
    
    bool scanSingleFile(const QString& path,
                        FileMetadata& outMetadata,
                        QStringList& warnings,
                        QString& error);
    
    void classifyFile(FileMetadata& metadata) const;
    
    bool mergeStreaming(const QVector<FileMetadata>& metadatas,
                        const QString& outputFilePath,
                        MergeResult& result);
    
    QString delimiterString(OutputDelimiter delimiter) const;
    bool shouldCancel() const noexcept;

private:
    Config m_config;
    std::atomic<bool> m_cancelled { false };
};

Q_DECLARE_METATYPE(StreamMergeEngine::Config)
Q_DECLARE_METATYPE(StreamMergeEngine::FileMetadata)
Q_DECLARE_METATYPE(StreamMergeEngine::MergeResult)

#endif // STREAMMERGEENGINE_H
