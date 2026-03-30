#ifndef DELIMITERDETECTOR_H
#define DELIMITERDETECTOR_H

#include <QString>
#include <QVector>

class DelimiterDetector {
public:

    enum class DelimiterType {
        Comma,      ///< 逗号（,）：CSV 格式
        Semicolon,  ///< 分号（;）：欧洲 CSV 格式
        Tab,        ///< 制表符（\t）：TSV 格式
        Space,      ///< 空格（ ）：空格分隔格式
        Unknown     ///< 未知：无法确定分隔符
    };
    
    static DelimiterType detect(const QVector<QString>& lines);
    
    static DelimiterType detect(const QString& line);
    
    static QChar toChar(DelimiterType type);

    static QString toString(DelimiterType type);
    
private:

    static int countOccurrences(const QString& line, QChar ch);
    
    static int scoreDelimiter(const QVector<QString>& lines, QChar ch);
};

#endif // DELIMITERDETECTOR_H
