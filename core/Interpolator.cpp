#include "Interpolator.h"
#include <cmath>
#include <algorithm>
#include <QDebug>

double Interpolator::interpolate(const QVector<TimeMerge::TimePoint>& timePoints,
                                 const QVector<double>& values,
                                 const TimeMerge::TimePoint& targetTime,
                                 InterpolationMethod method) {
    if (timePoints.isEmpty() || values.isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    switch (method) {
        case InterpolationMethod::Nearest:
            return nearestNeighbor(timePoints, values, targetTime);
        case InterpolationMethod::Linear:
            return linearInterpolate(timePoints, values, targetTime);
        case InterpolationMethod::None:
            return std::numeric_limits<double>::quiet_NaN();
        default:
            return nearestNeighbor(timePoints, values, targetTime);
    }
}

double Interpolator::nearestNeighbor(const QVector<TimeMerge::TimePoint>& timePoints,
                                     const QVector<double>& values,
                                     const TimeMerge::TimePoint& targetTime) {
    if (timePoints.isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    int bestIdx = 0;
    int64_t bestDiff = std::abs((targetTime - timePoints[0]).toMicroseconds());
    
    for (int i = 1; i < timePoints.size(); ++i) {
        int64_t diff = std::abs((targetTime - timePoints[i]).toMicroseconds());
        if (diff < bestDiff) {
            bestDiff = diff;
            bestIdx = i;
        }
    }
    
    return values[bestIdx];
}

double Interpolator::linearInterpolate(const QVector<TimeMerge::TimePoint>& timePoints,
                                       const QVector<double>& values,
                                       const TimeMerge::TimePoint& targetTime) {
    if (timePoints.isEmpty()) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    if (targetTime <= timePoints[0]) {
        return values[0];
    }
    if (targetTime >= timePoints.last()) {
        return values.last();
    }
    
    int lowerIdx = -1;
    int upperIdx = -1;
    
    for (int i = 0; i < timePoints.size() - 1; ++i) {
        if (timePoints[i] <= targetTime && timePoints[i+1] >= targetTime) {
            lowerIdx = i;
            upperIdx = i + 1;
            break;
        }
    }
    
    if (lowerIdx == -1 || upperIdx == -1) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    
    int64_t t0 = timePoints[lowerIdx].toMicroseconds();
    int64_t t1 = timePoints[upperIdx].toMicroseconds();
    int64_t t = targetTime.toMicroseconds();
    
    double v0 = values[lowerIdx];
    double v1 = values[upperIdx];
    
    if (t1 == t0) {
        return v0;
    }
    
    double ratio = static_cast<double>(t - t0) / (t1 - t0);
    return v0 + ratio * (v1 - v0);
}
