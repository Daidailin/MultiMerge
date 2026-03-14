#include "DelimiterDetector.h"
#include <QMap>

QChar DelimiterDetector::detect(const QVector<QString>& lines) {
    if (lines.isEmpty()) {
        return '\t';
    }
    
    int count = qMin(5, lines.size());
    
    QMap<QChar, int> scores;
    scores['\t'] = 0;
    scores[','] = 0;
    scores[';'] = 0;
    scores[' '] = 0;
    
    for (int i = 0; i < count; ++i) {
        const QString& line = lines[i];
        
        int tabCount = line.count('\t');
        int commaCount = line.count(',');
        int semicolonCount = line.count(';');
        int spaceCount = line.count(' ');
        
        if (tabCount > 1) scores['\t'] += tabCount;
        if (commaCount > 1) scores[','] += commaCount;
        if (semicolonCount > 1) scores[';'] += semicolonCount;
        if (spaceCount > 1) scores[' '] += spaceCount;
    }
    
    QChar bestDelimiter = '\t';
    int bestScore = 0;
    
    for (auto it = scores.begin(); it != scores.end(); ++it) {
        if (it.value() > bestScore) {
            bestScore = it.value();
            bestDelimiter = it.key();
        }
    }
    
    return bestDelimiter;
}

QChar DelimiterDetector::detect(const QString& line) {
    return detect(QVector<QString>() << line);
}

QVector<QString> DelimiterDetector::split(const QString& line, QChar delimiter) {
    QVector<QString> result;
    
    QStringList parts = line.split(delimiter);
    for (const QString& part : parts) {
        result << part.trimmed();
    }
    
    return result;
}
