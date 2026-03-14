#include "DataFileMerger.h"
#include "io/FileReader.h"
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QDebug>

DataFileMerger::DataFileMerger(const MergeConfig& config)
    : m_config(config), m_cancelled(false) {
}

MergeResult DataFileMerger::merge(const QVector<QString>& inputFiles, const QString& outputFile) {
    MergeResult result;
    m_cancelled = false;
    
    if (inputFiles.isEmpty()) {
        result.warnings << "没有输入文件";
        return result;
    }
    
    emit statusMessage("正在加载文件...");
    
    QVector<DataFile> files;
    for (int i = 0; i < inputFiles.size(); ++i) {
        if (m_cancelled) {
            result.warnings << "操作已取消";
            return result;
        }
        
        emit statusMessage(QString("正在加载文件 %1/%2: %3")
            .arg(i + 1).arg(inputFiles.size()).arg(inputFiles[i]));
        
        DataFile file = FileReader::load(inputFiles[i]);
        
        if (!file.valid) {
            result.warnings << QString("文件 %1 加载失败，已跳过").arg(inputFiles[i]);
            for (const QString& warning : file.warnings) {
                result.warnings << warning;
                emit warningOccurred(warning);
            }
            continue;
        }
        
        files.append(file);
        emit progressChanged((i + 1) * 30 / inputFiles.size());
    }
    
    if (files.isEmpty()) {
        result.warnings << "没有有效的输入文件";
        return result;
    }
    
    const DataFile& timeAxisFile = files[0];
    QVector<DataFile> otherFiles;
    for (int i = 1; i < files.size(); ++i) {
        otherFiles.append(files[i]);
    }
    
    emit statusMessage("正在合并数据...");
    writeOutput(outputFile, timeAxisFile, otherFiles, result);
    
    if (m_cancelled) {
        result.warnings << "操作已取消";
        return result;
    }
    
    result.success = true;
    result.outputFile = outputFile;
    emit statusMessage("合并完成");
    emit progressChanged(100);
    
    return result;
}

void DataFileMerger::cancel() {
    m_cancelled = true;
}

void DataFileMerger::writeOutput(const QString& outputFile, const DataFile& timeAxisFile,
                                 const QVector<DataFile>& otherFiles, MergeResult& result) {
    QFile file(outputFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.warnings << QString("无法创建输出文件：%1").arg(outputFile);
        return;
    }
    
    QTextStream out(&file);
    out.setCodec(QTextCodec::codecForName(m_config.outputEncoding.toUtf8()));
    
    QVector<QString> headers = generateOutputHeaders(timeAxisFile, otherFiles);
    out << QStringList(headers.toList()).join(m_config.outputDelimiter) << "\n";
    
    int totalRows = timeAxisFile.rowCount();
    for (int row = 0; row < totalRows; ++row) {
        if (m_cancelled) {
            file.close();
            return;
        }
        
        QVector<QString> rowData;
        rowData << timeAxisFile.timePoints[row].toString();
        
        for (int col = 0; col < timeAxisFile.paramCount(); ++col) {
            double value = timeAxisFile.data[row][col];
            rowData << (std::isnan(value) ? "NaN" : QString::number(value, 'g', 10));
        }
        
        for (int fileIdx = 0; fileIdx < otherFiles.size(); ++fileIdx) {
            const DataFile& otherFile = otherFiles[fileIdx];
            const TimeMerge::TimePoint& targetTime = timeAxisFile.timePoints[row];
            
            for (int col = 0; col < otherFile.paramCount(); ++col) {
                QString paramName = otherFile.paramNames[col];
                bool isDuplicate = timeAxisFile.paramNames.contains(paramName);
                
                if (!isDuplicate) {
                    for (int prevIdx = 0; prevIdx < fileIdx; ++prevIdx) {
                        if (otherFiles[prevIdx].paramNames.contains(paramName)) {
                            isDuplicate = true;
                            break;
                        }
                    }
                }
                
                if (isDuplicate) continue;
                
                QVector<double> columnData;
                for (int r = 0; r < otherFile.rowCount(); ++r) {
                    columnData << otherFile.data[r][col];
                }
                
                double value = Interpolator::interpolate(
                    otherFile.timePoints, columnData, targetTime, m_config.interpolation);
                
                rowData << (std::isnan(value) ? "NaN" : QString::number(value, 'g', 10));
            }
        }
        
        out << QStringList(rowData.toList()).join(m_config.outputDelimiter) << "\n";
        emit progressChanged(30 + (row + 1) * 60 / totalRows);
    }
    
    file.close();
    result.mergedRows = totalRows;
}

QVector<QString> DataFileMerger::generateOutputHeaders(const DataFile& timeAxisFile,
                                                       const QVector<DataFile>& otherFiles) {
    QVector<QString> headers;
    QSet<QString> usedNames;
    
    headers << "Time";
    usedNames.insert("Time");
    
    for (const QString& paramName : timeAxisFile.paramNames) {
        headers << paramName;
        usedNames.insert(paramName);
    }
    
    for (const DataFile& otherFile : otherFiles) {
        for (const QString& paramName : otherFile.paramNames) {
            if (usedNames.contains(paramName)) {
                qWarning() << QString("警告：列名 '%1' 已存在，跳过文件 %2 中的此列").arg(paramName).arg(otherFile.filename);
            } else {
                headers << paramName;
                usedNames.insert(paramName);
            }
        }
    }
    
    return headers;
}
