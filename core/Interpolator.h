#ifndef INTERPOLATOR_H
#define INTERPOLATOR_H

#include "TimePoint.h"
#include <QVector>

enum class InterpolationMethod {
    Nearest,
    Linear,
    None
};

class Interpolator {
public:
    static double interpolate(const QVector<TimeMerge::TimePoint>& timePoints,
                             const QVector<double>& values,
                             const TimeMerge::TimePoint& targetTime,
                             InterpolationMethod method);
    static double nearestNeighbor(const QVector<TimeMerge::TimePoint>& timePoints,
                                 const QVector<double>& values,
                                 const TimeMerge::TimePoint& targetTime);
    static double linearInterpolate(const QVector<TimeMerge::TimePoint>& timePoints,
                                   const QVector<double>& values,
                                   const TimeMerge::TimePoint& targetTime);
};

#endif // INTERPOLATOR_H
