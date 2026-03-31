#ifndef TIMEPOINT_H
#define TIMEPOINT_H

#include <string>
#include <iostream>

class TimePoint {
private:
    long long totalMilliseconds; // 总毫秒数

public:
    TimePoint();
    TimePoint(long long ms);
    TimePoint(int hour, int minute, int second, int millisecond);

    // 从字符串解析时间点
    bool fromString(const std::string& timeStr);

    // 转换为字符串
    std::string toString() const;

    // 获取总毫秒数
    long long getTotalMilliseconds() const;

    // 比较运算符
    bool operator<(const TimePoint& other) const;
    bool operator<=(const TimePoint& other) const;
    bool operator>(const TimePoint& other) const;
    bool operator>=(const TimePoint& other) const;
    bool operator==(const TimePoint& other) const;
    bool operator!=(const TimePoint& other) const;

    // 算术运算符
    TimePoint operator+(long long ms) const;
    TimePoint operator-(long long ms) const;
    long long operator-(const TimePoint& other) const;
};

#endif // TIMEPOINT_H