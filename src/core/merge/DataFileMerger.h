#ifndef DATAFILEMERGER_H
#define DATAFILEMERGER_H

#include <QString>
#include <QVector>
#include <QMap>
#include "../../io/FileReader.h"
#include "../interpolate/Interpolator.h"

class DataFileMerger {
public:
    // 合并多个数据文件
    static bool mergeFiles(const QVector<QString>& inputFiles, const QString& outputFile, 
                          Interpolator::InterpolationType interpolationType = Interpolator::NEAREST_NEIGHBOR);
    
    // 合并多个文件的元数据
    static FileMetadata mergeMetadata(const QVector<FileMetadata>& metadatas, 
                                     Interpolator::InterpolationType interpolationType);
    
    // 从元数据中提取时间点和对应的值
    static QMap<long long, QVector<double>> extractTimeValueMap(const FileMetadata& metadata);
    
    // 合并多个时间值映射
    static QMap<long long, QVector<double>> mergeTimeValueMaps(const QVector<QMap<long long, QVector<double>>>& maps);
    
    // 为缺失的时间点插值
    static void interpolateMissingValues(QMap<long long, QVector<double>>& mergedMap, 
                                        const QVector<QMap<long long, QVector<double>>>& originalMaps, 
                                        Interpolator::InterpolationType interpolationType);
};

#endif // DATAFILEMERGER_H