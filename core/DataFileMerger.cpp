﻿﻿﻿#include "DataFileMerger.h"
#include "io/FileReader.h"
#include <QFile>
#include <QTextStream>
#include <QSet>
#include <QDebug>
#include <cmath>
#pragma execution_character_set("utf-8")

DataFileMerger::DataFileMerger(const MergeConfig& config)
    : m_config(config), m_cancelled(false) {
}

MergeResult DataFileMerger::merge(const QVector<QString>& inputFiles,
                                  const QString& outputFile) {
    MergeResult result;  // 合并结果对象
    m_cancelled = false;  // 重置取消标志
    
    // 检查输入文件是否为空
    if (inputFiles.isEmpty()) {
        result.warnings << "没有输入文件";
        return result;
    }
    
    // 发送状态消息
    emit statusMessage("正在加载文件...");
    
    // ==================== 步骤 1：加载所有文件 ====================
    QVector<DataFile> files;  // 存储所有成功加载的文件
    for (int i = 0; i < inputFiles.size(); ++i) {
        // 检查是否被取消
        if (m_cancelled) {
            result.warnings << "操作已取消";
            return result;
        }
        
        // 发送加载进度消息
        emit statusMessage(QString("正在加载文件 %1/%2: %3")
            .arg(i + 1)
            .arg(inputFiles.size())
            .arg(inputFiles[i]));
        
        // 使用 FileReader 加载文件
        DataFile file = FileReader::load(inputFiles[i]);
        
        // 检查文件是否加载成功
        if (!file.valid) {
            // 加载失败，记录警告并跳过
            result.warnings << QString("文件 %1 加载失败，已跳过").arg(inputFiles[i]);
            for (const QString& warning : file.warnings) {
                result.warnings << warning;
                emit warningOccurred(warning);  // 发送警告信号
            }
            continue;  // 跳过此文件，继续加载下一个
        }
        
        // 加载成功，添加到文件列表
        files.append(file);
        
        // 更新进度（加载阶段占总进度的 30%）
        int progress = (i + 1) * 30 / inputFiles.size();
        emit progressChanged(progress);
    }
    
    // 检查是否有有效文件
    if (files.isEmpty()) {
        result.warnings << "没有有效的输入文件";
        return result;
    }
    
    // ==================== 步骤 2：确定时间轴 ====================
    // 使用第一个文件作为时间轴基准
    const DataFile& timeAxisFile = files[0];
    QVector<DataFile> otherFiles;  // 其他需要插值的文件
    
    // 将其余文件添加到 otherFiles 列表
    for (int i = 1; i < files.size(); ++i) {
        otherFiles.append(files[i]);
    }
    
    emit statusMessage("正在合并数据...");
    
    // ==================== 步骤 3：写入输出文件 ====================
    writeOutput(outputFile, timeAxisFile, otherFiles, result);
    
    // 检查是否被取消
    if (m_cancelled) {
        result.warnings << "操作已取消";
        return result;
    }
    
    // 设置成功标志
    result.success = true;
    result.outputFile = outputFile;
    
    // 发送完成消息
    emit statusMessage("合并完成");
    emit progressChanged(100);  // 进度 100%
    
    return result;
}

