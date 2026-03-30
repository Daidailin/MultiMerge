#ifndef DATAFILEMERGER_H
#define DATAFILEMERGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include "io/FileReader.h"
#include "Interpolator.h"

struct MergeConfig {
    InterpolationMethod interpolation = InterpolationMethod::Nearest;  //< 插值方法
    QString outputDelimiter = " ";          //< 输出分隔符（空格/逗号/制表符等）
    QString outputEncoding = "UTF-8";       //< 输出文件编码
    int64_t timeToleranceUs = 0;            //< 时间容差（微秒），0 表示不限制
    bool verbose = false;                   //< 是否启用详细输出
    
    MergeConfig() {}  ///< 默认构造函数
};


struct MergeResult {
    bool success;                    //< 是否成功
    QString outputFile;              //< 输出文件路径
    int mergedRows;                  //< 合并的行数
    int warningCount;                //< 警告数量
    QVector<QString> warnings;       //< 警告信息列表
    
    MergeResult() : success(false), mergedRows(0), warningCount(0) {}  //< 默认构造函数
};

class DataFileMerger : public QObject {
    Q_OBJECT
    
public:

    explicit DataFileMerger(const MergeConfig& config);
    
    MergeResult merge(const QVector<QString>& inputFiles, 
                     const QString& outputFile);
    
    void cancel();
    
signals:

    void progressChanged(int percent);
    
    void statusMessage(const QString& message);
    
    void warningOccurred(const QString& warning);
    
private:
    MergeConfig m_config;
    bool m_cancelled;
  // 写入输出文件
    bool writeOutput(const QString& outputFile,
                    const DataFile& timeAxisFile,
                    const QVector<DataFile>& otherFiles,
                    MergeResult& result);
    
    QVector<QString> generateOutputHeaders(
        const DataFile& timeAxisFile,
        const QVector<DataFile>& otherFiles);
};

#endif // DATAFILEMERGER_H
