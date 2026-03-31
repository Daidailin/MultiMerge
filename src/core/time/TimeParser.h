#ifndef TIMEPARSER_H
#define TIMEPARSER_H

#include <QString>
#include "TimePoint.h"

class TimeParser {
public:
    // 解析时间字符串为TimePoint对象
    static TimePoint parse(const QString& timeStr);
    
    // 检查时间字符串是否有效
    static bool isValidTimeString(const QString& timeStr);
};

#endif // TIMEPARSER_H