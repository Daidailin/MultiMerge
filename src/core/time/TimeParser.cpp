#include "TimeParser.h"
#include <sstream>
#include <regex>

TimePoint TimeParser::parse(const std::string& timeStr) {
    TimePoint result;
    if (!tryParse(timeStr, result)) {
        // 如果解析失败，返回默认时间点
        return TimePoint();
    }
    return result;
}

bool TimeParser::tryParse(const std::string& timeStr, TimePoint& result) {
    // 尝试标准格式: HH:MM:SS.fff
    if (result.fromString(timeStr)) {
        return true;
    }

    // 尝试其他格式...
    // 这里可以添加更多时间格式的支持

    return false;
}

bool TimeParser::isValidTimeFormat(const std::string& timeStr) {
    // 使用正则表达式检查时间格式
    std::regex timeRegex(R"(^\d{2}:\d{2}:\d{2}\.\d{3}$)");
    return std::regex_match(timeStr, timeRegex);
}