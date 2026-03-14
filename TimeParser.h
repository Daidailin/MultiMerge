#ifndef TIMEPARSER_H
#define TIMEPARSER_H

#include "TimePoint.h"
#include <QString>
#include <QVector>
#include <QMap>

class TimeParser {
public:
    static TimeMerge::TimePoint parse(const QString& timeStr,
                                     TimeMerge::TimeResolution resolution = TimeMerge::TimeResolution::Microseconds);
    static QVector<TimeMerge::TimePoint> parseColumn(const QVector<QString>& timeColumn);
    static TimeMerge::TimeResolution detectFormat(const QString& timeStr);
};

#endif // TIMEPARSER_H
