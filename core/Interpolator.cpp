#include "Interpolator.h"
#include <cmath>
#include <algorithm>

InterpolationResult Interpolator::interpolate(const QVector<TimeMerge::TimePoint>& timePoints,
                                           const QVector<double>& values,
                                           const TimeMerge::TimePoint& targetTime,
                                           InterpolationMethod method,
                                           int64_t toleranceUs) {
    if (timePoints.isEmpty() || values.isEmpty()) {
        return InterpolationResult::nan("Empty time points or values");
    }

    if (timePoints.size() != values.size()) {
        return InterpolationResult::nan("Time points and values size mismatch");
    }

    switch (method) {
        case InterpolationMethod::Nearest:
            return nearest(timePoints, values, targetTime, toleranceUs);
        case InterpolationMethod::Linear:
            return linear(timePoints, values, targetTime, toleranceUs);
        default:
            return InterpolationResult::nan("Unknown interpolation method");
    }
}

InterpolationResult Interpolator::nearest(const QVector<TimeMerge::TimePoint>& timePoints,
                                        const QVector<double>& values,
                                        const TimeMerge::TimePoint& targetTime,
                                        int64_t toleranceUs) {
    if (timePoints.isEmpty()) {
        return InterpolationResult::nan("Empty time points");
    }

    int index = findNearestIndexInternal(timePoints, targetTime, toleranceUs);

    if (index < 0) {
        int64_t diff = calculateMinDistance(timePoints, targetTime);
        return InterpolationResult::nan(
            QString("Nearest point out of tolerance. Target: %1, Distance: %2 us, Tolerance: %3 us")
                .arg(targetTime.toString())
                .arg(diff)
                .arg(toleranceUs)
        );
    }

    int64_t diff = qAbs(targetTime.toMicroseconds() - timePoints[index].toMicroseconds());
    return InterpolationResult::success(values[index], diff);
}

InterpolationResult Interpolator::linear(const QVector<TimeMerge::TimePoint>& timePoints,
                                      const QVector<double>& values,
                                      const TimeMerge::TimePoint& targetTime,
                                      int64_t toleranceUs) {
    if (timePoints.size() < 2) {
        return nearest(timePoints, values, targetTime, toleranceUs);
    }

    if (targetTime <= timePoints[0]) {
        int64_t diff = qAbs(targetTime.toMicroseconds() - timePoints[0].toMicroseconds());
        if (toleranceUs >= 0 && diff > toleranceUs) {
            return InterpolationResult::nan(
                QString("Target before data range, out of tolerance. Target: %1, First: %2, Distance: %3 us, Tolerance: %4 us")
                    .arg(targetTime.toString())
                    .arg(timePoints[0].toString())
                    .arg(diff)
                    .arg(toleranceUs)
            );
        }
        return InterpolationResult::success(values[0], diff);
    }

    if (targetTime >= timePoints[timePoints.size() - 1]) {
        int lastIdx = timePoints.size() - 1;
        int64_t diff = qAbs(targetTime.toMicroseconds() - timePoints[lastIdx].toMicroseconds());
        if (toleranceUs >= 0 && diff > toleranceUs) {
            return InterpolationResult::nan(
                QString("Target after data range, out of tolerance. Target: %1, Last: %2, Distance: %3 us, Tolerance: %4 us")
                    .arg(targetTime.toString())
                    .arg(timePoints[lastIdx].toString())
                    .arg(diff)
                    .arg(toleranceUs)
            );
        }
        return InterpolationResult::success(values[lastIdx], diff);
    }

    int pos = binarySearch(timePoints, targetTime);

    if (pos <= 0 || pos >= timePoints.size()) {
        return nearest(timePoints, values, targetTime, toleranceUs);
    }

    const TimeMerge::TimePoint& t1 = timePoints[pos - 1];
    const TimeMerge::TimePoint& t2 = timePoints[pos];
    double v1 = values[pos - 1];
    double v2 = values[pos];

    if (qIsNaN(v1) || qIsNaN(v2)) {
        if (!qIsNaN(v1)) return InterpolationResult::success(v1);
        if (!qIsNaN(v2)) return InterpolationResult::success(v2);
        return InterpolationResult::nan("Both interpolation points are NaN");
    }

    int64_t t1_us = t1.toMicroseconds();
    int64_t t2_us = t2.toMicroseconds();
    int64_t t_us = targetTime.toMicroseconds();

    if (t2_us == t1_us) {
        return InterpolationResult::success(v1, 0);
    }

    int64_t dist1 = t_us - t1_us;
    int64_t dist2 = t2_us - t_us;

    if (toleranceUs >= 0) {
        if (dist1 > toleranceUs || dist2 > toleranceUs) {
            return InterpolationResult::nan(
                QString("Interpolation points out of tolerance. Before: %1 us, After: %2 us, Tolerance: %3 us")
                    .arg(dist1)
                    .arg(dist2)
                    .arg(toleranceUs)
            );
        }
    }

    double ratio = static_cast<double>(dist1) / static_cast<double>(t2_us - t1_us);
    double result = v1 + (v2 - v1) * ratio;

    int64_t nearestDist = qMin(dist1, dist2);
    return InterpolationResult::success(result, nearestDist);
}

