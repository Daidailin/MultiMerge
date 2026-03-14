#ifndef DATAFILEMERGER_H
#define DATAFILEMERGER_H

#include <QObject>
#include <QString>
#include <QVector>
#include "io/FileReader.h"
#include "Interpolator.h"

struct MergeConfig {
    InterpolationMethod interpolation = InterpolationMethod::Nearest;
    QString outputDelimiter = " ";
    QString outputEncoding = "UTF-8";
    int64_t timeToleranceUs = 0;
    bool verbose = false;
    
    MergeConfig() {}
};

struct MergeResult {
    bool success;
    QString outputFile;
    int mergedRows;
    int warningCount;
    QVector<QString> warnings;
    
    MergeResult() : success(false), mergedRows(0), warningCount(0) {}
};

class DataFileMerger : public QObject {
    Q_OBJECT
    
public:
    explicit DataFileMerger(const MergeConfig& config);
    MergeResult merge(const QVector<QString>& inputFiles, const QString& outputFile);
    void cancel();
    
signals:
    void progressChanged(int percent);
    void statusMessage(const QString& message);
    void warningOccurred(const QString& warning);
    
private:
    MergeConfig m_config;
    bool m_cancelled;
    
    void writeOutput(const QString& outputFile, const DataFile& timeAxisFile,
                    const QVector<DataFile>& otherFiles, MergeResult& result);
    QVector<QString> generateOutputHeaders(const DataFile& timeAxisFile,
                                          const QVector<DataFile>& otherFiles);
};

#endif // DATAFILEMERGER_H
