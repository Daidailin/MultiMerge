#ifndef STREAMMERGEENGINE_H
#define STREAMMERGEENGINE_H

#include <QString>
#include <QVector>
#include <QMap>
#include "../../io/FileReader.h"
#include "../interpolate/Interpolator.h"

class StreamMergeEngine {
public:
    // 流式合并多个数据文件
    static bool mergeFiles(const QVector<QString>& inputFiles, const QString& outputFile, 
                          Interpolator::InterpolationType interpolationType = Interpolator::NEAREST_NEIGHBOR);
    
    // 处理表头
    static QStringList processHeaders(const QVector<FileMetadata>& metadatas);
    
    // 构建列映射
    static QVector<QVector<int>> buildColumnMappings(const QVector<FileMetadata>& metadatas);
};

#endif // STREAMMERGEENGINE_H