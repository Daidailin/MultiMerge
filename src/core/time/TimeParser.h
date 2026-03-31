#ifndef TIMEPARSER_H
#define TIMEPARSER_H

#include "TimePoint.h"
#include <string>

class TimeParser {
private:
    static const int BUFFER_SIZE = 256;

public:
    // 解析时间字符串为TimePoint
    static TimePoint parse(const std::string& timeStr);

    // 尝试多种时间格式解析
    static bool tryParse(const std::string& timeStr, TimePoint& result);

    // 检查字符串是否为有效的时间格式
    static bool isValidTimeFormat(const std::string& timeStr);
};

#endif // TIMEPARSER_H