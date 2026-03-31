#ifndef TIMEPOINT_H
#define TIMEPOINT_H

#include <QString>

class TimePoint {
private:
    int m_hours;
    int m_minutes;
    int m_seconds;
    int m_milliseconds;

public:
    TimePoint();
    TimePoint(int hours, int minutes, int seconds, int milliseconds);
    
    // 从字符串解析时间点
    bool fromString(const QString& timeStr);
    
    // 转换为字符串
    QString toString() const;
    
    // 比较运算符
    bool operator<(const TimePoint& other) const;
    bool operator==(const TimePoint& other) const;
    
    // 转换为毫秒数
    long long toMilliseconds() const;
    
    // 从毫秒数创建时间点
    static TimePoint fromMilliseconds(long long ms);
    
    // 获取各个时间分量
    int hours() const { return m_hours; }
    int minutes() const { return m_minutes; }
    int seconds() const { return m_seconds; }
    int milliseconds() const { return m_milliseconds; }
};

#endif // TIMEPOINT_H