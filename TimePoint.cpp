#include "TimePoint.h"
#include <QString>
#include <stdexcept>

namespace TimeMerge {

TimePoint::TimePoint(TimeResolution resolution)
    : m_hour(0), m_minute(0), m_second(0), m_fraction(0), m_resolution(resolution) {
}

TimePoint::TimePoint(int hour, int minute, int second, int64_t fraction,
                     TimeResolution resolution)
    : m_hour(hour), m_minute(minute), m_second(second), m_fraction(fraction),
      m_resolution(resolution) {
    normalize();
}

void TimePoint::normalize() {
    int64_t maxFrac = maxFraction();
    
    while (m_fraction >= maxFrac) {
        m_fraction -= maxFrac;
        m_second++;
    }
    while (m_fraction < 0) {
        m_fraction += maxFrac;
        m_second--;
    }
    
    while (m_second >= 60) {
        m_second -= 60;
        m_minute++;
    }
    while (m_second < 0) {
        m_second += 60;
        m_minute--;
    }
    
    while (m_minute >= 60) {
        m_minute -= 60;
        m_hour++;
    }
    while (m_minute < 0) {
        m_minute += 60;
        m_hour--;
    }
    
    while (m_hour >= 24) {
        m_hour -= 24;
    }
    while (m_hour < 0) {
        m_hour += 24;
    }
}

int64_t TimePoint::maxFraction() const {
    switch (m_resolution) {
        case TimeResolution::Milliseconds: return 1000;
        case TimeResolution::Microseconds: return 1000000;
        case TimeResolution::Nanoseconds: return 1000000000;
        default: return 1000000;
    }
}

int64_t TimePoint::toMicroseconds() const {
    int64_t us = m_hour * 3600000000LL;
    us += m_minute * 60000000LL;
    us += m_second * 1000000LL;
    
    switch (m_resolution) {
        case TimeResolution::Milliseconds:
            us += m_fraction * 1000;
            break;
        case TimeResolution::Microseconds:
            us += m_fraction;
            break;
        case TimeResolution::Nanoseconds:
            us += m_fraction / 1000;
            break;
    }
    return us;
}

int64_t TimePoint::toMilliseconds() const {
    return toMicroseconds() / 1000;
}

int64_t TimePoint::toNanoseconds() const {
    int64_t ns = m_hour * 3600000000000LL;
    ns += m_minute * 60000000000LL;
    ns += m_second * 1000000000LL;
    
    switch (m_resolution) {
        case TimeResolution::Milliseconds:
            ns += m_fraction * 1000000;
            break;
        case TimeResolution::Microseconds:
            ns += m_fraction * 1000;
            break;
        case TimeResolution::Nanoseconds:
            ns += m_fraction;
            break;
    }
    return ns;
}

QString TimePoint::toString(const QString& format) const {
    if (!format.isEmpty()) {
        return format
            .replace("hh", QString::number(m_hour).rightJustified(2, '0'))
            .replace("mm", QString::number(m_minute).rightJustified(2, '0'))
            .replace("ss", QString::number(m_second).rightJustified(2, '0'))
            .replace("fff", QString::number(m_fraction).leftJustified(3, '0'))
            .replace("u", QString::number(m_fraction).leftJustified(6, '0'))
            .replace("n", QString::number(m_fraction).leftJustified(9, '0'));
    }
    
    switch (m_resolution) {
        case TimeResolution::Milliseconds:
            return QString("%1:%2:%3.%4")
                .arg(m_hour, 2, 10, QChar('0'))
                .arg(m_minute, 2, 10, QChar('0'))
                .arg(m_second, 2, 10, QChar('0'))
                .arg(m_fraction, 3, 10, QChar('0'));
        case TimeResolution::Microseconds:
            return QString("%1:%2:%3.%4")
                .arg(m_hour, 2, 10, QChar('0'))
                .arg(m_minute, 2, 10, QChar('0'))
                .arg(m_second, 2, 10, QChar('0'))
                .arg(m_fraction, 6, 10, QChar('0'));
        case TimeResolution::Nanoseconds:
            return QString("%1:%2:%3.%4")
                .arg(m_hour, 2, 10, QChar('0'))
                .arg(m_minute, 2, 10, QChar('0'))
                .arg(m_second, 2, 10, QChar('0'))
                .arg(m_fraction, 9, 10, QChar('0'));
        default:
            return QString();
    }
}

double TimePoint::toSeconds() const {
    return toMicroseconds() / 1000000.0;
}

TimePoint TimePoint::fromSeconds(double totalSeconds, TimeResolution resolution) {
    int64_t micros = static_cast<int64_t>(totalSeconds * 1000000.0);
    
    int hour = micros / 3600000000LL;
    micros %= 3600000000LL;
    
    int minute = micros / 60000000LL;
    micros %= 60000000LL;
    
    int second = micros / 1000000LL;
    int64_t fraction = micros % 1000000LL;
    
    switch (resolution) {
        case TimeResolution::Milliseconds:
            fraction /= 1000;
            break;
        case TimeResolution::Nanoseconds:
            fraction *= 1000;
            break;
        case TimeResolution::Microseconds:
            break;
    }
    
    return TimePoint(hour, minute, second, fraction, resolution);
}

bool TimePoint::operator==(const TimePoint& other) const {
    return toMicroseconds() == other.toMicroseconds();
}

bool TimePoint::operator!=(const TimePoint& other) const {
    return toMicroseconds() != other.toMicroseconds();
}

bool TimePoint::operator<(const TimePoint& other) const {
    return toMicroseconds() < other.toMicroseconds();
}

bool TimePoint::operator<=(const TimePoint& other) const {
    return toMicroseconds() <= other.toMicroseconds();
}

bool TimePoint::operator>(const TimePoint& other) const {
    return toMicroseconds() > other.toMicroseconds();
}

bool TimePoint::operator>=(const TimePoint& other) const {
    return toMicroseconds() >= other.toMicroseconds();
}

TimePoint TimePoint::operator+(int64_t microseconds) const {
    return fromSeconds(toSeconds() + microseconds / 1000000.0, m_resolution);
}

TimePoint TimePoint::operator-(int64_t microseconds) const {
    return fromSeconds(toSeconds() - microseconds / 1000000.0, m_resolution);
}

int64_t TimePoint::operator-(const TimePoint& other) const {
    return toMicroseconds() - other.toMicroseconds();
}

} // namespace TimeMerge
