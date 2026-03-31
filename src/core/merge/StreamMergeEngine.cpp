#include "StreamMergeEngine.h"
#include <QFile>
#include <QTextStream>
#include "../../utils/DelimiterDetector.h"
#include "../../core/time/TimeParser.h"

using namespace std;

bool StreamMergeEngine::mergeFiles(const QVector<QString>& inputFiles, const QString& outputFile, 
                                 Interpolator::InterpolationType interpolationType) {
    QVector<FileMetadata> metadatas;
    QVector<QChar> delimiters;

    // 读取所有文件的元数据
    for (const QString& file : inputFiles) {
        QChar delimiter = DelimiterDetector::detectDelimiter(file);
        FileMetadata metadata = FileReader::readFile(file, delimiter);
        if (!metadata.headers.isEmpty()) {
            metadatas.append(metadata);
            delimiters.append(delimiter);
        }
    }

    if (metadatas.isEmpty()) {
        return false;
    }

    // 处理表头
    QStringList outputHeaders = processHeaders(metadatas);
    
    // 构建列映射
    QVector<QVector<int>> columnMappings = buildColumnMappings(metadatas);

    // 写入输出文件
    QFile outFile(outputFile);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&outFile);
    QChar delimiter = delimiters[0]; // 使用第一个文件的分隔符
    QString delimiterStr(delimiter);

    // 写入表头
    out << outputHeaders.join(delimiterStr) << "\n";

    // 合并数据 - 这里使用简化的方法，实际应用中可能需要更复杂的流式处理
    // 首先收集所有时间点
    QSet<long long> allTimes;
    QVector<QMap<long long, QVector<double>>> timeValueMaps;

    for (const FileMetadata& metadata : metadatas) {
        QMap<long long, QVector<double>> map;
        for (const QStringList& row : metadata.data) {
            if (row.isEmpty()) continue;

            // 解析时间
            TimePoint timePoint = TimeParser::parse(row[0]);
            long long timeMs = timePoint.toMilliseconds();
            allTimes.insert(timeMs);

            // 解析值
            QVector<double> values;
            for (int i = 1; i < row.size(); ++i) {
                values.append(Interpolator::stringToDouble(row[i]));
            }

            map[timeMs] = values;
        }
        timeValueMaps.append(map);
    }

    // 按时间排序
    QVector<long long> sortedTimes = allTimes.values().toVector();
    sort(sortedTimes.begin(), sortedTimes.end());

    // 为每个时间点生成输出行
    for (long long timeMs : sortedTimes) {
        QStringList row;
        TimePoint timePoint = TimePoint::fromMilliseconds(timeMs);
        row.append(timePoint.toString());

        // 为每个文件获取或插值数据
        for (int fileIndex = 0; fileIndex < timeValueMaps.size(); ++fileIndex) {
            const QMap<long long, QVector<double>>& map = timeValueMaps[fileIndex];
            const QVector<int>& columnMapping = columnMappings[fileIndex];

            if (map.contains(timeMs)) {
                // 使用原始值
                const QVector<double>& values = map[timeMs];
                for (double value : values) {
                    row.append(Interpolator::doubleToString(value));
                }
            } else {
                // 插值 - 这里使用简化的最近邻插值
                // 实际应用中可能需要更复杂的插值方法
                long long nearestTime = -1;
                double minDistance = LLONG_MAX;

                for (long long t : map.keys()) {
                    double distance = abs(t - timeMs);
                    if (distance < minDistance) {
                        minDistance = distance;
                        nearestTime = t;
                    }
                }

                if (nearestTime != -1) {
                    const QVector<double>& values = map[nearestTime];
                    for (double value : values) {
                        row.append(Interpolator::doubleToString(value));
                    }
                } else {
                    // 没有数据，填充默认值
                    for (int i = 0; i < columnMapping.size(); ++i) {
                        row.append("0");
                    }
                }
            }
        }

        out << row.join(delimiterStr) << "\n";
    }

    outFile.close();
    return true;
}

QStringList StreamMergeEngine::processHeaders(const QVector<FileMetadata>& metadatas) {
    QStringList outputHeaders;
    outputHeaders.append("Time");
    QSet<QString> headerSet;
    QMap<QString, int> headerCount;

    for (const FileMetadata& metadata : metadatas) {
        for (int i = 1; i < metadata.headers.size(); ++i) {
            QString header = metadata.headers[i];
            if (headerSet.contains(header)) {
                headerCount[header]++;
                header = header + "_" + QString::number(headerCount[header]);
            } else {
                headerSet.insert(header);
                headerCount[header] = 1;
            }
            outputHeaders.append(header);
        }
    }

    return outputHeaders;
}

QVector<QVector<int>> StreamMergeEngine::buildColumnMappings(const QVector<FileMetadata>& metadatas) {
    QVector<QVector<int>> columnMappings(metadatas.size());

    for (int fileIndex = 0; fileIndex < metadatas.size(); ++fileIndex) {
        const FileMetadata& metadata = metadatas[fileIndex];
        QVector<int> columnMapping;

        for (int i = 1; i < metadata.headers.size(); ++i) {
            columnMapping.append(i - 1); // 存储原始列索引
        }
        columnMappings[fileIndex] = columnMapping;
    }

    return columnMappings;
}