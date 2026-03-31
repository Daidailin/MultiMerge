#include "DataFileMerger.h"
#include <QFile>
#include <QTextStream>
#include "../../utils/DelimiterDetector.h"
#include "../../core/time/TimeParser.h"

using namespace std;

bool DataFileMerger::mergeFiles(const QVector<QString>& inputFiles, const QString& outputFile, 
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

    // 合并元数据
    FileMetadata mergedMetadata = mergeMetadata(metadatas, interpolationType);

    // 写入输出文件
    QFile outFile(outputFile);
    if (!outFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    QTextStream out(&outFile);
    QChar delimiter = delimiters[0]; // 使用第一个文件的分隔符

    // 写入表头
    out << mergedMetadata.headers.join(delimiter) << "\n";

    // 写入数据
    for (const QStringList& row : mergedMetadata.data) {
        out << row.join(delimiter) << "\n";
    }

    outFile.close();
    return true;
}

FileMetadata DataFileMerger::mergeMetadata(const QVector<FileMetadata>& metadatas, 
                                          Interpolator::InterpolationType interpolationType) {
    FileMetadata result;

    // 构建输出表头
    result.headers.append("Time");
    for (const FileMetadata& metadata : metadatas) {
        for (int i = 1; i < metadata.headers.size(); ++i) {
            result.headers.append(metadata.headers[i]);
        }
    }

    // 提取每个文件的时间值映射
    QVector<QMap<long long, QVector<double>>> timeValueMaps;
    for (const FileMetadata& metadata : metadatas) {
        timeValueMaps.append(extractTimeValueMap(metadata));
    }

    // 合并时间值映射
    QMap<long long, QVector<double>> mergedMap = mergeTimeValueMaps(timeValueMaps);

    // 为缺失的时间点插值
    interpolateMissingValues(mergedMap, timeValueMaps, interpolationType);

    // 转换为结果数据
    for (auto it = mergedMap.constBegin(); it != mergedMap.constEnd(); ++it) {
        QStringList row;
        TimePoint timePoint = TimePoint::fromMilliseconds(it.key());
        row.append(timePoint.toString());
        
        for (double value : it.value()) {
            row.append(Interpolator::doubleToString(value));
        }
        
        result.data.append(row);
    }

    return result;
}

QMap<long long, QVector<double>> DataFileMerger::extractTimeValueMap(const FileMetadata& metadata) {
    QMap<long long, QVector<double>> map;

    for (const QStringList& row : metadata.data) {
        if (row.isEmpty()) continue;

        // 解析时间
        TimePoint timePoint = TimeParser::parse(row[0]);
        long long timeMs = timePoint.toMilliseconds();

        // 解析值
        QVector<double> values;
        for (int i = 1; i < row.size(); ++i) {
            values.append(Interpolator::stringToDouble(row[i]));
        }

        map[timeMs] = values;
    }

    return map;
}

QMap<long long, QVector<double>> DataFileMerger::mergeTimeValueMaps(const QVector<QMap<long long, QVector<double>>>& maps) {
    QMap<long long, QVector<double>> mergedMap;

    // 收集所有时间点
    QSet<long long> allTimes;
    for (const QMap<long long, QVector<double>>& map : maps) {
        for (long long time : map.keys()) {
            allTimes.insert(time);
        }
    }

    // 计算总列数
    int totalColumns = 0;
    for (const QMap<long long, QVector<double>>& map : maps) {
        if (!map.isEmpty()) {
            totalColumns += map.values().first().size();
        }
    }

    // 初始化合并后的映射
    for (long long time : allTimes) {
        mergedMap[time] = QVector<double>(totalColumns, 0.0);
    }

    return mergedMap;
}

void DataFileMerger::interpolateMissingValues(QMap<long long, QVector<double>>& mergedMap, 
                                             const QVector<QMap<long long, QVector<double>>>& originalMaps, 
                                             Interpolator::InterpolationType interpolationType) {
    int columnOffset = 0;

    for (const QMap<long long, QVector<double>>& map : originalMaps) {
        if (map.isEmpty()) continue;

        int numColumns = map.values().first().size();

        // 提取时间点和对应的值
        QVector<double> times;
        QVector<QVector<double>> valuesList(numColumns);

        for (auto it = map.constBegin(); it != map.constEnd(); ++it) {
            times.append(it.key());
            for (int i = 0; i < numColumns; ++i) {
                valuesList[i].append(it.value()[i]);
            }
        }

        // 为每个时间点插值
        for (auto it = mergedMap.begin(); it != mergedMap.end(); ++it) {
            long long time = it.key();
            QVector<double>& values = it.value();

            // 检查是否有原始值
            if (map.contains(time)) {
                const QVector<double>& originalValues = map[time];
                for (int i = 0; i < numColumns; ++i) {
                    values[columnOffset + i] = originalValues[i];
                }
            } else {
                // 插值
                for (int i = 0; i < numColumns; ++i) {
                    values[columnOffset + i] = Interpolator::interpolate(times, valuesList[i], time, interpolationType);
                }
            }
        }

        columnOffset += numColumns;
    }
}