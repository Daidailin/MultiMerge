#include "TimePoint.h"

TimePoint::TimePoint() 
    : m_hours(0), m_minutes(0), m_seconds(0), m_milliseconds(0) {
}

TimePoint::TimePoint(int hours, int minutes, int seconds, int milliseconds) 
    : m_hours(hours), m_minutes(minutes), m_seconds(seconds), m_milliseconds(milliseconds) {
}

bool TimePoint::fromString(const QString& timeStr) {
    QStringList parts;
    if (timeStr.contains(":")) {
        parts = timeStr.split(":");
    } else if (timeStr.contains(".")) {
        parts = timeStr.split(".");
    } else {
        return false;
    }

    if (parts.size() != 4) {
        return false;
    }

    bool ok;
    m_hours = parts[0].toInt(&ok);
    if (!ok || m_hours < 0 || m_hours >= 24) return false;

    m_minutes = parts[1].toInt(&ok);
    if (!ok || m_minutes < 0 || m_minutes >= 60) return false;

    m_seconds = parts[2].toInt(&ok);
    if (!ok || m_seconds < 0 || m_seconds >= 60) return false;

    m_milliseconds = parts[3].toInt(&ok);
    if (!ok || m_milliseconds < 0 || m_milliseconds >= 1000) return false;

    return true;
}

QString TimePoint::toString() const {
    return QString("%1:%2:%3:%4")
        .arg(m_hours, 2, 10, QLatin1Char('0'))
        .arg(m_minutes, 2, 10, QLatin1Char('0'))
        .arg(m_seconds, 2, 10, QLatin1Char('0'))
        .arg(m_milliseconds, 3, 10, QLatin1Char('0'));
}

bool TimePoint::operator<(const TimePoint& other) const {
    if (m_hours != other.m_hours) return m_hours < other.m_hours;
    if (m_minutes != other.m_minutes) return m_minutes < other.m_minutes;
    if (m_seconds != other.m_seconds) return m_seconds < other.m_seconds;
    return m_milliseconds < other.m_milliseconds;
}

bool TimePoint::operator==(const TimePoint& other) const {
    return m_hours == other.m_hours &&
           m_minutes == other.m_minutes &&
           m_seconds == other.m_seconds &&
           m_milliseconds == other.m_milliseconds;
}

long long TimePoint::toMilliseconds() const {
    return (m_hours * 3600LL + m_minutes * 60LL + m_seconds) * 1000LL + m_milliseconds;
}

TimePoint TimePoint::fromMilliseconds(long long ms) {
    int hours = ms / 3600000;
    ms %= 3600000;
    int minutes = ms / 60000;
    ms %= 60000;
    int seconds = ms / 1000;
    int milliseconds = ms % 1000;
    return TimePoint(hours, minutes, seconds, milliseconds);
}