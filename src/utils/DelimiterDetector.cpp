#include "DelimiterDetector.h"
#include <QFile>
#include <QTextStream>
#include <QMap>

QChar DelimiterDetector::detectDelimiter(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return ','; // 默认返回逗号
    }

    QTextStream in(&file);
    QString line = in.readLine();
    file.close();

    if (line.isEmpty()) {
        return ','; // 默认返回逗号
    }

    return detectDelimiterFromString(line);
}

QChar DelimiterDetector::detectDelimiterFromString(const QString& content) {
    QMap<QChar, int> delimiterCounts;
    delimiterCounts[','] = 0;
    delimiterCounts[';'] = 0;
    delimiterCounts['\t'] = 0;
    delimiterCounts[' '] = 0;

    for (const QChar& c : content) {
        if (delimiterCounts.contains(c)) {
            delimiterCounts[c]++;
        }
    }

    QChar bestDelimiter = ',';
    int maxCount = 0;

    for (auto it = delimiterCounts.constBegin(); it != delimiterCounts.constEnd(); ++it) {
        if (it.value() > maxCount) {
            maxCount = it.value();
            bestDelimiter = it.key();
        }
    }

    return bestDelimiter;
}