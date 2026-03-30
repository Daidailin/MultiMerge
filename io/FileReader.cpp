﻿﻿﻿#include <QTextStream>
#include <QByteArray>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include "FileReader.h"
#include "TimeParser.h"
int DataFile::findNearestTime(const TimeMerge::TimePoint& target, int64_t toleranceUs) const {
    if (timePoints.isEmpty()) {
        return -1;
    }
    
    int left = 0;
    int right = timePoints.size() - 1;
    
    if (target <= timePoints[0]) {
        int64_t diff = qAbs(target.toMicroseconds() - timePoints[0].toMicroseconds());
        if (toleranceUs < 0 || diff <= toleranceUs) {
            return 0;
        }
        return -1;
    }

    if (target >= timePoints[right]) {
        int64_t diff = qAbs(target.toMicroseconds() - timePoints[right].toMicroseconds());
        if (toleranceUs < 0 || diff <= toleranceUs) {
            return right;
        }
        return -1;
    }
    
    while (left <= right) {
        int mid = left + (right - left) / 2;
        
        if (timePoints[mid] == target) {
            return mid;
        }
        
        if (timePoints[mid] < target) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }
    
    if (left >= timePoints.size()) {
        return right;
    }
    if (right < 0) {
        return left;
    }
    
    int64_t diffLeft = qAbs(target.toMicroseconds() - timePoints[left].toMicroseconds());
    int64_t diffRight = qAbs(target.toMicroseconds() - timePoints[right].toMicroseconds());
    
    int nearestIndex = (diffLeft <= diffRight) ? left : right;
    int64_t nearestDiff = qAbs(target.toMicroseconds() - timePoints[nearestIndex].toMicroseconds());
    if (toleranceUs >= 0 && nearestDiff > toleranceUs) {
        return -1;
    }
    
    return nearestIndex;
}

DataFile FileReader::load(const QString& filename) {
    DataFile result;
    result.filename = filename;
    result.valid = false;
    
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.warnings << QString("无法打开文件：%1").arg(filename);
        return result;
    }
    
    result.encoding = detectEncoding(filename);
    
    QTextStream in(&file);
    in.setCodec(result.encoding);
    
    QVector<QString> lines;
    QString line;
    while (!in.atEnd()) {
        line = in.readLine().trimmed();
        if (!line.isEmpty()) {
            lines.append(line);
        }
    }
    file.close();
    
    if (lines.isEmpty()) {
        result.warnings << QString("文件为空：%1").arg(filename);
        return result;
    }
    
    result.delimiterType = DelimiterDetector::detect(lines);
    result.delimiter = DelimiterDetector::toChar(result.delimiterType);
    
    result.paramNames = parseHeader(lines[0], result.delimiter);
    
    int expectedColumns = result.paramNames.size() + 1;
    
    for (int i = 1; i < lines.size(); ++i) {
        TimeMerge::TimePoint timePoint;
        QVector<double> values;
        
        if (parseLine(lines[i], result.delimiter, timePoint, values)) {
            result.timePoints.append(timePoint);
            result.data.append(values);
            
            if (values.size() != result.paramNames.size()) {
                QString warning = QString("文件 %1 第 %2 行：期望 %3 列，实际 %4 列")
                    .arg(filename)
                    .arg(i + 1)
                    .arg(result.paramNames.size())
                    .arg(values.size());
                result.warnings << warning;
            }
        } else {
            QString warning = QString("文件 %1 第 %2 行：解析失败，已跳过")
                .arg(filename)
                .arg(i + 1);
            result.warnings << warning;
        }
    }
    
    if (result.timePoints.isEmpty()) {
        result.warnings << QString("文件 %1 中没有有效的数据行").arg(filename);
        return result;
    }
    
    result.valid = true;
    return result;
}

QTextCodec* FileReader::detectEncoding(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) {
        return QTextCodec::codecForName("UTF-8");
    }
    
    QByteArray data = file.read(1024);  // 读取前 1KB
    file.close();
    
    // 检查 BOM
    if (data.startsWith("\xEF\xBB\xBF")) {
        return QTextCodec::codecForName("UTF-8");
    }
    if (data.startsWith("\xFF\xFE")) {
        return QTextCodec::codecForName("UTF-16");
    }
    if (data.startsWith("\xFE\xFF")) {
        return QTextCodec::codecForName("UTF-16BE");
    }
    
    // 尝试检测是否包含中文字符（GBK 编码）
    // 简单检测：如果有连续的高字节，可能是 GBK
    bool hasHighBytes = false;
    for (int i = 0; i < data.size(); ++i) {
        uchar ch = static_cast<uchar>(data[i]);
        if (ch >= 0x80) {
            hasHighBytes = true;
            // 检查下一个字节
            if (i + 1 < data.size()) {
                uchar nextCh = static_cast<uchar>(data[i + 1]);
                if (nextCh >= 0x40 && nextCh <= 0xFE) {
                    // 可能是 GBK
                    return QTextCodec::codecForName("GBK");
                }
            }
        }
    }
    
    // 默认返回 UTF-8
    return QTextCodec::codecForName("UTF-8");
}

bool FileReader::parseLine(const QString& line,
                           QChar delimiter,
                           TimeMerge::TimePoint& timePoint,
                           QVector<double>& values) {
    QStringList parts = line.split(delimiter);

    if (parts.isEmpty()) {
        return false;
    }

    QString timeStr = parts[0].trimmed();
    timeStr = timeStr.replace('T', ' ');

    TimeMerge::ParseResult result = TimeMerge::TimeParser::instance().parse(timeStr);
    if (!result.isSuccess()) {
        return false;
    }

    timePoint = result.timePoint();

    for (int i = 1; i < parts.size(); ++i) {
        QString valueStr = parts[i].trimmed();
        if (valueStr.isEmpty() || valueStr == "NaN" || valueStr == "nan") {
            values.append(std::numeric_limits<double>::quiet_NaN());
        } else {
            bool ok = false;
            double value = valueStr.toDouble(&ok);
            if (ok) {
                values.append(value);
            } else {
                values.append(std::numeric_limits<double>::quiet_NaN());
            }
        }
    }

    return true;
}

QVector<QString> FileReader::parseHeader(const QString& headerLine, 
                                         QChar delimiter) {
    QVector<QString> paramNames;
    QStringList parts = headerLine.split(delimiter);
    
    // 跳过第一列（时间列）
    for (int i = 1; i < parts.size(); ++i) {
        paramNames.append(parts[i].trimmed());
    }
    
    return paramNames;
}
