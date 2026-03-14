#include "FileReader.h"
#include "DelimiterDetector.h"
#include "TimeParser.h"
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QFileInfo>

DataFile FileReader::load(const QString& filename) {
    DataFile file;
    file.filename = QFileInfo(filename).fileName();
    
    QFile f(filename);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        file.warnings << QString("无法打开文件：%1").arg(filename);
        return file;
    }
    
    QTextStream in(&f);
    in.setCodec("UTF-8");
    
    QVector<QString> lines;
    while (!in.atEnd()) {
        lines << in.readLine().trimmed();
    }
    f.close();
    
    if (lines.isEmpty()) {
        file.warnings << "文件为空";
        return file;
    }
    
    return loadFromText(lines.join("\n"), file.filename);
}

DataFile FileReader::loadFromText(const QString& content, const QString& filename) {
    DataFile file;
    file.filename = filename;
    
    QVector<QString> lines = content.split('\n');
    
    lines.erase(std::remove_if(lines.begin(), lines.end(),
                               [](const QString& s) { return s.trimmed().isEmpty(); }),
                lines.end());
    
    if (lines.isEmpty()) {
        file.warnings << "文件为空";
        return file;
    }
    
    file.delimiter = DelimiterDetector::detect(lines);
    
    QVector<QString> headers = DelimiterDetector::split(lines[0], file.delimiter);
    if (headers.isEmpty()) {
        file.warnings << "无法解析表头";
        return file;
    }
    
    if (headers[0].toLower() != "time" && headers[0].toLower() != "时间") {
        file.warnings << "警告：第一列可能不是时间列";
    }
    
    for (int i = 1; i < headers.size(); ++i) {
        file.paramNames << headers[i];
    }
    
    for (int i = 1; i < lines.size(); ++i) {
        const QString& line = lines[i];
        QVector<QString> values = DelimiterDetector::split(line, file.delimiter);
        
        if (values.size() < 2) {
            continue;
        }
        
        TimeMerge::TimePoint tp = TimeParser::parse(values[0]);
        file.timePoints << tp;
        
        QVector<double> rowData;
        for (int j = 1; j < values.size(); ++j) {
            bool ok = false;
            double value = values[j].toDouble(&ok);
            if (!ok) {
                if (values[j].toLower() == "nan" || values[j].toLower() == "null" ||
                    values[j].toLower() == "none" || values[j].isEmpty()) {
                    value = std::numeric_limits<double>::quiet_NaN();
                } else {
                    file.warnings << QString("第%1行第%2列解析失败：%3").arg(i+1).arg(j+1).arg(values[j]);
                    value = 0.0;
                }
            }
            rowData << value;
        }
        
        while (rowData.size() < file.paramNames.size()) {
            rowData << std::numeric_limits<double>::quiet_NaN();
        }
        
        file.data << rowData;
    }
    
    if (file.timePoints.isEmpty()) {
        file.warnings << "没有有效的数据行";
        return file;
    }
    
    file.valid = true;
    return file;
}
