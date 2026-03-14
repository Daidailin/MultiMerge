#ifndef TIMEPOINT_H
#define TIMEPOINT_H

#include <QString>
#include <cstdint>

namespace TimeMerge {

enum class TimeResolution {
    Milliseconds = 3,
    Microseconds = 6,
    Nanoseconds = 9
};

class TimePoint {
private:
    void normalize();
    int64_t maxFraction() const;
    
    int m_hour;
    int m_minute;
    int m_second;
    int64_t m_fraction;
    TimeResolution m_resolution;

public:
    explicit TimePoint(TimeResolution resolution = TimeResolution::Microseconds);
    TimePoint(int hour, int minute, int second, int64_t fraction = 0,
              TimeResolution resolution = TimeResolution::Microseconds);
    
    int hour() const { return m_hour; }
    int minute() const { return m_minute; }
    int second() const { return m_second; }
    int64_t fraction() const { return m_fraction; }
    TimeResolution resolution() const { return m_resolution; }
    
    int64_t toMicroseconds() const;
    int64_t toMilliseconds() const;
    int64_t toNanoseconds() const;
    QString toString(const QString& format = QString()) const;
    double toSeconds() const;
    static TimePoint fromSeconds(double totalSeconds, 
                                 TimeResolution resolution = TimeResolution::Microseconds);
    
    static constexpr int64_t microsecondsPerDay() { return 86400000000LL; }
    static constexpr int64_t millisecondsPerDay() { return 86400000LL; }
    static constexpr int64_t nanosecondsPerDay() { return 86400000000000LL; }
    
    bool operator==(const TimePoint& other) const;
    bool operator!=(const TimePoint& other) const;
    bool operator<(const TimePoint& other) const;
    bool operator<=(const TimePoint& other) const;
    bool operator>(const TimePoint& other) const;
    bool operator>=(const TimePoint& other) const;
    
    TimePoint operator+(int64_t microseconds) const;
    TimePoint operator-(int64_t microseconds) const;
    int64_t operator-(const TimePoint& other) const;
};

} // namespace TimeMerge

#endif // TIMEPOINT_H
