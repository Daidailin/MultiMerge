#include "TimePoint.h"
#include <QStringBuilder>
#include <cmath>

namespace TimeMerge {

TimePoint::TimePoint(TimeResolution resolution)
    : m_hour(0), m_minute(0), m_second(0), m_fraction(0), m_resolution(resolution) {
}

TimePoint::TimePoint(int hour, int minute, int second, int64_t fraction,
                     TimeResolution resolution)
    : m_hour(hour), m_minute(minute), m_second(second),
      m_fraction(fraction), m_resolution(resolution) {
    normalize();
}

void TimePoint::normalize() {
    // 转换为纳秒以便统一处理（内部计算单位）
    int64_t totalNs = toNanoseconds();
    
    // 定义时间常量（单位：纳秒）
    constexpr int64_t nsPerDay = 86400000000000LL;     // 一天的纳秒数
    constexpr int64_t nsPerHour = 3600000000000LL;     // 一小时的纳秒数
    constexpr int64_t nsPerMinute = 60000000000LL;     // 一分钟的纳秒数
    constexpr int64_t nsPerSecond = 1000000000LL;      // 一秒的纳秒数
    
    // 终极取模公式：处理正负溢出，确保结果在 [0, nsPerDay) 范围内
    // 公式原理：(a % n + n) % n 可以正确处理负数取模
    totalNs = (totalNs % nsPerDay + nsPerDay) % nsPerDay;
    
    // 提取小时（总纳秒 / 每小时纳秒数）
    m_hour = static_cast<int>(totalNs / nsPerHour);
    totalNs %= nsPerHour;
    
    // 提取分钟（剩余纳秒 / 每分钟纳秒数）
    m_minute = static_cast<int>(totalNs / nsPerMinute);
    totalNs %= nsPerMinute;
    
    // 提取秒（剩余纳秒 / 每秒纳秒数）
    m_second = static_cast<int>(totalNs / nsPerSecond);
    totalNs %= nsPerSecond;
    
    // 根据分辨率提取小数部分（只支持毫秒和微秒）
    switch (m_resolution) {
        case TimeResolution::Milliseconds:
            m_fraction = totalNs / 1000000LL;  // 纳秒 → 毫秒
            break;
        case TimeResolution::Microseconds:
            m_fraction = totalNs / 1000LL;     // 纳秒 → 微秒
            break;
    }
}


int64_t TimePoint::toNanoseconds() const {
    int64_t total = 0;
    // 累加各时间分量到纳秒（内部计算单位）
    total += static_cast<int64_t>(m_hour) * 3600000000000LL;    // 小时 → 纳秒
    total += static_cast<int64_t>(m_minute) * 60000000000LL;    // 分钟 → 纳秒
    total += static_cast<int64_t>(m_second) * 1000000000LL;     // 秒 → 纳秒
    
    // 根据分辨率累加小数部分（只支持毫秒和微秒）
    switch (m_resolution) {
        case TimeResolution::Milliseconds:
            total += m_fraction * 1000000LL;    // 毫秒 → 纳秒
            break;
        case TimeResolution::Microseconds:
            total += m_fraction * 1000LL;       // 微秒 → 纳秒
            break;
    }
    
    return total;
}

int64_t TimePoint::toMicroseconds() const {
    return toNanoseconds() / 1000LL;
}
int64_t TimePoint::toMilliseconds() const {
    return toNanoseconds() / 1000000LL;
}

double TimePoint::toSeconds() const {
    return static_cast<double>(toNanoseconds()) / 1000000000.0;
}

TimePoint TimePoint::fromSeconds(double totalSeconds, TimeResolution resolution) {
    return fromNanoseconds(static_cast<int64_t>(totalSeconds * 1000000000.0), resolution);
}

TimePoint TimePoint::fromNanoseconds(int64_t totalNanoseconds, TimeResolution resolution) {
    TimePoint tp(resolution);
    
    // 定义时间常量（单位：纳秒，内部计算单位）
    constexpr int64_t nsPerDay = 86400000000000LL;     // 一天的纳秒数
    constexpr int64_t nsPerHour = 3600000000000LL;     // 一小时的纳秒数
    constexpr int64_t nsPerMinute = 60000000000LL;     // 一分钟的纳秒数
    constexpr int64_t nsPerSecond = 1000000000LL;      // 一秒的纳秒数
    
    // 终极取模公式：处理正负溢出，确保在 [0, nsPerDay) 范围内
    totalNanoseconds = (totalNanoseconds % nsPerDay + nsPerDay) % nsPerDay;
    
    // 依次提取时、分、秒
    tp.m_hour = static_cast<int>(totalNanoseconds / nsPerHour);
    totalNanoseconds %= nsPerHour;
    
    tp.m_minute = static_cast<int>(totalNanoseconds / nsPerMinute);
    totalNanoseconds %= nsPerMinute;
    
    tp.m_second = static_cast<int>(totalNanoseconds / nsPerSecond);
    totalNanoseconds %= nsPerSecond;
    
    // 根据分辨率设置小数部分（只支持毫秒和微秒）
    switch (resolution) {
        case TimeResolution::Milliseconds:
            tp.m_fraction = totalNanoseconds / 1000000LL;  // 纳秒 → 毫秒
            break;
        case TimeResolution::Microseconds:
            tp.m_fraction = totalNanoseconds / 1000LL;     // 纳秒 → 微秒
            break;
    }
    
    tp.m_resolution = resolution;
    return tp;
}

QString TimePoint::toString(const QString& format) const {
    // 默认格式：HH:MM:SS:SSS（毫秒）或 HH:MM:SS:SSSSSS（微秒）
    if (format.isEmpty()) {
        QString result = QString::number(m_hour).rightJustified(2, '0') + ":" +
                        QString::number(m_minute).rightJustified(2, '0') + ":" +
                        QString::number(m_second).rightJustified(2, '0');
        
        // 添加冒号分隔的小数部分
        int decimalPlaces = 0;
        switch (m_resolution) {
            case TimeResolution::Milliseconds:
                decimalPlaces = 3;
                break;
            case TimeResolution::Microseconds:
                decimalPlaces = 6;
                break;
        }
        
        if (decimalPlaces > 0) {
            result += ":" + QString::number(m_fraction).rightJustified(decimalPlaces, '0');
        }
        
        return result;
    }
    
    // 自定义格式处理
    QString result = format;
    // 替换时分秒占位符（统一补零）
    result.replace("hh", QString::number(m_hour).rightJustified(2, '0'));
    result.replace("mm", QString::number(m_minute).rightJustified(2, '0'));
    result.replace("ss", QString::number(m_second).rightJustified(2, '0'));
    
    // 计算纳秒级别的小数尾数（用于高精度格式化）
    int64_t nsFraction = toNanoseconds() % 1000000000LL;
    int ms = nsFraction / 1000000LL;  // 提取毫秒
    int us = nsFraction / 1000LL;     // 提取微秒
    
    // 替换小数占位符（支持点号和冒号分隔符，统一补零）
    // 点号分隔
    result.replace("fff", QString::number(ms).rightJustified(3, '0'));
    result.replace("u", QString::number(us).rightJustified(6, '0'));
    // 冒号分隔
    result.replace(":fff", ":" + QString::number(ms).rightJustified(3, '0'));
    result.replace(":u", ":" + QString::number(us).rightJustified(6, '0'));
    
    return result;
}

int64_t TimePoint::maxFraction() const {
    switch (m_resolution) {
        case TimeResolution::Milliseconds:
            return 1000LL;        // 毫秒：0-999
        case TimeResolution::Microseconds:
            return 1000000LL;     // 微秒：0-999999
    }
    return 1000000LL;  // 默认返回微秒
}

bool TimePoint::operator==(const TimePoint& other) const {
    return toNanoseconds() == other.toNanoseconds();
}


bool TimePoint::operator!=(const TimePoint& other) const {
    return toNanoseconds() != other.toNanoseconds();
}


bool TimePoint::operator<(const TimePoint& other) const {
    return toNanoseconds() < other.toNanoseconds();
}

bool TimePoint::operator<=(const TimePoint& other) const {
    return toNanoseconds() <= other.toNanoseconds();
}

bool TimePoint::operator>(const TimePoint& other) const {
    return toNanoseconds() > other.toNanoseconds();
}

bool TimePoint::operator>=(const TimePoint& other) const {
    return toNanoseconds() >= other.toNanoseconds();
}

TimePoint TimePoint::operator+(int64_t microseconds) const {
    int64_t totalNs = toNanoseconds() + microseconds * 1000LL;
    return fromNanoseconds(totalNs, m_resolution);
}

TimePoint TimePoint::operator-(int64_t microseconds) const {
    int64_t totalNs = toNanoseconds() - microseconds * 1000LL;
    return fromNanoseconds(totalNs, m_resolution);
}

int64_t TimePoint::operator-(const TimePoint& other) const {
    return (toNanoseconds() - other.toNanoseconds()) / 1000LL;
}

} // namespace TimeMerge
