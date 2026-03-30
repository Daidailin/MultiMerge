#include "DelimiterDetector.h"
#include <QMap>
#include <QRegularExpression>

DelimiterDetector::DelimiterType DelimiterDetector::detect(const QVector<QString>& lines) {
    if (lines.isEmpty()) {
        return DelimiterType::Unknown;
    }
    
    // 候选分隔符及其优先级
    QMap<QChar, int> candidates = {
        std::make_pair(QChar(','), 0),
        std::make_pair(QChar(';'), 0),
        std::make_pair(QChar('\t'), 0),
        std::make_pair(QChar(' '), 0)
    };
    
    // 计算每个候选分隔符的得分
    for (auto it = candidates.begin(); it != candidates.end(); ++it) {
        it.value() = scoreDelimiter(lines, it.key());
    }
    
    // 找到得分最高的分隔符
    QChar bestDelimiter = ',';
    int bestScore = -1;
    
    for (auto it = candidates.begin(); it != candidates.end(); ++it) {
        if (it.value() > bestScore) {
            bestScore = it.value();
            bestDelimiter = it.key();
        }
    }
    
    // 将 QChar 转换为 DelimiterType
    switch (bestDelimiter.toLatin1()) {
        case ',': return DelimiterType::Comma;
        case ';': return DelimiterType::Semicolon;
        case '\t': return DelimiterType::Tab;
        case ' ': return DelimiterType::Space;
        default: return DelimiterType::Unknown;
    }
}

DelimiterDetector::DelimiterType DelimiterDetector::detect(const QString& line) {
    return detect(QVector<QString>{line});
}

QChar DelimiterDetector::toChar(DelimiterType type) {
    switch (type) {
        case DelimiterType::Comma: return ',';
        case DelimiterType::Semicolon: return ';';
        case DelimiterType::Tab: return '\t';
        case DelimiterType::Space: return ' ';
        default: return ',';
    }
}

QString DelimiterDetector::toString(DelimiterType type) {
    switch (type) {
        case DelimiterType::Comma: return "逗号 (,)";
        case DelimiterType::Semicolon: return "分号 (;)";
        case DelimiterType::Tab: return "制表符 (\\t)";
        case DelimiterType::Space: return "空格";
        default: return "未知";
    }
}

int DelimiterDetector::countOccurrences(const QString& line, QChar ch) {
    int count = 0;
    for (const QChar& c : line) {
        if (c == ch) {
            count++;
        }
    }
    return count;
}

int DelimiterDetector::scoreDelimiter(const QVector<QString>& lines, QChar ch) {
    if (lines.isEmpty()) {
        return 0;
    }
    
    // 统计第一行中该字符出现的次数
    int firstLineCount = countOccurrences(lines[0], ch);
    if (firstLineCount == 0) {
        return 0;  // 第一行没有出现，不可能是分隔符
    }
    
    // 检查其他行是否有一致的出现次数
    int consistentCount = 1;  // 第一行已经算一次
    for (int i = 1; i < lines.size() && i < 5; ++i) {  // 最多检查前 5 行
        int count = countOccurrences(lines[i], ch);
        if (count == firstLineCount) {
            consistentCount++;
        }
    }
    
    // 得分 = 出现次数 × 一致性
    // 制表符和分号有额外加分（因为更少歧义）
    int score = firstLineCount * consistentCount;
    
    if (ch == '\t') {
        score *= 2;  // 制表符优先级最高
    } else if (ch == ';') {
        score *= 1.5;  // 分号次之
    }
    
    return score;
}