int Interpolator::findNearestIndexInternal(const QVector<TimeMerge::TimePoint>& timePoints,
                                         const TimeMerge::TimePoint& targetTime,
                                         int64_t toleranceUs) {
    if (timePoints.isEmpty()) {
        return -1;
    }

    if (targetTime <= timePoints[0]) {
        int64_t diff = qAbs(targetTime.toMicroseconds() - timePoints[0].toMicroseconds());
        if (toleranceUs < 0 || diff <= toleranceUs) {
            return 0;
        }
        return -1;
    }

    if (targetTime >= timePoints[timePoints.size() - 1]) {
        int lastIdx = timePoints.size() - 1;
        int64_t diff = qAbs(targetTime.toMicroseconds() - timePoints[lastIdx].toMicroseconds());
        if (toleranceUs < 0 || diff <= toleranceUs) {
            return lastIdx;
        }
        return -1;
    }

    int pos = binarySearch(timePoints, targetTime);

    if (pos <= 0) {
        return 0;
    }
    if (pos >= timePoints.size()) {
        return timePoints.size() - 1;
    }

    int64_t diff1 = qAbs(targetTime.toMicroseconds() - timePoints[pos - 1].toMicroseconds());
    int64_t diff2 = qAbs(targetTime.toMicroseconds() - timePoints[pos].toMicroseconds());

    int nearestIndex = (diff1 <= diff2) ? (pos - 1) : pos;
    int64_t nearestDiff = qMin(diff1, diff2);

    if (toleranceUs >= 0 && nearestDiff > toleranceUs) {
        return -1;
    }

    return nearestIndex;
}

int64_t Interpolator::calculateMinDistance(const QVector<TimeMerge::TimePoint>& timePoints,
                                         const TimeMerge::TimePoint& targetTime) {
    if (timePoints.isEmpty()) {
        return -1;
    }

    int64_t minDiff = qAbs(targetTime.toMicroseconds() - timePoints[0].toMicroseconds());

    for (int i = 1; i < timePoints.size(); ++i) {
        int64_t diff = qAbs(targetTime.toMicroseconds() - timePoints[i].toMicroseconds());
        if (diff < minDiff) {
            minDiff = diff;
        }
    }

    return minDiff;
}

int Interpolator::binarySearch(const QVector<TimeMerge::TimePoint>& timePoints,
                              const TimeMerge::TimePoint& targetTime) {
    int left = 0;
    int right = timePoints.size() - 1;

    while (left <= right) {
        int mid = left + (right - left) / 2;

        if (timePoints[mid] == targetTime) {
            return mid;
        }

        if (timePoints[mid] < targetTime) {
            left = mid + 1;
        } else {
            right = mid - 1;
        }
    }

    return left;
}

InterpolationResult Interpolator::createToleranceWarning(int64_t distance,
                                                       int64_t tolerance,
                                                       const QString& context) {
    return InterpolationResult::nan(
        QString("%1: Out of tolerance. Distance: %2 us, Tolerance: %3 us")
            .arg(context)
            .arg(distance)
            .arg(tolerance)
    );
}
