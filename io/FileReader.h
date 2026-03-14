#ifndef FILEREADER_H
#define FILEREADER_H

#include "TimePoint.h"
#include <QString>
#include <QVector>
#include <QMap>

struct DataFile {
    QString filename;
    QVector<TimeMerge::TimePoint> timePoints;
    QVector<QVector<double>> data;
    QVector<QString> paramNames;
    QChar delimiter;
    bool valid;
    QVector<QString> warnings;
    
    DataFile() : valid(false), delimiter('\t') {}
    
    int rowCount() const { return timePoints.size(); }
    int paramCount() const { return paramNames.size(); }
};

class FileReader {
public:
    static DataFile load(const QString& filename);
    static DataFile loadFromText(const QString& content, const QString& filename = QString());
};

#endif // FILEREADER_H
