#include "TimePoint.h"
#include <sstream>
#include <iomanip>

TimePoint::TimePoint() : totalMilliseconds(0) {}

TimePoint::TimePoint(long long ms) : totalMilliseconds(ms) {}

TimePoint::TimePoint(int hour, int minute, int second, int millisecond) {
    totalMilliseconds = (hour * 3600LL + minute * 60LL + second) * 1000LL + millisecond;
}

bool TimePoint::fromString(const std::string& timeStr) {
    // 支持格式: HH:MM:SS.fff
    std::istringstream ss(timeStr);
    int hour, minute, second, millisecond = 0;
    char sep1, sep2, sep3;

    if (ss >> hour >> sep1 >> minute >> sep2 >> second >> sep3 >> millisecond) {
        if (sep1 == ':' && sep2 == ':' && (sep3 == '.' || sep3 == ',')) {
            totalMilliseconds = (hour * 3600LL + minute * 60LL + second) * 1000LL + millisecond;
            return true;
        }
    }

    // 尝试其他格式...
    return false;
}

std::string TimePoint::toString() const {
    long long ms = totalMilliseconds;
    int millisecond = ms % 1000;
    ms /= 1000;
    int second = ms % 60;
    ms /= 60;
    int minute = ms % 60;
    ms /= 60;
    int hour = ms;

    std::ostringstream ss;
    ss << std::setw(2) << std::setfill('0') << hour << ":";
    ss << std::setw(2) << std::setfill('0') << minute << ":";
    ss << std::setw(2) << std::setfill('0') << second << ".";
    ss << std::setw(3) << std::setfill('0') << millisecond;

    return ss.str();
}

long long TimePoint::getTotalMilliseconds() const {
    return totalMilliseconds;
}

bool TimePoint::operator<(const TimePoint& other) const {
    return totalMilliseconds < other.totalMilliseconds;
}

bool TimePoint::operator<=(const TimePoint& other) const {
    return totalMilliseconds <= other.totalMilliseconds;
}

bool TimePoint::operator>(const TimePoint& other) const {
    return totalMilliseconds > other.totalMilliseconds;
}

bool TimePoint::operator>=(const TimePoint& other) const {
    return totalMilliseconds >= other.totalMilliseconds;
}

bool TimePoint::operator==(const TimePoint& other) const {
    return totalMilliseconds == other.totalMilliseconds;
}

bool TimePoint::operator!=(const TimePoint& other) const {
    return totalMilliseconds != other.totalMilliseconds;
}

TimePoint TimePoint::operator+(long long ms) const {
    return TimePoint(totalMilliseconds + ms);
}

TimePoint TimePoint::operator-(long long ms) const {
    return TimePoint(totalMilliseconds - ms);
}

long long TimePoint::operator-(const TimePoint& other) const {
    return totalMilliseconds - other.totalMilliseconds;
}