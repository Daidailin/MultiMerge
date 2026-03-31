#ifndef DELIMITERDETECTOR_H
#define DELIMITERDETECTOR_H

#include <QString>
#include <QChar>

class DelimiterDetector {
public:
    // 检测文件中的分隔符
    static QChar detectDelimiter(const QString& filePath);
    
    // 检测字符串中的分隔符
    static QChar detectDelimiterFromString(const QString& content);
};

#endif // DELIMITERDETECTOR_H