void DataFileMerger::cancel() {
    m_cancelled = true;
}
void DataFileMerger::writeOutput(const QString& outputFile,
                                 const DataFile& timeAxisFile,
                                 const QVector<DataFile>& otherFiles,
                                 MergeResult& result) {
    // 创建并打开输出文件
    QFile file(outputFile);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        result.warnings << QString("无法创建输出文件：%1").arg(outputFile);
        return;
    }
    
    // 创建文本流并设置编码
    QTextStream out(&file);
    out.setCodec(QTextCodec::codecForName(m_config.outputEncoding.toUtf8()));
    
    // ==================== 步骤 1：生成并写入表头 ====================
    QVector<QString> headers = generateOutputHeaders(timeAxisFile, otherFiles);
    out << QStringList(headers.toList()).join(m_config.outputDelimiter) << "\n";
    
    // ==================== 步骤 2：写入数据行 ====================
    int totalRows = timeAxisFile.rowCount();  // 总行数（由时间轴文件决定）
    
    // 逐行写入数据
    for (int row = 0; row < totalRows; ++row) {
        // 检查是否被取消
        if (m_cancelled) {
            file.close();
            return;
        }
        
        QVector<QString> rowData;  // 当前行的数据
        
        // --- 写入时间列 ---
        rowData << timeAxisFile.timePoints[row].toString();
        
        // --- 写入第一个文件的数据列（不需要插值） ---
        for (int col = 0; col < timeAxisFile.paramCount(); ++col) {
            double value = timeAxisFile.data[row][col];
            if (std::isnan(value)) {
                rowData << "NaN";  // NaN 值特殊处理
            } else {
                rowData << QString::number(value, 'g', 10);  // 使用 10 位有效数字
            }
        }
        
        // --- 写入其他文件的数据列（需要插值） ---
        for (int fileIdx = 0; fileIdx < otherFiles.size(); ++fileIdx) {
            const DataFile& otherFile = otherFiles[fileIdx];
            // 当前行的目标时间点（来自时间轴文件）
            const TimeMerge::TimePoint& targetTime = timeAxisFile.timePoints[row];
            
            // 遍历当前文件的所有参数列
            for (int col = 0; col < otherFile.paramCount(); ++col) {
                // 获取当前列的参数名
                QString paramName = otherFile.paramNames[col];
                bool isDuplicate = false;  // 重复标志
                
                // 检查是否与第一个文件重复
                if (timeAxisFile.paramNames.contains(paramName)) {
                    isDuplicate = true;
                }
                
                // 检查是否与前面的 otherFiles 重复
                if (!isDuplicate) {
                    for (int prevIdx = 0; prevIdx < fileIdx; ++prevIdx) {
                        if (otherFiles[prevIdx].paramNames.contains(paramName)) {
                            isDuplicate = true;
                            break;
                        }
                    }
                }
                
                // 如果列名重复，跳过此列
                if (isDuplicate) {
                    continue;  // 跳过重复列
                }
                
                // 提取当前列的所有数据
                QVector<double> columnData;
                for (int r = 0; r < otherFile.rowCount(); ++r) {
                    columnData << otherFile.data[r][col];
                }
                
                // 执行插值计算
                InterpolationResult interpResult = Interpolator::interpolate(
                    otherFile.timePoints,      // 其他文件的时间点
                    columnData,                // 其他文件的当前列数据
                    targetTime,                // 目标时间点（来自时间轴）
                    m_config.interpolation,    // 插值方法（最近邻/线性）
                    m_config.timeToleranceUs   // 时间容差（微秒）
                );

                // 写入插值结果
                if (!interpResult.isValid) {
                    rowData << "NaN";  // 无法插值时写入 NaN
                    if (!interpResult.warningMessage.isEmpty()) {
                        // 记录警告信息
                    }
                } else {
                    rowData << QString::number(interpResult.value, 'g', 10);
                }
            }
        }
        
        // 写入整行数据
        out << QStringList(rowData.toList()).join(m_config.outputDelimiter) << "\n";
        
        // 更新进度（写入阶段占总进度的 60%）
        int progress = 30 + (row + 1) * 60 / totalRows;
        emit progressChanged(progress);
    }
    
    // 关闭文件
    file.close();
    result.mergedRows = totalRows;  // 记录合并行数
}

QVector<QString> DataFileMerger::generateOutputHeaders(
    const DataFile& timeAxisFile,
    const QVector<DataFile>& otherFiles) {
    
    QVector<QString> headers;  // 表头列表
    QSet<QString> usedNames;   // 记录已使用的列名（用于去重）
    
    // 添加时间列
    headers << "Time";
    usedNames.insert("Time");
    
    // 添加第一个文件的参数列
    for (const QString& paramName : timeAxisFile.paramNames) {
        headers << paramName;
        usedNames.insert(paramName);
    }
    
    // 添加其他文件的参数列（检查重复）
    for (const DataFile& otherFile : otherFiles) {
        for (const QString& paramName : otherFile.paramNames) {
            // 检查是否重复
            if (usedNames.contains(paramName)) {
                // 重复，发出警告，跳过此列
                qWarning() << QString("警告：列名 '%1' 已存在，跳过文件 %2 中的此列")
                    .arg(paramName)
                    .arg(otherFile.filename);
            } else {
                // 不重复，添加列名
                headers << paramName;
                usedNames.insert(paramName);
            }
        }
    }
    
    return headers;
}
