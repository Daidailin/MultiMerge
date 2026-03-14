#include "TimeParser.h"
#include <QRegularExpression>
#include <QDebug>

TimeMerge::TimePoint TimeParser::parse(const QString& timeStr,
                                       TimeMerge::TimeResolution resolution) {
    QRegularExpression re(R"((\d{1,2}):(\d{2}):(\d{2})(?:\.(\d+))?)");
    QRegularExpressionMatch match = re.match(timeStr.trimmed());
    
    if (!match.hasMatch()) {
        qWarning() << "无法解析时间字符串:" << timeStr;
        return TimeMerge::TimePoint(0, 0, 0, 0, resolution);
    }
    
    int hour = match.captured(1).toInt();
    int minute = match.captured(2).toInt();
    int second = match.captured(3).toInt();
    
    int64_t fraction = 0;
    if (match.captured(4).isValid()) {
        QString fracStr = match.captured(4);
        
        switch (resolution) {
            case TimeMerge::TimeResolution::Milliseconds:
                if (fracStr.length() > 3) {
                    fracStr = fracStr.left(3);
                } else {
                    fracStr = fracStr.rightJustified(3, '0');
                }
                break;
            case TimeMerge::TimeResolution::Microseconds:
                if (fracStr.length() > 6) {
                    fracStr = fracStr.left(6);
                } else {
                    fracStr = fracStr.rightJustified(6, '0');
                }
                break;
            case TimeMerge::TimeResolution::Nanoseconds:
                if (fracStr.length() > 9) {
                    fracStr = fracStr.left(9);
                } else {
                    fracStr = fracStr.rightJustified(9, '0');
                }
                break;
        }
        
        fraction = fracStr.toLongLong();
    }
    
    return TimeMerge::TimePoint(hour, minute, second, fraction, resolution);
}

QVector<TimeMerge::TimePoint> TimeParser::parseColumn(const QVector<QString>& timeColumn) {
    QVector<TimeMerge::TimePoint> result;
    result.reserve(timeColumn.size());
    
    TimeMerge::TimeResolution resolution = TimeMerge::TimeResolution::Microseconds;
    if (!timeColumn.isEmpty()) {
        resolution = detectFormat(timeColumn[0]);
    }
    
    for (const QString& timeStr : timeColumn) {
        result << parse(timeStr, resolution);
    }
    
    return result;
}

TimeMerge::TimeResolution TimeParser::detectFormat(const QString& timeStr) {
    QRegularExpression re(R"((\d{1,2}):(\d{2}):(\d{2})(?:\.(\d+))?)");
    QRegularExpressionMatch match = re.match(timeStr.trimmed());
    
    if (!match.hasMatch()) {
        return TimeMerge::TimeResolution::Microseconds;
    }
    
    if (match.captured(4).isValid()) {
        int fracLen = match.captured(4).length();
        if (fracLen <= 3) {
            return TimeMerge::TimeResolution::Milliseconds;
        } else if (fracLen <= 6) {
            return TimeMerge::TimeResolution::Microseconds;
        } else {
            return TimeMerge::TimeResolution::Nanoseconds;
        }
    }
    
    return TimeMerge::TimeResolution::Microseconds;
}
