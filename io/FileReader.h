#ifndef FILEREADER_H
#define FILEREADER_H

#include <QString>
#include <QVector>
#include <QTextCodec>
#include "TimePoint.h"
#include "utils/DelimiterDetector.h"


struct DataFile {
    QString filename;
    DelimiterDetector::DelimiterType delimiterType;
    QChar delimiter;
    QVector<QString> paramNames;
    QVector<TimeMerge::TimePoint> timePoints;
    QVector<QVector<double>> data;
    QTextCodec* encoding;
    bool valid;
    QVector<QString> warnings;
    
    DataFile() : delimiter(','), encoding(nullptr), valid(false) {}
    

    int rowCount() const {
        return timePoints.size();
    }

    int paramCount() const {
        return paramNames.size();
    }
    

    int findNearestTime(const TimeMerge::TimePoint& target, int64_t toleranceUs = 0) const;
};

class FileReader {
public:
    static DataFile load(const QString& filename);
    
    static QTextCodec* detectEncoding(const QString& filename);
    
private:
    static bool parseLine(const QString& line, 
                         QChar delimiter,
                         TimeMerge::TimePoint& timePoint,
                         QVector<double>& values);
    
    static QVector<QString> parseHeader(const QString& headerLine, 
                                       QChar delimiter);
};

#endif // FILEREADER_H
