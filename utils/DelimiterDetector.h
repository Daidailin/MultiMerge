#ifndef DELIMITERDETECTOR_H
#define DELIMITERDETECTOR_H

#include <QString>
#include <QVector>

class DelimiterDetector {
public:
    static QChar detect(const QVector<QString>& lines);
    static QChar detect(const QString& line);
    static QVector<QString> split(const QString& line, QChar delimiter);
};

#endif // DELIMITERDETECTOR_H
