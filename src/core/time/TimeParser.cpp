#include "TimeParser.h"

TimePoint TimeParser::parse(const QString& timeStr) {
    TimePoint timePoint;
    timePoint.fromString(timeStr);
    return timePoint;
}

bool TimeParser::isValidTimeString(const QString& timeStr) {
    TimePoint timePoint;
    return timePoint.fromString(timeStr);
}