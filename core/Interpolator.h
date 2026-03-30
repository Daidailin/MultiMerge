#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include "TimePoint.h"
#include <QVector>
#include <QString>
#include <QStringList>
#include <functional>
#include <limits>

enum class InterpolationMethod {
    Nearest,      ///< 最近邻插值：取最接近的时间点的值
    Linear        ///< 线性插值：在两个相邻点之间线性插值
};

struct InterpolationResult {
    double value;
    bool isValid;
    QString warningMessage;
    int64_t nearestDistanceUs;

    static InterpolationResult success(double val, int64_t distUs = 0) {
        return {val, true, QString(), distUs};
    }

    static InterpolationResult nan(const QString& warning = QString()) {
        return {std::numeric_limits<double>::quiet_NaN(), false, warning, -1};
    }
};

class Interpolator {
public:

    static InterpolationResult interpolate(const QVector<TimeMerge::TimePoint>& timePoints,
                                         const QVector<double>& values,
                                         const TimeMerge::TimePoint& targetTime,
                                         InterpolationMethod method,
                                         int64_t toleranceUs = -1);

    static InterpolationResult nearest(const QVector<TimeMerge::TimePoint>& timePoints,
                                      const QVector<double>& values,
                                      const TimeMerge::TimePoint& targetTime,
                                      int64_t toleranceUs = -1);

    static InterpolationResult linear(const QVector<TimeMerge::TimePoint>& timePoints,
                                      const QVector<double>& values,
                                      const TimeMerge::TimePoint& targetTime,
                                      int64_t toleranceUs = -1);

    static QStringList supportedMethods() {
        return {"nearest", "linear"};
    }

    static QString methodToString(InterpolationMethod method) {
        switch (method) {
            case InterpolationMethod::Nearest: return "nearest";
            case InterpolationMethod::Linear: return "linear";
            default: return "unknown";
        }
    }

    static InterpolationMethod stringToMethod(const QString& str) {
        if (str == "linear") return InterpolationMethod::Linear;
        return InterpolationMethod::Nearest;
    }

private:

    static int findNearestIndexInternal(const QVector<TimeMerge::TimePoint>& timePoints,
                                     const TimeMerge::TimePoint& targetTime,
                                     int64_t toleranceUs);

    static int64_t calculateMinDistance(const QVector<TimeMerge::TimePoint>& timePoints,
                                       const TimeMerge::TimePoint& targetTime);

    static int binarySearch(const QVector<TimeMerge::TimePoint>& timePoints,
                           const TimeMerge::TimePoint& targetTime);

    static InterpolationResult createToleranceWarning(int64_t distance, int64_t tolerance, const QString& context);
};

#endif // INTERPOLATOR_H
